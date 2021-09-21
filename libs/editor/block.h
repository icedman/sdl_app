#ifndef BLOCK_H
#define BLOCK_H

#include "grammar.h"

#include <fstream>
#include <istream>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct document_t;
struct block_t;
struct cursor_t;

typedef std::shared_ptr<block_t> block_ptr;
typedef std::vector<block_ptr> block_list;

enum block_state_e {
    BLOCK_STATE_UNKNOWN = 0,
    BLOCK_STATE_COMMENT = 1 << 4,
    BLOCK_STATE_STRING = 1 << 5
};

struct span_info_t {
    int start;
    int length;
    int colorIndex;
    bool bold;
    bool italic;
    block_state_e state;
    std::string scope;

    // for rendered span
    int x;
    int y;
    int line;
    int line_x;
};

struct bracket_info_t {
    size_t line;
    size_t position;
    int bracket;
    bool open;
    bool unpaired;
};

struct blockdata_t {
    blockdata_t();
    ~blockdata_t();

    std::vector<span_info_t> spans;
    std::vector<span_info_t> rendered_spans;
    std::vector<bracket_info_t> foldingBrackets;
    std::vector<bracket_info_t> brackets;
    parse::stack_ptr parser_state;
    // std::map<size_t, scope::scope_t> scopes;

    int* dots;

    size_t lastRule;
    size_t lastPrevBlockRule;
    block_state_e state;

    bool dirty;
    bool folded;
    bool foldable;
    size_t foldedBy;
    int indent;
    bool ifElseHack;
};

struct block_t {
    block_t();
    ~block_t();

    struct document_t* document;
    size_t lineNumber;
    size_t originalLineNumber;

    int cachedLength;
    int lineCount;
    int lineHeight;

    std::string content;
    std::wstring wcontent;
    std::ifstream* file;
    size_t filePosition;
    bool dirty;

    std::string text();
    std::wstring wide_text();
    std::string utf8_text();
    
    void setText(std::string text);
    void setWText(std::wstring text);
    void print();
    size_t length();

    bool isValid();

    block_ptr next();
    block_ptr previous();

    std::shared_ptr<blockdata_t> data;

    // rendered block position
    int x;
    int y;
};

#endif // BLOCK_H
