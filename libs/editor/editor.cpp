#include "editor.h"
#include "indexer.h"
#include "utf8.h"
#include "util.h"

#include "app.h"
#include "backend.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include <unistd.h>

editor_t::editor_t()
    : view(0)
    , indexer(0)
    , singleLineEdit(false)
{
    document.editor = this;
    _scrollToCursor = true;
    _foldedLines = 0;

    highlighter.editor = this;
}

editor_t::~editor_t()
{
    if (indexer) {
        delete indexer;
    }
}

void editor_t::enableIndexer()
{
    indexer = new indexer_t();
    indexer->editor = this;
}

void editor_t::pushOp(std::string op, std::string params)
{
    pushOp(operationFromName(op), params);
}

void editor_t::pushOp(operation_e op, std::string params)
{
    operation_t operation = {
        .op = op,
        .params = params
    };
    pushOp(operation);
}

void editor_t::pushOp(operation_t op)
{
    operations.push_back(op);
}

void editor_t::runOp(operation_t op)
{
    int intParam = 0;
    try {
        intParam = std::stoi(op.params);
    } catch (std::exception e) {
    }

    std::string strParam = op.params;
    operation_e _op = op.op;

    // make sure somewhere block lineNumbers are accurate
    // document.updateBlocks(document.blocks);

    if (snapshots.size()) {
        snapshot_t& snapshot = snapshots.back();

        bool trouble = false;
        op.cursors = document.cursors;
        for (auto& c : op.cursors) {
            c.cursor.line = c.block()->lineNumber;
            c.anchor.line = c.anchorBlock()->lineNumber;
        }

        snapshot.history.push_back(op);
    }

    //-------------------
    // handle selections
    //-------------------
    if (document.hasSelections()) {
        snapshot_t& snapshot = snapshots.back();
        operation_list items = snapshot.history;
        switch (_op) {
        case TAB:
        case ENTER:
        case INSERT: {
            operation_t d = { .op = DELETE_SELECTION };
            runOp(d);
            snapshot.history = items;
            break;
        }
        case DEL:
        case BACKSPACE: {
            operation_t d = { .op = DELETE_SELECTION };
            runOp(d);
            snapshot.history = items;
            return;
        }
        default:
            break;
        }
    }

    cursor_list cursors = document.cursors;
    cursor_t mainCursor = document.cursor();
    cursor_util::sortCursors(cursors);

    if (mainCursor.block()) {
        // log("%s %d %d", nameFromOperation(op.op).c_str(), mainCursor.block()->lineNumber, mainCursor.position());
    }

    switch (_op) {

    case DEBUG_SCOPES: {
        blockdata_ptr data = mainCursor.block()->data;
        log("-------------\n%s", mainCursor.block()->text().c_str());
        for (auto s : data->spans) {
            log("scope: %s", s.scope.c_str());
        }
        struct span_info_t span = spanAtBlock(data.get(), mainCursor.position());
        log("scope: %s", span.scope.c_str());
        return;
    }

    case CANCEL:
        operations.clear();
        return;
    case OPEN:
        document.open(strParam);
        createSnapshot();
        return;
    case SAVE: {
        if (document.fileName == "") {
            // popup_t::instance()->prompt("filename");
            return;
        }
        log("saving %s", document.fileName.c_str());
        document.save();
        std::ostringstream ss;
        ss << "saved ";
        ss << document.fullPath;
        // statusbar_t::instance()->setStatus(ss.str(), 3500);
        return;
    }
    case SAVE_AS:
        return;
    case SAVE_COPY:
        return;
    case UNDO:
        return;
    case REDO:
        return;
    case CLOSE:
        return;

    case PASTE:
        if (!backend_t::instance()->getClipboard().length()) {
            return;
        }

        inputBuffer = backend_t::instance()->getClipboard();
        break;

    case CUT:
    case COPY: {
        if (mainCursor.hasSelection()) {
            backend_t::instance()->setClipboard(mainCursor.selectedText());
        }
        if (_op == COPY) {
            return;
        }
        _op = DELETE_SELECTION;
        if (!mainCursor.hasSelection()) {
            _op = DELETE_LINE;
        }
        break;
    }

    case ADD_CURSOR_AND_MOVE: {
        document.addCursor(mainCursor);

        std::vector<std::string> strings;
        std::istringstream f(strParam);
        std::string s;
        while (getline(f, s, ':')) {
            strings.push_back(s);
        }

        if (strings.size() == 2) {
            size_t line = 0;
            size_t pos = 0;
            try {
                line = std::stoi(strings[0]);
                pos = std::stoi(strings[1]);
                block_ptr block = document.blockAtLine(line);
                if (!block) {
                    block = document.lastBlock();
                }
                mainCursor.setPosition(block, pos, false);
            } catch (std::exception e) {
            }

            document.setCursor(mainCursor, true);
            document.clearDuplicateCursors();
        }
        return;
    }

    case ADD_CURSOR_AND_MOVE_UP: {
        document.addCursor(mainCursor);
        // _op = MOVE_CURSOR_UP;
        mainCursor.moveUp(1, false);
        document.setCursor(mainCursor, true);
        document.clearDuplicateCursors();
        return;
    }
    case ADD_CURSOR_AND_MOVE_DOWN: {
        document.addCursor(mainCursor);
        // _op = MOVE_CURSOR_DOWN;
        mainCursor.moveDown(1, false);
        document.setCursor(mainCursor, true);
        document.clearDuplicateCursors();
        return;
    }
    case ADD_CURSOR_FOR_SELECTED_WORD:
        if (mainCursor.hasSelection() && mainCursor.selectedText().length()) {
            cursor_t res = document.findNextOccurence(mainCursor, mainCursor.selectedText());
            if (!res.isNull()) {
                res.normalizeSelection(mainCursor.isSelectionNormalized());
                document.addCursor(mainCursor);
                document.setCursor(res, true); // replace main cursor
                log("found %s at %s", mainCursor.selectedText().c_str(), res.block()->text().c_str());
                return;
            }
        } else {
            _op = SELECT_WORD;
        }
        break;

    case CLEAR_LAST_CURSOR:
        if (document.cursors.size() > 1) {
            document.cursors[0] = document.cursors.back();
            document.cursors.pop_back();
        } else {
            mainCursor.clearSelection();
            document.setCursor(mainCursor, true);
        }
        break;

    case CLEAR_CURSORS:
        mainCursor.clearSelection();
        document.clearCursors();
        document.setCursor(mainCursor, true);
        break;

    case SELECT_ALL:
        document.clearCursors();
        mainCursor = document.cursor();
        mainCursor.moveStartOfDocument();
        mainCursor.moveEndOfDocument(true);
        document.setCursor(mainCursor);
        return;

    case TOGGLE_FOLD:
        toggleFold(mainCursor.block()->lineNumber);
        return;

    default:
        break;
    }

    for (auto& cur : cursors) {

        bool skipOp = false;
        if (singleLineEdit) {
            switch (_op) {
            case ENTER:
            case TAB:
            case INDENT:
            case UNINDENT:
            case DUPLICATE_LINE:
                skipOp = true;
                break;
            }
        }

        if (skipOp)
            continue;

        if (_op == DUPLICATE_SELECTION) {
            if (!cur.hasSelection()) {
                _op = DUPLICATE_LINE;
            }
        }

        switch (_op) {

        case SELECT_WORD:
            cur.selectWord();
            break;

        case TOGGLE_COMMENT: {
            int count = cur.toggleLineComment();
            bool m = cur.isMultiBlockSelection();
            cursor_t c = cur;
            c.cursor.position = 0;
            c.anchor.position = 0;
            cursor_t a = cur;
            a.cursor = a.anchor;
            a.cursor.position = 0;
            a.anchor.position = 0;
            cursor_util::advanceBlockCursors(cursors, c, count);
            if (m) {
                cursor_util::advanceBlockCursors(cursors, a, count);
            }
            break;
        }
        case INDENT: {
            int count = cur.indent();
            bool m = cur.isMultiBlockSelection();
            cursor_t c = cur;
            c.cursor.position = 0;
            c.anchor.position = 0;
            cursor_t a = cur;
            a.cursor = a.anchor;
            a.cursor.position = 0;
            a.anchor.position = 0;
            cursor_util::advanceBlockCursors(cursors, c, count);
            if (m) {
                cursor_util::advanceBlockCursors(cursors, a, count);
            }
        } break;
        case UNINDENT: {
            int count = cur.unindent();
            if (count > 0) {
                bool m = cur.isMultiBlockSelection();
                cursor_t c = cur;
                c.cursor.position = 0;
                c.anchor.position = 0;
                cursor_t a = cur;
                a.cursor = a.anchor;
                a.cursor.position = 0;
                a.anchor.position = 0;
                if (count > 1) {
                    cursor_util::advanceBlockCursors(cursors, c, -count + 1);
                    if (m) {
                        cursor_util::advanceBlockCursors(cursors, a, -count + 1);
                    }
                }
            }
        } break;
        case SELECT_LINE:
            cur.moveStartOfLine();
            cur.moveEndOfLine(true);
            break;
        case DUPLICATE_LINE: {
            cur.moveStartOfLine();
            cur.moveEndOfLine(true);
            std::string text = cur.selectedText();
            cur.splitLine();
            cur.moveDown(1);
            cur.moveStartOfLine();
            cur.insertText(text);
            break;
        }
        case DELETE_LINE:
            cur.moveStartOfLine();
            cur.eraseText(cur.block()->length());
            cur.mergeNextLine();
            break;
        case MOVE_LINE_UP:
            break;
        case MOVE_LINE_DOWN:
            break;

        case CLEAR_SELECTION:
            break;

        case DUPLICATE_SELECTION: {
            std::vector<std::string> text = cur.selectedTextArray();
            cur.moveEndOfSelection();
            cursor_t s = cur;
            bool first = true;
            for (auto t : text) {
                if (!first) {
                    cur.splitLine();
                    cur.moveDown(1);
                    cur.moveStartOfLine();
                }
                cur.insertText(t);
                int len = utf8_length(t);
                if (len) {
                    cur.moveRight(len);
                }
                first = false;
            }
            cursor_t e = cur;
            e.moveLeft(1);
            cur = s;
            cur.setPosition(e.cursor, true);
            break;
        }

        case DELETE_SELECTION:
            if (cur.hasSelection()) {
                cursor_position_t pos = cur.selectionStart();
                cursor_position_t end = cur.selectionEnd();
                bool snap = (end.block->lineNumber - pos.block->lineNumber > 100);
                cur.eraseSelection();
                cur.setPosition(pos);
                cur.clearSelection();
                int count = end.position - pos.position;
                if (count > 0 && end.block == pos.block) {
                    count++;
                    cursor_util::advanceBlockCursors(cursors, cur, -count);
                }
                if (snap) {
                    createSnapshot();
                }
            }
            break;

        case MOVE_CURSOR:
        case MOVE_CURSOR_ANCHORED: {
            std::vector<std::string> strings;
            std::istringstream f(strParam);
            std::string s;
            while (getline(f, s, ':')) {
                strings.push_back(s);
            }

            if (strings.size() == 2) {
                size_t line = 0;
                size_t pos = 0;
                try {
                    line = std::stoi(strings[0]);
                    pos = std::stoi(strings[1]);
                    block_ptr block = document.blockAtLine(line);
                    if (!block) {
                        block = document.lastBlock();
                    }
                    cur.setPosition(block, pos, _op == MOVE_CURSOR_ANCHORED);
                } catch (std::exception e) {
                }
            }
            //
        } break;

        case MOVE_CURSOR_LEFT:
        case MOVE_CURSOR_LEFT_ANCHORED:
            if (!cur.moveLeft(intParam, _op == MOVE_CURSOR_LEFT_ANCHORED)) {
                if (document.cursors.size() == 1 && _op != MOVE_CURSOR_LEFT_ANCHORED) {
                    document.clearCursors();
                }
            }
            break;
        case MOVE_CURSOR_RIGHT:
        case MOVE_CURSOR_RIGHT_ANCHORED: {
            if (!cur.moveRight(intParam, _op == MOVE_CURSOR_RIGHT_ANCHORED)) {
                if (document.cursors.size() == 1 && _op != MOVE_CURSOR_RIGHT_ANCHORED) {
                    document.clearCursors();
                }
            }
            break;
        }
        case MOVE_CURSOR_UP:
        case MOVE_CURSOR_UP_ANCHORED:
            cur.moveUp(intParam, _op == MOVE_CURSOR_UP_ANCHORED);
            break;
        case MOVE_CURSOR_DOWN:
        case MOVE_CURSOR_DOWN_ANCHORED:
            cur.moveDown(intParam, _op == MOVE_CURSOR_DOWN_ANCHORED);
            break;
        case MOVE_CURSOR_NEXT_WORD:
        case MOVE_CURSOR_NEXT_WORD_ANCHORED:
            cur.moveNextWord(_op == MOVE_CURSOR_NEXT_WORD_ANCHORED);
            break;
        case MOVE_CURSOR_PREVIOUS_WORD:
        case MOVE_CURSOR_PREVIOUS_WORD_ANCHORED:
            cur.movePreviousWord(_op == MOVE_CURSOR_PREVIOUS_WORD_ANCHORED);
            break;
        case MOVE_CURSOR_START_OF_LINE:
        case MOVE_CURSOR_START_OF_LINE_ANCHORED:
            cur.moveStartOfLine(_op == MOVE_CURSOR_START_OF_LINE_ANCHORED);
            break;
        case MOVE_CURSOR_END_OF_LINE:
        case MOVE_CURSOR_END_OF_LINE_ANCHORED:
            cur.moveEndOfLine(_op == MOVE_CURSOR_END_OF_LINE_ANCHORED);
            break;
        case MOVE_CURSOR_START_OF_DOCUMENT:
        case MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED:
            cur.moveStartOfDocument(_op == MOVE_CURSOR_START_OF_DOCUMENT_ANCHORED);
            break;
        case MOVE_CURSOR_END_OF_DOCUMENT:
        case MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED:
            cur.moveEndOfDocument(_op == MOVE_CURSOR_END_OF_DOCUMENT_ANCHORED);
            break;
        case MOVE_CURSOR_NEXT_PAGE:
        case MOVE_CURSOR_NEXT_PAGE_ANCHORED:
            // implement in editor_view
            // cur.moveNextBlock(document.blocks.size(), _op == MOVE_CURSOR_NEXT_PAGE_ANCHORED);
            break;
        case MOVE_CURSOR_PREVIOUS_PAGE:
        case MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED:
            // implement in editor_view
            // cur.movePreviousBlock(document.blocks.size(), _op == MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED);
            break;

        case TAB: {
            std::string tab = "";
            for (int i = 0; i < app_t::instance()->tabSize; i++) {
                tab += " ";
            }
            cur.insertText(tab);
            cursor_util::advanceBlockCursors(cursors, cur, tab.length());
            cur.moveRight(tab.length());
            break;
        }
        case ENTER:
            cur.splitLine();
            cur.moveDown(1);
            cur.moveStartOfLine();
            break;
        case DEL:
            if (cur.position() == cur.block()->length() - 1) {
                cur.mergeNextLine();
                break;
            }
            cur.eraseText(1);
            cursor_util::advanceBlockCursors(cursors, cur, -1);
            break;
        case BACKSPACE:
            if (cur.position() == 0) {
                if (cur.movePreviousBlock(1)) {
                    int newPos = cur.block()->length() - 1;
                    cur.mergeNextLine();
                    cur.setPosition(cur.block(), newPos, false);
                }
                break;
            }
            cur.moveLeft(1);
            cur.eraseText(1);
            cursor_util::advanceBlockCursors(cursors, cur, -1);
            break;
        case INSERT:
            cur.insertText(strParam);
            cursor_util::advanceBlockCursors(cursors, cur, utf8_length(strParam));
            cur.moveRight(utf8_length(strParam));
            break;

        default:
            break;
        }

        document.updateCursors(cursors);
    }

    document.clearDuplicateCursors();

    _scrollToCursor = true;
}

