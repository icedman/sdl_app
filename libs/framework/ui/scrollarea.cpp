#include "scrollarea.h"
#include "damage.h"
#include "renderer.h"

#define SCROLL_FACTOR 16

scrollarea_t::scrollarea_t()
    : view_t()
    , scroll_factor_x(SCROLL_FACTOR)
    , scroll_factor_y(SCROLL_FACTOR)
{
    _content = std::make_shared<view_t>();
    _content->layout()->fit_children_x = true;
    _content->layout()->fit_children_y = true;
    _content->layout()->wrap = true;

    add_child(_content);

    // TODO: event is consumed before panel
    // on(EVT_MOUSE_WHEEL, [this](event_t& evt) {
    //     if (evt.cancelled) {
    //         return true;
    //     }
    //     evt.cancelled = true;
    //     return this->handle_mouse_wheel(evt);
    // });
}

bool scrollarea_t::handle_mouse_wheel(event_t& event)
{
    layout()->scroll_x += event.sx * scroll_factor_x;
    layout()->scroll_y += event.sy * scroll_factor_y;
    layout_compute_absolute_position(layout());
    return true;
}

void scrollarea_t::render(renderer_t* renderer)
{
    // render_frame(renderer);
}