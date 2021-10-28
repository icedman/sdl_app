#include "search_view.h"
#include "editor.h"
#include "editor_view.h"
#include "indexer.h"
#include "input_text.h"
#include "popup.h"
#include "search.h"
#include "system.h"

#include <algorithm>

search_view_t::search_view_t(editor_view_t* e)
    : prompt_view_t(e)
{
    _searchDirection = 0;
}

bool search_view_t::update_data(std::string text)
{
    input->cast<input_text_t>()->set_value(text);
    if (text.length()) {
        editor_ptr e = input->cast<input_text_t>()->editor;
        e->pushOp(SELECT_ALL, "");
        e->runAllOps();
    }
    layout()->height = font()->height + 16;
    popup_manager_t::instance()->cast<popup_manager_t>()->clear();

    layout_clear_hash(input->layout(), 6);
    input->relayout();
    return true;
}

bool search_view_t::commit()
{
    editor_ptr editor = this->editor->editor;
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t& block = *cursor.block();

    std::string inputtext = input->cast<input_text_t>()->value();

    // goto line
    if (inputtext[0] == ':') {
        // std::string sInput = R"(AA #-0233 338982-FFB /ADR1 2)";
        // inputtext = std::regex_replace(inputtext, std::regex(R"([\D])"), "");
        if (inputtext.length()) {
            char lastChar = inputtext.back();
            if (lastChar < '0' || lastChar > '9') {
                inputtext.pop_back();
            }
        } else {
            return false;
        }

        inputtext = (inputtext.c_str() + 1);
        std::ostringstream ss;
        ss << inputtext;
        ss << ":";
        ss << 0;
        editor->pushOp(MOVE_CURSOR, ss.str());
        editor->runAllOps();

    } else {

        if (inputtext.length() < 3) {
            _findNext = false;
            return false;
        }

        struct cursor_t cur = cursor;
        if (!_findNext) {
            cur.moveLeft(inputtext.length());
        }
        _findNext = false;

        bool found = cur.findWord(inputtext, _searchDirection);
        if (!found) {
            if (_searchDirection == 0) {
                cur.moveStartOfDocument();
            } else {
                cur.moveEndOfDocument();
            }
            found = cur.findWord(inputtext, _searchDirection);
        }

        if (found) {
            // printf("found %s %d\n", inputtext.c_str(), cur.block()->lineNumber + 1);
            std::ostringstream ss;
            ss << (cur.anchorBlock()->lineNumber + 1);
            ss << ":";
            ss << cur.anchorPosition();
            editor->pushOp(MOVE_CURSOR, ss.str());
            ss.str("");
            ss.clear();
            ss << (cur.block()->lineNumber + 1);
            ss << ":";
            ss << cur.position();
            editor->pushOp(MOVE_CURSOR_ANCHORED, ss.str());
            editor->runAllOps();
            _findNext = true;
        }
    }

    this->editor->update_blocks();
    this->editor->ensure_visible_cursor();
    this->editor->relayout_virtual_blocks();
    return false;
}