void editor_t::runAllOps()
{
    while (operations.size()) {
        operation_t op = operations.front();
        operations.erase(operations.begin());
        runOp(op);
    }

    // handle the inputBuffer (TODO handle UTF8)
    operation_t op;

    bool snap = inputBuffer.size() > 1000;

    // paste
    if (inputBuffer.size()) {

        // snapshot_t& snapshot = snapshots.back();
        // operation_list items = snapshot.history;

        char* _s = (char*)inputBuffer.c_str();
        char* _t = _s;
        char* p = _s;

        op.group = 0xff;

        while (*p) {

            if (singleLineEdit && *p == '\n') {
                break;
            }

            op.op = INSERT;
            op.params = "";

            switch (*p) {
            case '\n': {
                if (p - _t > 0) {
                    op.params = std::string(_t, p - _t);
                    runOp(op);
                    p++;
                    _t = p;
                }
                op.op = ENTER;
                runOp(op);

                if (*_t == '\n') {
                    op.op = ENTER;
                    runOp(op);
                    _t++;
                    p++;
                }

                break;
            }
            case '\t': {
                if (p - _t > 0) {
                    op.params = std::string(_t, p - _t);
                    runOp(op);
                    p++;
                    _t = p;
                }
                op.op = TAB;
                runOp(op);
                break;
            }

            default:
                break;
            }

            p++;
        }

        if (p - _t > 0) {
            op.op = INSERT;
            op.params = std::string(_t, p - _t);
            runOp(op);
        }
        inputBuffer = "";

        // fix undo for paste
        // snapshot.history = items;
    }

    if (snap) {
        createSnapshot();
    }
}

