#include "panel.h"
#include "scrollarea.h"
#include "scrollbar.h"
#include "render_cache.h"

panel_view::panel_view()
{
    int scrollbar_size = 18;
    
    v_scroll = std::make_shared<vscrollbar_view>();
    v_scroll->layout()->width = scrollbar_size;
    h_scroll = std::make_shared<hscrollbar_view>();
    h_scroll->layout()->height = scrollbar_size;

    scrollarea = std::make_shared<scrollarea_view>();

    view_item_ptr container = std::make_shared<horizontal_container>();
    container->add_child(scrollarea);
    container->add_child(v_scroll);
    add_child(container);

    resizer = std::make_shared<view_item>();
    resizer->layout()->width = scrollbar_size;

    container = std::make_shared<horizontal_container>();
    container->layout()->height = scrollbar_size;
    container->add_child(h_scroll);
    container->add_child(resizer);
    add_child(container);

    _hscrollbar_container = container;

    v_scroll->on(EVT_SCROLLBAR_MOVE, [this](event_t& evt) {
        evt.cancelled = true;
        return this->scrollbar_move();
    });
    h_scroll->on(EVT_SCROLLBAR_MOVE, [this](event_t& evt) {
        evt.cancelled = true;
        return this->scrollbar_move();
    });
    on(EVT_MOUSE_WHEEL, [this](event_t& evt) {
        evt.cancelled = true;
        return this->mouse_wheel(evt.x, evt.y);
    });
}

view_item_ptr panel_view::content()
{
    return ((scrollarea_view*)scrollarea.get())->content;
}

void panel_view::_validate()
{
    scrollarea_view *scroll = (scrollarea_view*)(scrollarea.get());
    
    if (scroll->layout()->scroll_x > 0) {
        scroll->layout()->scroll_x = 0;
        layout_request();
    }
    if (scroll->layout()->scroll_y > 0) {
        scroll->layout()->scroll_y = 0;
    }
}

bool panel_view::scrollbar_move()
{
    scrollarea_view *scroll = (scrollarea_view*)(scrollarea.get());

    // ((scrollbar_view*)h_scroll.get())->index + ((scrollbar_view*)h_scroll.get())->window;
    // ((scrollbar_view*)v_scroll.get())->index + ((scrollbar_view*)v_scroll.get())->window;

    scroll->layout()->scroll_x = -((scrollbar_view*)h_scroll.get())->index + ((scrollbar_view*)h_scroll.get())->window;
    scroll->layout()->scroll_y = -((scrollbar_view*)v_scroll.get())->index + ((scrollbar_view*)v_scroll.get())->window;
    _validate();

    // printf("%d\n", (int)rand());
    rencache_invalidate();
    return true;
}

bool panel_view::mouse_wheel(int x, int y)
{
    scrollarea_view *scroll = (scrollarea_view*)(scrollarea.get());
    
    scroll->layout()->scroll_x += x * scroll->move_factor_x;
    scroll->layout()->scroll_y += y * scroll->move_factor_y;

    _validate();

    ((scrollbar_view*)h_scroll.get())->set_index(-scroll->layout()->scroll_x);
    ((scrollbar_view*)v_scroll.get())->set_index(-scroll->layout()->scroll_y);

    return false;
}

void panel_view::update()
{
    // ((scrollbar_view*)v_scroll.get())->set_size(100, 10);
    // ((scrollbar_view*)h_scroll.get())->set_size(100, 10);
}
