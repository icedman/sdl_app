#include "highlighter.h"
#include "app.h"
#include "backend.h"
#include "document.h"
#include "editor.h"
#include "indexer.h"
#include "parse.h"
#include "util.h"

#include <cstring>
#include <unistd.h>

#define LINE_LENGTH_LIMIT 500
#define MAX_THREAD_COUNT 32
#define LINES_PER_THREAD 400

highlighter_t::highlighter_t()
    : editor(0)
{
}

struct span_info_t spanAtBlock(struct blockdata_t* blockData, int pos, bool rendered)
{
    span_info_t res;
    res.bold = false;
    res.italic = false;
    res.length = 0;
    res.state = BLOCK_STATE_UNKNOWN;
    if (!blockData) {
        return res;
    }

    std::vector<span_info_t>& spans = blockData->spans;
    if (rendered) {
        spans = blockData->rendered_spans;
    }

    for (auto span : spans) {
        if (span.length == 0) {
            continue;
        }
        if (pos >= span.start && pos < span.start + span.length) {
            res = span;
            if (res.state == BLOCK_STATE_UNKNOWN) {
                // log("unknown >>%d", span.start);
            }
            return res;
        }
    }

    return res;
}

int highlighter_t::highlightBlocks(block_ptr block, int count)
{
    int lighted = 0;
    while (block && count-- > 0) {
        lighted += highlightBlock(block);
        block = block->next();
    }

    return lighted;
}

static void addCommentSpan(std::vector<span_info_t>& spans, span_info_t comment)
{
    std::vector<span_info_t>::iterator it = spans.begin();
    while (it != spans.end()) {
        span_info_t s = *it;
        if (s.start >= comment.start && s.start + s.length <= comment.start + comment.length) {
            spans.erase(it);
            continue;
        }
        it++;
    }
    // for(auto &s : spans) {
    //     if (s.start >= comment.start && s.start + s.length <= comment.start + comment.length) {
    //         s.length = 0;
    //     }
    // }
    spans.insert(spans.begin(), 1, comment);
}