void editor_t::matchBracketsUnderCursor()
{
    cursor_t cursor = document.cursor();
    if (cursor.position() != cursorBracket1.position || cursor.block()->lineNumber != cursorBracket1.line) {
        cursorBracket1 = bracketAtCursor(cursor);
        if (cursorBracket1.bracket != -1) {
            cursor_t matchCursor = findBracketMatchCursor(cursorBracket1, cursor);
            cursorBracket2 = bracketAtCursor(matchCursor);
        }
    }
}

struct bracket_info_t editor_t::bracketAtCursor(struct cursor_t& cursor)
{
    bracket_info_t b;
    b.bracket = -1;

    block_ptr block = cursor.block();
    if (!block) {
        return b;
    }

    blockdata_ptr blockData = block->data;
    if (!blockData) {
        return b;
    }

    size_t p = cursor.position();
    size_t l = cursor.block()->lineNumber;
    for (auto bracket : blockData->brackets) {
        if (bracket.position == p && bracket.line == l) {
            return bracket;
        }
    }

    return b;
}

cursor_t editor_t::cursorAtBracket(struct bracket_info_t bracket)
{
    cursor_t cursor;

    block_ptr block = document.cursor().block();
    while (block) {
        if (block->lineNumber == bracket.line) {
            cursor = document.cursor();
            cursor.setPosition(block, bracket.position);
            break;
        }
        if (block->lineNumber > bracket.line) {
            block = block->next();
        } else {
            block = block->previous();
        }
    }
    return cursor;
}

