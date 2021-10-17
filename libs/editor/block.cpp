#include "block.h"
#include "document.h"
#include "editor.h"
#include "utf8.h"
#include "util.h"

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
    , lineNumber(0)
    , lineCount(0)
    , data(0)
    , dirty(false)
    , cachedLength(0)
{
    uid = blockUID++;
    blocksCreated++;
}

block_t::~block_t()
{
    blocksCreated--;
}

std::string block_t::text()
{
    if (dirty) {
        // ...
    }
    return content;
}

void block_t::setText(std::string t)
{
    dirty = true;
    content = t;

    if (data) {
        data->dirty = true;
        if (data->folded && data->foldable) {
            document->editor->toggleFold(lineNumber);
        }
    }

    cachedLength = 0;
}

size_t block_t::length()
{
    if (cachedLength == 0) {
        cachedLength = utf8_length(content) + 1;
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
