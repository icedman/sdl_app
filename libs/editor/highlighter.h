#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include "block.h"
#include "cursor.h"
#include "extension.h"
#include "grammar.h"
#include "theme.h"

#include <pthread.h>
#include <string>
#include <vector>
#include <functional>

#define HIGHLIGHT_REQUEST_SIZE 512

typedef std::function<bool(int)> highlight_callback_t;

struct editor_t;
struct highlighter_t {
    language_info_ptr lang;
    theme_ptr theme;

    highlighter_t();

    void gatherBrackets(block_ptr block, char* first, char* last);
    void clearRequests();
    void requestHighlightBlock(block_ptr block, bool priority = false);
    int highlightBlocks(block_ptr block, int count = 1);
    int highlightBlock(block_ptr block);
    void run(editor_t* editor);
    void cancel();
    void pause();
    void resume();
    
    block_ptr highlightRequests[HIGHLIGHT_REQUEST_SIZE];
    size_t requestIdx;
    size_t processIdx;

    editor_t* editor;
    pthread_t threadId;

    highlight_callback_t callback;

    bool _paused;
};

span_info_t spanAtBlock(struct blockdata_t* blockData, int pos, bool rendered = false);

#endif // HIGHLIGHTER_H