cursor_t editor_t::findLastOpenBracketCursor(block_ptr block)
{
    if (!block->isValid()) {
        return cursor_t();
    }

    blockdata_ptr blockData = block->data;
    if (!blockData) {
        return cursor_t();
    }

    cursor_t res;
    for (auto b : blockData->foldingBrackets) {
        if (b.open) {
            if (res.isNull()) {
                res = document.cursor();
            }
            res.setPosition(block, b.position);
        }
    }

    return res;
}

cursor_t editor_t::findBracketMatchCursor(struct bracket_info_t bracket, cursor_t cursor)
{
    cursor_t cs = cursor;

    std::vector<bracket_info_t> brackets;
    block_ptr block = cursor.block();

    if (bracket.open) {

        while (block) {
            blockdata_ptr blockData = block->data;
            if (!blockData) {
                break;
            }

            for (auto b : blockData->brackets) {
                if (b.line == bracket.line && b.position < bracket.position) {
                    continue;
                }

                if (!b.open) {
                    auto l = brackets.back();
                    if (l.open && l.bracket == b.bracket) {
                        brackets.pop_back();
                    } else {
                        // error .. unpaired?
                        return cursor_t();
                    }

                    if (!brackets.size()) {
                        // std::cout << "found end!" << std::endl;
                        cursor.setPosition(block, b.position);
                        return cursor;
                    }
                    continue;
                }
                brackets.push_back(b);
            }

            block = block->next();
        }

    } else {

        // reverse
        while (block) {
            blockdata_ptr blockData = block->data;
            if (!blockData) {
                break;
            }

            // for(auto b : blockData->brackets) {
            for (auto it = blockData->brackets.rbegin(); it != blockData->brackets.rend(); ++it) {
                bracket_info_t b = *it;
                if (b.line == bracket.line && b.position > bracket.position) {
                    continue;
                }

                if (b.open) {
                    auto l = brackets.back();
                    if (!l.open && l.bracket == b.bracket) {
                        brackets.pop_back();
                    } else {
                        // error .. unpaired?
                        return cursor_t();
                    }

                    if (!brackets.size()) {
                        // std::cout << "found begin!" << std::endl;
                        cursor.setPosition(block, b.position);
                        return cursor;
                    }
                    continue;
                }
                brackets.push_back(b);
            }

            block = block->previous();
        }
    }
    return cursor_t();
}

