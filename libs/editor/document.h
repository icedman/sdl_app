#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <string>
#include <vector>

#include "block.h"
#include "cursor.h"

struct editor_t;
struct document_t {
    document_t();
    ~document_t();

    editor_t* editor;
    block_list blocks;
    std::ifstream file;

    bool open(std::string path);
    void close();
    void save();
    void saveAs(const char* path, bool replacePath = false);
    void print();

    block_ptr blockAtLine(size_t line);
    block_ptr addBlockAtLine(size_t line);
    void removeBlockAtLine(size_t line, size_t count = 1);

    block_ptr firstBlock();
    block_ptr lastBlock();
    block_ptr nextBlock(block_t* block);
    block_ptr previousBlock(block_t* block);

    std::string filePath;
    std::string fileName;
    std::string fullPath;
    std::string tempFileFullPath;
    bool binary;

    cursor_t cursor();
    void addCursor(cursor_t cur);
    void setCursor(cursor_t cur, bool mainCursor = false);
    void updateCursors(cursor_list& curs);
    void clearCursors();
    void clearDuplicateCursors();

    bool hasSelections();
    void clearSelections();

    cursor_t findNextOccurence(cursor_t cur, std::string word);

    cursor_list cursors;

    size_t cursorId;
    size_t blockId;

    bool windowsLineEnd;

    void updateBlocks(block_list& blocks, size_t lineNumber = 0, size_t count = 0);

    bool lineNumberingIntegrity();
};

#endif // DOCUMENT_H
