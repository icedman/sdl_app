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

struct rgba_t { int r; int g; int b; int a; };
    
struct span_info_t {
    int start;
    int length;
    rgba_t fg;
    rgba_t bg;
    bool bold;
    bool italic;
    bool underline;
    block_state_e state;
    std::string scope;
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
    std::vector<bracket_info_t> foldingBrackets;
    std::vector<bracket_info_t> brackets;

    parse::stack_ptr parser_state;

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

typedef std::shared_ptr<blockdata_t> blockdata_ptr;

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
    bool dirty;

    std::string text();

    void setText(std::string text);
    void print();
    size_t length();

    bool isValid();

    block_ptr next();
    block_ptr previous();

    blockdata_ptr data;

    size_t uid;
};

#endif // BLOCK_H