int highlighter_t::highlightBlock(block_ptr block)
{
    if (!block || !lang)
        return 0;

    if (!block->data) {
        block->data = std::make_shared<blockdata_t>();
        block->data->dirty = true;
    }

    if (!block->data->dirty) {
        return 0;
    }

    // log("hl %d", block->lineNumber);

    blockdata_ptr blockData = block->data;

    bool previousState = blockData->state;
    block_state_e previousBlockState = BLOCK_STATE_UNKNOWN;

    block_ptr prevBlock = block->previous();
    blockdata_ptr prevBlockData = NULL;
    if (prevBlock) {
        prevBlockData = prevBlock->data;
    }

    std::string text = block->text();
    std::string str = text;
    str += "\n";

    //--------------
    // for minimap line cache
    //--------------
    for (int i = 0; i < 4; i++) {
        int ln = block->lineNumber - 4 + i;
        if (ln < 0 || ln >= block->document->blocks.size()) {
            ln = 0;
        }
        block_ptr pb = block->document->blockAtLine(ln);
        if (pb->data && pb->data->dots) {
            free(pb->data->dots);
            pb->data->dots = 0;
        }
    }

    //--------------
    // call the grammar parser
    //--------------
    bool firstLine = true;
    parse::stack_ptr parser_state = NULL;
    std::map<size_t, scope::scope_t> scopes;
    blockData->spans.clear();
    blockData->rendered_spans.clear();

    const char* first = str.c_str();
    const char* last = first + text.length() + 1;

    if (prevBlockData) {
        previousBlockState = prevBlockData->state;
        parser_state = prevBlockData->parser_state;
        if (parser_state && parser_state->rule) {
            blockData->lastPrevBlockRule = parser_state->rule->rule_id;
        }
        firstLine = !(parser_state != NULL);
    }

    if (!parser_state) {
        parser_state = lang->grammar->seed();
        firstLine = true;
    }

    if (text.length() > LINE_LENGTH_LIMIT) {
        // too long to parse
        blockData->dirty = false;
        return 1;
    } else {
        parser_state = parse::parse(first, last, parser_state, scopes, firstLine);
    }

    std::map<size_t, scope::scope_t>::iterator it = scopes.begin();
    size_t n = 0;
    size_t l = block->length();
    while (it != scopes.end()) {
        n = it->first;
        scope::scope_t scope = it->second;
        std::string scopeName(scope);
        style_t s = theme->styles_for_scope(scopeName);

        block_state_e state;
        if (scopeName.find("comment") != std::string::npos) {
            state = BLOCK_STATE_COMMENT;
        } else if (scopeName.find("string") != std::string::npos) {
            state = BLOCK_STATE_STRING;
        } else {
            state = BLOCK_STATE_UNKNOWN;
        }

        style_t style = theme->styles_for_scope(scopeName);
        span_info_t span = {
            .start = (int)n,
            .length = (int)(l - n),
            .colorIndex = style.foreground.index,
            .bold = style.bold == bool_true,
            .italic = style.italic == bool_true,
            .state = state,
            .scope = scopeName
        };

        if (blockData->spans.size() > 0) {
            span_info_t& prevSpan = blockData->spans.front();
            prevSpan.length = n - prevSpan.start;
        }

        span.x = 0;
        span.y = 0;
        span.line = 0;

        blockData->spans.insert(blockData->spans.begin(), 1, span);
        // blockData->spans.push_back(span);
        it++;
    }

    //----------------------
    // langauge config
    //----------------------

    //----------------------
    // find block comments
    //----------------------
    if (lang->blockCommentStart.length()) {
        size_t beginComment = text.find(lang->blockCommentStart);
        size_t endComment = text.find(lang->blockCommentEnd);
        style_t s = theme->styles_for_scope("comment");

        if (endComment == std::string::npos && (beginComment != std::string::npos || previousBlockState == BLOCK_STATE_COMMENT)) {
            blockData->state = BLOCK_STATE_COMMENT;
            int b = beginComment != std::string::npos ? beginComment : 0;
            int e = endComment != std::string::npos ? endComment : (last - first);

            span_info_t span = {
                .start = b,
                .length = e - b,
                .colorIndex = s.foreground.index,
                .bold = s.bold == bool_true,
                .italic = s.italic == bool_true,
                .state = BLOCK_STATE_COMMENT,
                .scope = "comment"
            };

            addCommentSpan(blockData->spans, span);
            // blockData->spans.insert(blockData->spans.begin(), 1, span);

        } else if (beginComment != std::string::npos && endComment != std::string::npos) {
            blockData->state = BLOCK_STATE_UNKNOWN;
            int b = beginComment;
            int e = endComment + lang->blockCommentEnd.length();

            span_info_t span = {
                .start = b,
                .length = e - b,
                .colorIndex = s.foreground.index,
                .bold = s.bold == bool_true,
                .italic = s.italic == bool_true,
                .state = BLOCK_STATE_COMMENT,
                .scope = "comment"
            };

            addCommentSpan(blockData->spans, span);
            // blockData->spans.insert(blockData->spans.begin(), 1, span);

        } else {
            blockData->state = BLOCK_STATE_UNKNOWN;
            if (endComment != std::string::npos && previousBlockState == BLOCK_STATE_COMMENT) {
                // setFormatFromStyle(0, endComment + lang->blockCommentEnd.length(), s, first, blockData, "comment");
                span_info_t span = {
                    .start = 0,
                    .length = (int)(endComment + lang->blockCommentEnd.length()),
                    .colorIndex = s.foreground.index,
                    .bold = s.bold == bool_true,
                    .italic = s.italic == bool_true,
                    .state = BLOCK_STATE_UNKNOWN,
                    .scope = "comment"
                };
                addCommentSpan(blockData->spans, span);
                // blockData->spans.insert(blockData->spans.begin(), 1, span);
            }
        }
    }

    if (lang->lineComment.length()) {
        // comment out until the end
    }

    //----------------------
    // reset brackets
    //----------------------
    blockData->brackets.clear();
    blockData->foldable = false;
    blockData->foldingBrackets.clear();
    blockData->ifElseHack = false;
    gatherBrackets(block, (char*)first, (char*)last);

    blockData->parser_state = parser_state;
    blockData->dirty = false;
    blockData->lastRule = parser_state->rule->rule_id;
    if (blockData->state == BLOCK_STATE_COMMENT) {
        blockData->lastRule = BLOCK_STATE_COMMENT;
    }

    //----------------------
    // mark next block for highlight
    // .. if necessary
    //----------------------
    if (blockData->lastRule) {
        block_ptr next = block->next();
        if (next && next->isValid()) {
            blockdata_ptr nextBlockData = next->data;
            if (nextBlockData && blockData->lastRule != nextBlockData->lastPrevBlockRule) {
                nextBlockData->dirty = true;
            }

            if (nextBlockData && (blockData->lastRule == BLOCK_STATE_COMMENT || nextBlockData->lastRule == BLOCK_STATE_COMMENT)) {
                nextBlockData->dirty = true;
            }
        }
    }

    if (editor && editor->indexer) {
        editor->indexer->requestIndexBlock(block);
    }

    return 1;
}

