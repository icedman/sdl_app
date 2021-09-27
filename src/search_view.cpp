#include "search_view.h"
#include "renderer.h"
#include "list.h"
#include "inputtext.h"

#include "app.h"
#include "search.h"
#include "indexer.h"

#include <algorithm>

struct custom_editor_view_t : editor_view {
    custom_editor_view_t() : editor_view()
    {}

    bool input_text(std::string text)
    {
        editor_view::input_text(text);
        search->update_list();
    }

    bool input_sequence(std::string text)
    {
        operation_e op = operationFromKeys(text);

        list_view* lv = view_item::cast<list_view>(list);

        switch (op) {
        case MOVE_CURSOR_UP:
        case MOVE_CURSOR_DOWN:

            if (op == MOVE_CURSOR_UP) {
                lv->focus_previous();
            } else {
                lv->focus_next();
            }
            lv->ensure_visible_cursor();
            if (lv->focused_value != "") {
                view_item::cast<inputtext_view>(input)->set_value(lv->focused_value.c_str());
                editor->pushOp(SELECT_ALL, "");
                editor->runAllOps();
            }

            return true;
        case ENTER:
            search->commit();
            return true;
        break;
        }

        if (lv->data.size()) {
            lv->clear();
            layout_request();
            Renderer::instance()->throttle_up_events();
        }

        return editor_view::input_sequence(text);
    }

    search_view *search;
    view_item_ptr input;
    view_item_ptr list;
};

search_view::search_view()
    : popup_view()
{
    searchDirection = 0;

    class_name = "completer";
    list = std::make_shared<list_view>();
    input = std::make_shared<inputtext_view>();

    view_item_ptr _editor = std::make_shared<custom_editor_view_t>();
    custom_editor_view_t *ev = view_item::cast<custom_editor_view_t>(_editor);
    ev->search = this;
    ev->input = input;
    ev->list = list;
    view_item::cast<inputtext_view>(input)->set_editor(_editor);

    view_item_ptr container = std::make_shared<horizontal_container>();
    container->class_name = "editor";
    if (Renderer::instance()->is_terminal()) {
        container->layout()->height = 1;
    } else {
        container->layout()->margin = 4;
        container->layout()->height = 28;
    }
    container->add_child(input);

    content()->add_child(container);
    content()->add_child(list);
    interactive = true;
}

void search_view::show_search(std::string value)
{
    list_view* lv = view_item::cast<list_view>(list);
    lv->clear();
    lv->layout()->visible = false;

    layout_request();
    Renderer::instance()->throttle_up_events();

    view_set_focused(view_item::cast<inputtext_view>(input)->editor.get());
}

void search_view::update_list()
{
    list_view* lv = view_item::cast<list_view>(list);
    
    struct app_t* app = app_t::instance();

    struct editor_t* editor = app->currentEditor.get();

    std::string inputtext = view_item::cast<inputtext_view>(input)->value();

    // indexer
    int prev_size = lv->data.size();
    lv->data.clear();
    lv->value = "";
    if (editor->indexer) {
        std::vector<std::string> words = editor->indexer->findWords(inputtext);
        for(auto w : words) {
            int score = levenshtein_distance((char*)inputtext.c_str(), (char*)(w.c_str()));
            list_item_data_t item = {
                text : w,
                value : w,
                score: score
            };
            lv->data.push_back(item);
        }

        if (lv->data.size()) {
            sort(lv->data.begin(), lv->data.end(), list_view::compare_item);
        }
    }

    if (lv->data.size() != prev_size) {
        layout_request();
        Renderer::instance()->throttle_up_events();
    }
}

bool search_view::commit()
{
    list_view* lv = view_item::cast<list_view>(list);
    
    struct app_t* app = app_t::instance();

    struct editor_t* editor = app->currentEditor.get();
    struct document_t* doc = &editor->document;
    struct cursor_t cursor = doc->cursor();
    struct block_t& block = *cursor.block();

    std::string inputtext = view_item::cast<inputtext_view>(input)->value();

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
        ((editor_view*)editor->view)->ensure_visible_cursor();
        return true;
    }

    // searchHistory.push_back(text);
    // historyIndex = 0;

    if (inputtext.length() < 3) {
        _findNext = false;
        return false;
    }

    struct cursor_t cur = cursor;
    if (!_findNext) {
        cur.moveLeft(inputtext.length());
    }
    _findNext = false;

    bool found = cur.findWord(inputtext, searchDirection);
    if (!found) {
        if (searchDirection == 0) {
            cur.moveStartOfDocument();
        } else {
            cur.moveEndOfDocument();
        }
        found = cur.findWord(inputtext, searchDirection);
    }

    if (found) {
        app_t::log("found %s %d\n", inputtext.c_str(), cur.block()->lineNumber + 1);
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
        ((editor_view*)editor->view)->ensure_visible_cursor();
        _findNext = true;
        return true;
    }

    return false;
}

void search_view::prelayout()
{
    list_view* lv = view_item::cast<list_view>(list);
       
    int fw, fh;
    Renderer::instance()->get_font_extents(Renderer::instance()->font((char*)style.font.c_str()), &fw, &fh, NULL, 1);

    int list_size = lv->data.size();    
    if (list_size > 10) {
        list_size = 10;
    }

    int list_height = list_size * (Renderer::instance()->is_terminal() ? 1 : 24);
    int input_height = Renderer::instance()->is_terminal() ? 1 : 28;
    int search_width = 30;
    layout()->width = search_width * fw;
    layout()->height = input_height + list_height;

    lv->layout()->visible = list_size > 0;
}