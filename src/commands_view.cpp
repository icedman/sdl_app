#include "commands_view.h"
#include "input_text.h"
#include "list.h"
#include "editor.h"
#include "indexer.h"
#include "search.h"
#include "popup.h"

commands_t::commands_t()
    : popup_t()
{
    can_focus = true;

    input = std::make_shared<input_text_t>();
    list = std::make_shared<list_t>();
    list->cast<list_t>()->tail_pad = 0;

    content()->add_child(input);
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

bool commands_t::update_data()
{
    return false;
}

void commands_t::render(renderer_t *renderer)
{
    layout_item_ptr lo = layout();

    renderer->draw_rect(lo->render_rect, { 50,50,50 }, true);
}

bool commands_t::handle_key_sequence(event_t& event)
{
    operation_e op = operationFromKeys(event.text);
    switch(op)
    {
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
    popup_manager_t::instance()->cast<popup_manager_t>()->clear();
    return true;
}

bool commands_t::handle_key_text(event_t& event)
{
    input->cast<input_text_t>()->handle_key_text(event);
    return true;
}

bool commands_t::handle_item_select(event_t& event)
{
    return true;
}