void editor_t::toggleFold(size_t line)
{
    document_t* doc = &document;
    block_ptr folder = doc->blockAtLine(line + 1);

    // printf("%d %s\n", line, folder->text().c_str());

    cursor_t openBracket = findLastOpenBracketCursor(folder);
    if (openBracket.isNull()) {
        return;
    }

    bracket_info_t bracket = bracketAtCursor(openBracket);
    if (bracket.bracket == -1) {
        // printf("2> %d", folder->lineNumber);
        return;
    }
    cursor_t endBracket = findBracketMatchCursor(bracket, openBracket);
    if (endBracket.isNull()) {
        // printf("3> %d", folder->lineNumber);
        return;
    }

    block_ptr block = openBracket.block();
    block_ptr endBlock = endBracket.block();

    blockdata_ptr blockData = block->data;
    if (!blockData) {
        return;
    }

    blockData->folded = !blockData->folded;
    block = block->next();
    while (block) {
        blockdata_ptr targetData = block->data;
        if (!targetData)
            break;

        targetData->folded = blockData->folded;
        targetData->foldable = false;
        targetData->foldedBy = blockData->folded ? block->uid : 0;
        targetData->dirty = true;

        if (block == endBlock) {
            break;
        }
        block = block->next();
    }
}

void editor_t::undo()
{
    snapshot_t& snapshot = snapshots.back();
    operation_list items = snapshot.history;

    if (items.size() == 0)
        return;

    // for (auto op : items) {
    //     std::string on = nameFromOperation(op.op);
    //     printf("%s\n", on.c_str());
    // }

    while (items.size() > 0) {
        auto lastOp = items.back();
        items.pop_back();

        bool endPop = false;

        switch (lastOp.op) {
        case TAB:
        case ENTER:
        case DEL:
        case BACKSPACE:
        case INSERT:
        case CUT:
        case DUPLICATE_LINE:
        case DELETE_LINE:
        case INDENT:
        case UNINDENT:
            endPop = true;
        default:
            break;
        }

        if (lastOp.group == 0xff) {
            endPop = false;
        }

        if (endPop)
            break;
    }

    snapshot.restore(document.blocks);

    for (auto op : items) {

        switch (op.op) {
        case OPEN:
        case COPY:
        case PASTE:
        case SAVE:
        case UNDO:
            continue;
        default:
            break;
        }

        for (auto& c : op.cursors) {
            c.cursor.block = document.blockAtLine(c.cursor.line + 1);
            c.anchor.block = document.blockAtLine(c.anchor.line + 1);
        }

        document.cursors = op.cursors;

        pushOp(op);
        runAllOps();
    }

    document.updateBlocks(document.blocks);

    snapshot.history = items;
    if (snapshots.size() > 1 && items.size() == 0) {
        snapshots.pop_back();
    }
}

