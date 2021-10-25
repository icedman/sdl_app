#include "search_view.h"
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
		// commit        
        return true;
    }

    input->cast<input_text_t>()->handle_key_sequence(event);
    // popup_manager_t::instance()->cast<popup_manager_t>()->clear();
    return true;
}

bool search_view_t::handle_key_text(event_t& event)
{
    input->cast<input_text_t>()->handle_key_text(event);
    return true;
}
