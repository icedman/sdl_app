#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include "block.h"
#include "cursor.h"
#include "extension.h"
#include "grammar.h"
#include "theme.h"

#include <functional>
#include <pthread.h>
#include <string>
#include <vector>

typedef std::function<bool(int)> highlight_callback_t;

struct editor_t;
struct highlighter_t {
    language_info_ptr lang;
    theme_ptr theme;

    highlighter_t();

    int highlightBlocks(block_ptr block, int count = 1);
    int highlightBlock(block_ptr block);
    void updateBrackets(block_ptr block);
    void run(editor_t* editor);

    static bool hasRunningThreads();
    
    editor_t* editor;

    highlight_callback_t callback;
};

span_info_t spanAtBlock(struct blockdata_t* blockData, int pos, bool rendered = false);

#endif // HIGHLIGHTER_H
