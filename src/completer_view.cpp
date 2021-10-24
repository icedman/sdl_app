#include "completer_view.h"
#include "editor.h"
#include "editor_view.h"
#include "indexer.h"
#include "input_text.h"
#include "list.h"
#include "popup.h"
#include "search.h"
#include "system.h"

completer_t::completer_t(editor_view_t* e)
    : popup_t()
    , editor(e)
{
    can_focus = true;

    list = std::make_shared<list_t>();
    list->cast<list_t>()->tail_pad = 0;

    content()->add_child(list);

    on(EVT_KEY_SEQUENCE, [this](event_t& event) {
        event.cancelled = true;
        this->handle_key_sequence(event);
        return true;
    });
    on(EVT_KEY_TEXT, [this](event_t& event) {
        event.cancelled = true;
        this->handle_key_text(event);
        return true;
    });
    on(EVT_ITEM_SELECT, [this](event_t& event) {
        event.cancelled = true;
        this->handle_item_select(event);
        return true;
    });
}

bool completer_t::update_data()
{
    editor_ptr editor = this->editor->editor;
    if (!editor || editor->singleLineEdit) {
        return false;
    }

    editor_view_t* ev = this->editor;
    layout_item_ptr elo = ev->layout();

    struct document_t* doc = &editor->document;
    if (doc->cursors.size() > 1) {
        return false;
    }

    std::string prefix;

    cursor_t cursor = doc->cursor();
    cursor_t _cursor = cursor;
    struct block_t& block = *cursor.block();
    if (cursor.position() < 3)
        return false;

    if (cursor.moveLeft(1)) {
        cursor.selectWord();
        prefix = cursor.selectedText();

        selected_word = prefix;
        printf(">%s\n", selected_word.c_str());

        cursor.cursor = cursor.anchor;
        current_cursor = cursor;

        if (prefix.length() != _cursor.position() - cursor.position()) {
            return false;
        }
    }

    if (prefix.length() < 2) {
        return false;
    }

    std::string _prefix = prefix;
    if (prefix.length() > 3) {
        _prefix.pop_back();
    }
    if (_prefix.length() > 3) {
        _prefix.pop_back();
    }

    completer_t* cm = this;
    list_t* list = cm->list->cast<list_t>();
    std::vector<list_item_data_t> data;

    int completerItemsWidth = 0;
    if (editor->indexer) {
        std::vector<std::string> words = editor->indexer->findWords(_prefix);
        for (auto w : words) {
            if (w.length() <= prefix.length()) {
                continue;
            }
            int score = levenshtein_distance((char*)prefix.c_str(), (char*)(w.c_str()));

            if (completerItemsWidth < w.length()) {
                completerItemsWidth = w.length();
            }

            list_item_data_t item = {
                value : w,
                text : w,
                score : score
            };
            data.push_back(item);
        }
    }

    if (data.size()) {
        std::sort(data.begin(), data.end(), list_item_data_t::compare_item);
        list->update_data(data);
        list->select_item(-1);
        layout()->height = (data.size() * (font()->height + 4));
        layout()->width = (completerItemsWidth + 1) * (font()->width + 4);
        return true;
    } else {
        popup_manager_t::instance()->cast<popup_manager_t>()->clear();
    }

    list->cast<list_t>()->scroll_to_top();
    return false;
}

void completer_t::render(renderer_t* renderer)
{
    layout_item_ptr lo = layout();
    color_t clr = color_darker(system_t::instance()->renderer.background, 10);
    renderer->draw_rect(lo->render_rect, clr, true, 0);
}

bool completer_t::handle_key_sequence(event_t& event)
{
    operation_e op = operationFromKeys(event.text);
    switch (op) {
    case MOVE_CURSOR_UP:
        list->cast<list_t>()->select_previous();
        return true;
    case MOVE_CURSOR_DOWN:
        list->cast<list_t>()->select_next();
        return true;
    case ENTER:
        list->cast<list_t>()->select_item();
        return true;
    }

    editor->handle_key_sequence(event);
    popup_manager_t::instance()->cast<popup_manager_t>()->clear();
    return true;
}

bool completer_t::handle_key_text(event_t& event)
{
    editor->handle_key_text(event);
    return true;
}

bool completer_t::handle_item_select(event_t& event)
{
    editor_ptr editor = this->editor->editor;

    list_item_t* item = (list_item_t*)event.source;
    list_item_data_t d = item->item_data;
    std::string text = d.value;
    cursor_t cur = editor->document.cursor();
    std::ostringstream ss;
    ss << (current_cursor.block()->lineNumber + 1);
    ss << ":";
    ss << current_cursor.position();
    editor->pushOp(MOVE_CURSOR, ss.str());
    ss.str("");
    ss.clear();
    ss << (cur.block()->lineNumber + 1);
    ss << ":";
    ss << (cur.position() - 1);
    editor->pushOp(MOVE_CURSOR_ANCHORED, ss.str());
    editor->pushOp(INSERT, text);
    editor->runAllOps();

    this->editor->update_blocks();
    this->editor->relayout_virtual_blocks();

    popup_manager_t* pm = popup_manager_t::instance()->cast<popup_manager_t>();
    pm->clear();

    return true;
}
