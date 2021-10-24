#include "commands_view.h"
#include "editor.h"
#include "explorer.h"
#include "indexer.h"
#include "input_text.h"
#include "list.h"
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

commands_t::commands_t()
    : popup_t()
{
    can_focus = true;

    view_ptr vc = std::make_shared<vertical_container_t>();

    input = std::make_shared<input_text_t>();
    list = std::make_shared<list_t>();
    list->cast<list_t>()->tail_pad = 0;

    vc->add_child(input);
    vc->add_child(list);
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
    on(EVT_ITEM_SELECT, [this](event_t& event) {
        event.cancelled = true;
        this->handle_item_select(event);
        return true;
    });

    // retain focus - always
    input->on(EVT_FOCUS_IN, [this](event_t& event) {
        view_t::set_focused(this);
        return true;
    });
}

bool commands_t::update_data()
{
    input->cast<input_text_t>()->draw_cursors = true;
    layout()->width = 400;
    return true;
}

void commands_t::render(renderer_t* renderer)
{
    layout_item_ptr lo = layout();
    renderer->draw_rect(lo->render_rect, { 50, 50, 50 }, true);
}

bool commands_t::handle_key_sequence(event_t& event)
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

    input->cast<input_text_t>()->handle_key_sequence(event);
    // popup_manager_t::instance()->cast<popup_manager_t>()->clear();
    return true;
}

bool commands_t::handle_key_text(event_t& event)
{
    input->cast<input_text_t>()->handle_key_text(event);
    update_data();
    return true;
}

bool commands_t::handle_item_select(event_t& event)
{
    return true;
}

bool commands_files_t::update_data()
{
    commands_t::update_data();

    view_ptr popups = popup_manager_t::instance();

    std::vector<list_item_data_t> data;

    explorer_t* explorer = explorer_t::instance();

    list_t* list = this->list->cast<list_t>();

    std::string inputtext = input->cast<input_text_t>()->value();
    if (inputtext.length()) {
        for (auto f : explorer->fileList()) {
            if (f->isDirectory)
                continue;
            if (f->name.length() <= inputtext.length())
                continue;

            if (!_compare_prefix(f->name, inputtext, 3))
                continue;

            int score = levenshtein_distance((char*)inputtext.c_str(), (char*)(f->name.c_str()));
            list_item_data_t item = {
                value : f->fullPath,
                text : f->name,
                data : (void*)f,
                score : score
            };

            data.push_back(item);
        }
    }

    if (data.size()) {
        std::sort(data.begin(), data.end(), list_item_data_t::compare_item);
    }

    list->update_data(data);
    list->select_item(-1);

    // todo... initially popups is unparented
    int max_height = parent ? parent->layout()->render_rect.h / 3 : system_t::instance()->renderer.height() / 3;
    layout()->height = data.size() * (popups->font()->height + 4);
    layout()->height += popups->font()->height + 16;
    if (layout()->height > max_height) {
        layout()->height = max_height;
    }

    if (parent) {
        layout_clear_hash(parent->layout(), 4);
        parent->relayout();
    }

    list->cast<list_t>()->scroll_to_top();
    return true;
}

bool commands_files_t::handle_item_select(event_t& event)
{
    list_item_t* item = (list_item_t*)event.source;
    list_item_data_t d = item->item_data;
    popup_manager_t* pm = popup_manager_t::instance()->cast<popup_manager_t>();
    pm->clear();
    return true;
}