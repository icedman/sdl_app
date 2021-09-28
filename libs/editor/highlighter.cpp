#include "highlighter.h"
#include "app.h"
#include "document.h"
#include "editor.h"
#include "indexer.h"
#include "parse.h"
#include "util.h"

#include <cstring>
#include <unistd.h>

#define LINE_LENGTH_LIMIT 500

highlighter_t::highlighter_t()
    : threadId(0)
    , editor(0)
    , requestIdx(0)
    , processIdx(0)
    , _paused(false)
{
    clearRequests();
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

void highlighter_t::clearRequests()
{
    for (int i = 0; i < HIGHLIGHT_REQUEST_SIZE; i++) {
        highlightRequests[i] = nullptr;
    }
    processIdx = 0;
    requestIdx = 0;
}

void highlighter_t::_requestHighlightBlock(block_ptr block, bool priority)
{
    if (!threadId || _paused) {
        highlightBlock(block);
    }

    if (_paused)
        return;

    if (block->data && !block->data->dirty) {
        return;
    }

    for (int i = 0; i < HIGHLIGHT_REQUEST_SIZE; i++) {
        if (highlightRequests[i] == block) {
            // already pending
            return;
        }
    }

    // ensures thread is not reading text from file buffer
    block->wideText();

    highlightRequests[requestIdx++] = block;
    if (requestIdx >= HIGHLIGHT_REQUEST_SIZE) {
        requestIdx = 0;
    }

    return;
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
    // blockData->scopes.clear();
    blockData->spans.clear();

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

        // test state serializationn
        // parse::stack_serialized_t ss = lang->grammar->serialize_state(parser_state);
        // parser_state = lang->grammar->unserialize_state(ss);
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
    pthread_t threadId;
    editor_ptr editor;
    size_t start;
    size_t count;
};

volatile int running_threads = 0;
pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;

static
void sleep(int ms)
{
    struct timespec waittime;

    waittime.tv_sec = (ms / 1000);
    ms = ms % 1000;
    waittime.tv_nsec = ms * 1000 * 1000;

    nanosleep( &waittime, NULL);
}

void* highlightThread(void* arg)
{
    highlight_thread_t* threadHl = (highlight_thread_t*)arg;
    editor_ptr editor = threadHl->editor;

    printf("start:%d count:%d\n", threadHl->start, threadHl->count);

    editor->highlight(threadHl->start, threadHl->count);

    pthread_mutex_lock(&running_mutex);
    running_threads--;
    pthread_mutex_unlock(&running_mutex);

    return NULL;
}

void highlighter_t::run(editor_t* _editor)
{
    if (!_editor->document.blocks.size()) {
        return;
    }

    if (!_editor->highlighter.lang) {
        return;
    }

    this->editor = _editor;

    #define THREAD_COUNT 8
    #define MIN_PER_PAGE 2000
    // printf("lines: %d\n", editor->document.blocks.size());
    int per_page = editor->document.blocks.size() / THREAD_COUNT;
    if (per_page < MIN_PER_PAGE) {
        // highlight first page
        int c = 0;
        block_ptr b = editor->document.firstBlock();
        while (b && c++ < MIN_PER_PAGE) {
            b->wideText();
            editor->highlighter.highlightBlock(b);
            b = b->next();
        }
        return;
    }

    // printf("per page: %d\n", per_page);

    highlight_thread_t threads[THREAD_COUNT];

    for(int i=0;i<THREAD_COUNT;i++) {
        
        editor_ptr editor = std::make_shared<editor_t>();
        editor->pushOp("OPEN", _editor->document.fullPath);
        editor->runAllOps();

        threads[i].editor = editor;
        threads[i].start = i * per_page;
        threads[i].count = per_page;

        editor->highlighter.lang = editor->highlighter.lang;
        editor->highlighter.theme = _editor->highlighter.theme;

        pthread_create(&threads[i].threadId, NULL, &highlightThread, &threads[i]);
        pthread_mutex_lock(&running_mutex);
        running_threads++;
        pthread_mutex_unlock(&running_mutex);
    }

    while (running_threads) {
        sleep(50);
    }

    block_list::iterator mit = this->editor->document.blocks.begin();
    for(int i=0;i<THREAD_COUNT;i++) {
        block_list::iterator it = threads[i].editor->document.blocks.begin();
        it += threads[i].start;
        for(int j=0;j<per_page;j++) {
            block_ptr bm = *mit++;
            block_ptr b = *it++;
            bm->data = b->data;
        }
    }

    printf("done\n");
}

void highlighter_t::cancel()
{
    if (threadId) {
        pthread_cancel(threadId);
        threadId = 0;
    }
}

void highlighter_t::pause()
{
    if (_paused || !threadId) {
        return;
    }
    _paused = true;
    clearRequests();
    usleep(5000);
}

void highlighter_t::resume()
{
    _paused = false;
}
