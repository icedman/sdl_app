#include "search_view.h"
#include "editor_view.h"
#include "editor.h"
#include "indexer.h"
#include "input_text.h"
#include "popup.h"
#include "search.h"
#include "system.h"

#include <algorithm>

static inline bool _compare_prefix(std::string s1, std::string s2, int len)
{
    if (s1.length() < len || s2.length() < len) {
        return true;
    }

    std::string _s1 = s1.substr(0, len);
    std::string _s2 = s2.substr(0, len);

    if (_s1 == _s2) {
        return true;
    }

    // tolower
    std::transform(_s1.begin(), _s1.end(), _s1.begin(),
        [](unsigned char c) { return std::tolower(c); });
    std::transform(_s2.begin(), _s2.end(), _s2.begin(),
        [](unsigned char c) { return std::tolower(c); });

    return _s1 == _s2;
}

search_view_t::search_view_t(editor_view_t* e)
    : popup_t()
    , editor(e)
{
    can_focus = true;

    _searchDirection = 0;

    layout()->margin_top = 4;
    layout()->width = 400;

    view_ptr vc = std::make_shared<vertical_container_t>();

    input = std::make_shared<input_text_t>();

    vc->add_child(input);
    content()->add_child(vc);

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

    // retain focus - always
    input->on(EVT_FOCUS_IN, [this](event_t& event) {
        view_t::set_focused(this);
        return true;
    });
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

void search_view_t::render(renderer_t* renderer)
{
    layout_item_ptr lo = layout();
    color_t clr = color_darker(system_t::instance()->renderer.background, 20);
    renderer->draw_rect(lo->render_rect, clr, true, 0);
}

bool search_view_t::handle_key_sequence(event_t& event)
{
    operation_e op = operationFromKeys(event.text);
    switch (op) {
    case ENTER:
		commit();
        return true;
    }

    input->cast<input_text_t>()->handle_key_sequence(event);
    return true;
}

bool search_view_t::handle_key_text(event_t& event)
{
    input->cast<input_text_t>()->handle_key_text(event);
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