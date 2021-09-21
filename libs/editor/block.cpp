#include "block.h"
#include "document.h"
#include "util.h"

static size_t blocksCreated = 0;

static const char* utf8_to_codepoint(const char* p, unsigned* dst)
{
    unsigned res, n;
    switch (*p & 0xf0) {
    case 0xf0:
        res = *p & 0x07;
        n = 3;
        break;
    case 0xe0:
        res = *p & 0x0f;
        n = 2;
        break;
    case 0xd0:
    case 0xc0:
        res = *p & 0x1f;
        n = 1;
        break;
    default:
        res = *p;
        n = 0;
        break;
    }
    while (n--) {
        res = (res << 6) | (*(++p) & 0x3f);
    }
    *dst = res;
    return p + 1;
}

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

    if (file) {
        file->seekg(filePosition, file->beg);
        size_t pos = file->tellg();
        std::string line;
        if (std::getline(*file, line)) {
            return std::wstring(line.begin(), line.end());
        }
    }

    return L"";
}

std::string block_t::utf8_text()
{
    if (dirty) {
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

        char ch = cp < 0xff ? cp : '?';
        content += ch;

        wchar_t wc[2] = { cp, 0 };
        wcontent += wc;
    }

    content += "\n";
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
