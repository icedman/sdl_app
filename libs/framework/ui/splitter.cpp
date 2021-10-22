#include "splitter.h"
#include "renderer.h"
#include "system.h"

splitter_t::splitter_t(view_t* target, view_t* container)
    : view_t()
    , target(target)
    , container(container)
    , dragging(false)
{
    can_hover = true;

    layout()->width = 8;
    layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;

    on(EVT_MOUSE_DRAG_START, [this](event_t& evt) {
        evt.cancelled = true;
        return this->handle_mouse_drag_start(evt);
    });
    on(EVT_MOUSE_DRAG, [this](event_t& evt) {
        evt.cancelled = true;
        if (this->layout()->is_column()) {
            system_t::instance()->set_cursor(cursor_e::RESIZE_EW);
        } else {
            system_t::instance()->set_cursor(cursor_e::RESIZE_NS);
        }
        return this->handle_mouse_drag(evt);
    });
    on(EVT_MOUSE_DRAG_END, [this](event_t& evt) {
        evt.cancelled = true;
        system_t::instance()->set_cursor(cursor_e::ARROW);
        return this->handle_mouse_drag_end(evt);
    });
    on(EVT_HOVER_IN, [this](event_t& evt) {
        evt.cancelled = true;
        if (this->layout()->is_column()) {
            system_t::instance()->set_cursor(cursor_e::RESIZE_EW);
        } else {
            system_t::instance()->set_cursor(cursor_e::RESIZE_NS);
        }
        return true;
    });
    on(EVT_HOVER_OUT, [this](event_t& evt) {
        evt.cancelled = true;
        system_t::instance()->set_cursor(cursor_e::ARROW);
        return true;
    });
}

bool splitter_t::handle_mouse_drag_start(event_t& event)
{
    if (!target)
        return true;
    start_width = target->layout()->render_rect.w;
    start_height = target->layout()->render_rect.h;
    drag_start_x = event.x;
    drag_start_y = event.y;
    dragging = true;
    return true;
}

bool splitter_t::handle_mouse_drag_end(event_t& event)
{
    dragging = false;
    return true;
}

bool splitter_t::handle_mouse_drag(event_t& event)
{
    if (dragging) {
        if (layout()->is_column()) {
            bool prior = target->layout()->render_rect.x <= layout()->render_rect.x;
            target->layout()->width = start_width + (prior ? (event.x - drag_start_x) : -(event.x - drag_start_x));
        } else {
            bool prior = target->layout()->render_rect.y <= layout()->render_rect.y;
            target->layout()->height = start_height + (prior ? (event.y - drag_start_y) : -(event.y - drag_start_y));
        }
        system_t::instance()->caffeinate();
        layout_clear_hash(container->layout(), 6);
        container->relayout();
    }
    return true;
}

void splitter_t::render(renderer_t* renderer)
{
    // render_frame(renderer);
}

horizontal_splitter_t::horizontal_splitter_t(view_t* target, view_t* container)
    : splitter_t(target, container)
{
    layout()->height = layout()->width;
    layout()->width = 0;
    layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
}