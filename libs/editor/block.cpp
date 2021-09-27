#include "block.h"
#include "document.h"
#include "util.h"
#include "utf8.h"

#include <algorithm>

static size_t blocksCreated = 0;
static size_t blockUID = 0xff;

blockdata_t::blockdata_t()
    : dirty(true)
    , folded(false)
    , foldable(false)
    , foldedBy(0)
    , dots(0)
    , indent(0)
    , lastPrevBlockRule(0)
{
}

blockdata_t::~blockdata_t()
{
    if (dots) {
        free(dots);
    }
}

block_t::block_t()
    : document(0)
    , file(0)
    , filePosition(0)
    , lineNumber(0)
    , lineCount(0)
    , data(0)
    , dirty(false)
    , cachedLength(0)
    , x(-1)
    , y(-1)
{
    uid = blockUID++;
    blocksCreated++;
}

block_t::~block_t()
{
    blocksCreated--;
    // std::cout << "blocks: " << blocksCreated << " : " << text() << std::endl;
}

std::string block_t::text()
{
    if (dirty) {
        if (!content.length() && wcontent.length()) {
            for(auto wc : wcontent) {
                content += (char)(wc & 0xff);
            }
        }
        return content;
    }

    if (file) {
        file->seekg(filePosition, file->beg);
        size_t pos = file->tellg();
        std::string line;
        if (std::getline(*file, line)) {
            return line;
        }
    }

    return "";
}

std::wstring block_t::wide_text()
{
    if (dirty) {
        return wcontent;
    }
    
    setText(text());

    return wcontent;
}

std::string block_t::utf8_text()
{
    return wstring_to_utf8string(wide_text());
}

void block_t::setText(std::string t)
{
    dirty = true;
    
    content = "";
    wcontent = L"";

    if (data) {
        data->dirty = true;
        // if (data->folded && data->foldable) {
        //     document->editor->toggleFold(lineNumber);
        // }
    }

    char *p = (char*)t.c_str();
    while(*p) {
        unsigned cp;
        p = (char*)utf8_to_codepoint(p, &cp);
        // wchar_t wc[2] = { (wchar_t)cp, 0 };
        wcontent += (wchar_t)cp;
    }

    cachedLength = 0;
}

void block_t::setWText(std::wstring t)
{
    dirty = true;
    
    content = "";
    wcontent = t;

    if (data) {
        data->dirty = true;
        // if (data->folded && data->foldable) {
        //     document->editor->toggleFold(lineNumber);
        // }
    }

    cachedLength = 0;
}

size_t block_t::length()
{
    if (cachedLength == 0) {
        cachedLength = wcontent.length() + 1;
    }
    return cachedLength;
}

block_ptr block_t::next()
{
    return document->nextBlock(this);
}

block_ptr block_t::previous()
{
    return document->previousBlock(this);
}

void block_t::print()
{
    // std::cout << lineNumber << ": " << text() << std::endl;
    log("%d %s", lineNumber, text().c_str());
}

bool block_t::isValid()
{
    block_ptr b = document->blockAtLine(lineNumber + 1);
    return (b.get() == this);
}


static std::vector<span_info_t> splitSpan(span_info_t si, const std::string& str)
{
    static std::set<char> delimiters = {
        '.', ',', ';', ':',
        '-', '+', '*', '/', '%', '=',
        '"', ' ', '\'', '\\',
        '(', ')', '[', ']', '<', '>',
        '&', '!', '?', '_', '~', '@'
    };

    std::vector<span_info_t> result;

    char const* line = str.c_str();
    char const* pch = str.c_str();
    char const* start = pch;
    for (; *pch; ++pch) {
        if (delimiters.find(*pch) != delimiters.end()) {
            span_info_t s = si;
            s.start = start - line;
            s.length = pch - start;
            result.push_back(s);
            start = pch;

            // std::string t(line + s.start, s.length);
            // printf(">%d %d >%s<\n", s.start, s.length, t.c_str());
        }
    }

    span_info_t s = si;
    s.start = start - line;
    s.length = pch - start;
    if (start == pch) {
        s.length = 1;
    }
    result.push_back(s);

    return result;
}

static bool compareSpan(span_info_t a, span_info_t b)
{
    return a.start < b.start;
}

std::vector<span_info_t> block_t::layoutSpan(int cols, bool wrap, int indent)
{
    std::vector<span_info_t> spans;
    if (!data) {
        return spans;
    }

    std::string text = this->text();

    std::vector<span_info_t> source_spans = data->spans;
    if (!source_spans.size()) {
        span_info_t span = {
                start : 0,
                length : (int)length(),
                colorIndex : 0,
                bold : false,
                italic : false,
                state : BLOCK_STATE_UNKNOWN,
                scope : ""
            };
        source_spans.push_back(span);
    }

    lineCount = 1;

    // wrap
    if (wrap && text.length() > cols) {
        spans.clear();
        for (auto& s : source_spans) {
            if (s.length == 0)
                continue;
            std::string span_text = text.substr(s.start, s.length);
            std::vector<span_info_t> ss = splitSpan(s, span_text);
            for (auto _s : ss) {
                _s.start += s.start;
                spans.push_back(_s);
            }
        }

        std::sort(spans.begin(), spans.end(), compareSpan);

        int line = 0;
        int line_x = 0;
        for (auto& _s : spans) {
            if (_s.start - (line * cols) + _s.length > cols) {
                line++;
                line_x = 0;
            }
            _s.line = line;
            if (_s.line > 0) {
                _s.line_x = indent + line_x;
                line_x += _s.length;
            }
            std::string span_text = text.substr(_s.start, _s.length);
        }

        lineCount = line;
    } else {
        spans = data->spans;
    }

    return spans;
}