void highlighter_t::gatherBrackets(block_ptr block, char* first, char* last)
{
    blockdata_ptr blockData = block->data;
    if (!block->data) {
        block->data = std::make_shared<blockdata_t>();
        block->data->dirty = true;
        blockData = block->data;
    }

    if (blockData->brackets.size()) {
        return;
    }

    if (lang->brackets) {
        std::vector<bracket_info_t> brackets;
        for (char* c = (char*)first; c < last;) {
            bool found = false;

            struct span_info_t span = spanAtBlock(blockData.get(), c - first);
            if (span.length && (span.state == BLOCK_STATE_COMMENT || span.state == BLOCK_STATE_STRING)) {
                c++;
                continue;
            }

            // opening
            int i = 0;
            for (auto b : lang->bracketOpen) {
                if (strstr(c, b.c_str()) == c) {
                    found = true;
                    size_t l = (c - first);
                    brackets.push_back({ .line = block->lineNumber,
                        .position = l,
                        // .absolutePosition = block->position + l,
                        .bracket = i,
                        .open = true });
                    c += b.length();
                    break;
                }
            }

            if (found) {
                continue;
            }

            // closing
            i = 0;
            for (auto b : lang->bracketClose) {
                if (strstr(c, b.c_str()) == c) {
                    found = true;
                    size_t l = (c - first);
                    brackets.push_back({ .line = block->lineNumber,
                        .position = l,
                        //.absolutePosition = block->position + l,
                        .bracket = i,
                        .open = false });
                    c += b.length();
                    break;
                }
            }

            if (found) {
                continue;
            }

            c++;
        }

        blockData->brackets = brackets;

        // bracket pairing
        for (auto b : brackets) {
            if (!b.open && blockData->foldingBrackets.size()) {
                auto l = blockData->foldingBrackets.back();
                if (l.open && l.bracket == b.bracket) {
                    blockData->foldingBrackets.pop_back();
                } else {
                    // std::cout << "error brackets" << std::endl;
                }
                continue;
            }
            blockData->foldingBrackets.push_back(b);
        }

        // hack for if-else-
        if (blockData->foldingBrackets.size() == 2) {
            if (blockData->foldingBrackets[0].open != blockData->foldingBrackets[1].open && blockData->foldingBrackets[0].bracket == blockData->foldingBrackets[1].bracket) {
                blockData->foldingBrackets.clear();
                blockData->ifElseHack = true;
            }
        }

        if (blockData->foldingBrackets.size()) {
            auto l = blockData->foldingBrackets.back();
            blockData->foldable = l.open;
        }

        // log("brackets %d %d", blockData->brackets.size(), blockData->foldingBrackets.size());
    }
}