void editor_t::createSnapshot()
{
    snapshot_t snapshot;
    snapshot.save(document.blocks);
    snapshots.emplace_back(snapshot);
}

bool editor_t::input(char ch, std::string keySequence)
{
    operation_e op = operationFromKeys(keySequence);
    editor_t* editor = this;

    if (op == CANCEL) {
        editor->pushOp(CLEAR_CURSORS);
        return true;
    }
    if (op == UNDO) {
        editor->undo();
        return true;
    }
    if (op != UNKNOWN) {
        editor->pushOp(op);
        return true;
    }
    if (keySequence != "") {
        return true;
    }
    return true;
}

int editor_t::highlight(int startingLine, int count)
{
    struct cursor_t mainCursor = document.cursor();
    block_ptr mainBlock = mainCursor.block();

    cursor_list cursors = document.cursors;

    size_t cx;
    size_t cy;

    block_list::iterator it = document.blocks.begin();

    int idx = startingLine;
    if (idx >= document.blocks.size()) {
        return 0;
    }
    if (idx < 0) {
        idx = 0;
    }
    it += idx;

    int lighted = 0;
    int c = 0;
    while (it != document.blocks.end()) {
        block_ptr b = *it++;
        b->lineNumber = idx++;
        lighted += highlighter.highlightBlock(b);
        // log("%d %d %d", c, mainBlock->lineNumber, preceedingBlocks, count);
        if (c++ >= count) // mainBlock->lineNumber + preceedingBlocks + trailingBlocks + count)
            break;
    }

    // printf("><%d\n", lighted);
    return lighted;
}

void editor_t::toMarkup()
{
}