struct highlight_thread_t {
    int index;
    pthread_t threadId;
    editor_ptr editor;
    size_t start;
    size_t count;
    bool done;
};

volatile int running_threads = 0;
pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;
static highlight_thread_t threads[MAX_THREAD_COUNT];

bool highlighter_t::hasRunningThreads()
{
    pthread_mutex_lock(&running_mutex);
    bool res = running_threads > 0;
    pthread_mutex_unlock(&running_mutex);
    return res;
}

static void sleep(int ms)
{
    struct timespec waittime;

    waittime.tv_sec = (ms / 1000);
    ms = ms % 1000;
    waittime.tv_nsec = ms * 1000 * 1000;

    nanosleep(&waittime, NULL);
}

void* highlightThread(void* arg)
{
    highlight_thread_t* threadHl = (highlight_thread_t*)arg;
    editor_ptr editor = threadHl->editor;

    // log("start:%d count:%d\n", threadHl->start, threadHl->count);

    // int lighted = 0;
    // for (int i = 0; i < threadHl->count; i += 1000) {
    //     int c = 1000;
    //     if (i + c > threadHl->count) {
    //         c = threadHl->count - i;
    //     }
    //     lighted += editor->highlight(threadHl->start + i, c);
    //     sleep(10);
    // }
    // threadHl->count = lighted;

    threadHl->count = editor->highlight(threadHl->start, threadHl->count);

    pthread_mutex_lock(&running_mutex);
    running_threads--;
    pthread_mutex_unlock(&running_mutex);

    // log("done %d %d\n", (int)threadHl->index, lighted);
    return NULL;
}

void highlighter_t::run(editor_t* _editor)
{
    if (_editor->document.blocks.size() <= 20) {
        return;
    }

    this->editor = _editor;

    size_t lines = editor->document.blocks.size();

    log("lines: %d\n", lines);
    int per_thread = (float)lines / MAX_THREAD_COUNT;
    if (per_thread < LINES_PER_THREAD) {
        per_thread = LINES_PER_THREAD;
    }

    int thread_count = 0.5f + ((float)lines / per_thread);
    if (thread_count == 0) thread_count = 1;

    log("threads: %d per threads: %d\n", thread_count, per_thread);

    backend_t::instance()->ticks();
    
    running_threads = 0;

    for (int i = 0; i < thread_count; i++) {
        editor_ptr editor = std::make_shared<editor_t>();
        editor->document.blocks = _editor->document.blocks;
    
        if (i % 8 == 0)
        editor->highlighter.lang = language_from_file(_editor->document.fullPath, app_t::instance()->extensions);
        else
        editor->highlighter.lang = _editor->highlighter.lang;

        editor->highlighter.theme = _editor->highlighter.theme;

        threads[i].index = i;
        threads[i].editor = editor;
        threads[i].start = i * per_thread;
        threads[i].count = per_thread;

        if (i + 1 == thread_count) {
            size_t total = (i  + 1) * per_thread;
            threads[i].count += (lines - total);
        }

        editor->highlight(threads[i].start, 4);
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_create(&threads[i].threadId, NULL, &highlightThread, &threads[i]);
        pthread_mutex_lock(&running_mutex);
        running_threads++;
        pthread_mutex_unlock(&running_mutex);
    }

    while (running_threads) {
        sleep(10);
    }

    // re-highlight start of threads .. to reconsider previous block
    for (int i = 0; i < thread_count; i++) {
        editor->document.blockAtLine(threads[i].start)->data->dirty = true;
        editor->highlight(threads[i].start, 10);
    }

    log("whole document highlighting done in %fs\n", (float)backend_t::instance()->ticks() / 1000);
}
