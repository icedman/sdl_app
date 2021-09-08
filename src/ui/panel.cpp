#include "panel.h"
#include "render_cache.h"
#include "scrollarea.h"
#include "scrollbar.h"

panel_view::panel_view()
{
    type = "panel";
    int scrollbar_size = 12;

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

    bottom = container;

    _hscrollbar_container = container;

    v_scroll->on(EVT_SCROLLBAR_MOVE, [this](event_t& evt) {
        evt.cancelled = true;
        return this->scrollbar_move();
    });
    h_scroll->on(EVT_SCROLLBAR_MOVE, [this](event_t& evt) {
        evt.cancelled = true;
        return this->scrollbar_move();
    });

    content()->layout()->wrap = false;
    content()->layout()->fit_children = true;
    
    on(EVT_MOUSE_WHEEL, [this](event_t& evt) {
        evt.cancelled = true;
        return this->mouse_wheel(evt.x, evt.y);
    });
}

view_item_ptr panel_view::content()
{
    // return ((scrollarea_view*)scrollarea.get())->content;
    return (view_item::cast<scrollarea_view>(scrollarea))->content;
}

void panel_view::_validate()
{
    scrollarea_view* area = view_item::cast<scrollarea_view>(scrollarea);
    scrollbar_view* vs = view_item::cast<scrollbar_view>(v_scroll);
    scrollbar_view* hs = view_item::cast<scrollbar_view>(h_scroll);

    int hp = ((hs->count + (area->overscroll * hs->count) - hs->window));
    if (area->layout()->scroll_x < -hp) {
        area->layout()->scroll_x = -hp;
    }
    int vp = ((vs->count + (area->overscroll * vs->count) - vs->window));
    if (area->layout()->scroll_y < -vp) {
        area->layout()->scroll_y = -vp;
    }

    if (area->layout()->scroll_x > 0) {
        area->layout()->scroll_x = 0;
    }
    if (area->layout()->scroll_y > 0) {
        area->layout()->scroll_y = 0;
    }
}

bool panel_view::scrollbar_move()
{
    scrollarea_view* area = view_item::cast<scrollarea_view>(scrollarea);
    scrollbar_view* vs = view_item::cast<scrollbar_view>(v_scroll);
    scrollbar_view* hs = view_item::cast<scrollbar_view>(h_scroll);

    if (vs->window < vs->count) {
        int vp = ((vs->count + (area->overscroll * vs->count) - vs->window) * vs->index / vs->count);
        area->layout()->scroll_y = -vp;
    }
    if (hs->window < hs->count) {
        int hp = ((hs->count + (area->overscroll * hs->count) - hs->window) * hs->index / hs->count);
        area->layout()->scroll_x = -hs->index + hs->window;
    }

    _validate();

    // printf("%d\n", (int)rand());
    rencache_invalidate();
    return true;
}

bool panel_view::mouse_wheel(int x, int y)
{
    scrollarea_view* area = view_item::cast<scrollarea_view>(scrollarea);
    scrollbar_view* vs = view_item::cast<scrollbar_view>(v_scroll);
    scrollbar_view* hs = view_item::cast<scrollbar_view>(h_scroll);

    area->layout()->scroll_x += x * area->move_factor_x;
    area->layout()->scroll_y += y * area->move_factor_y;

    _validate();

    int hi = (-area->layout()->scroll_x * hs->count) / (hs->count + (area->overscroll * hs->count) - hs->window);
    if (hi < 0)
        hi = 0;
    hs->set_index(hi);

    int vi = (-area->layout()->scroll_y * vs->count) / (vs->count + (area->overscroll * vs->count) - vs->window);
    if (vi < 0)
        vi = 0;
    vs->set_index(vi);

    return true;
}

void panel_view::update()
{
    scrollbar_view* vs = view_item::cast<scrollbar_view>(v_scroll);
    scrollbar_view* hs = view_item::cast<scrollbar_view>(h_scroll);

    // vs->layout()->visible = vs->disabled ? false : content()->layout()->rect.h < scrollarea->layout()->rect.h;
    // hs->layout()->visible = hs->disabled ? false : content()->layout()->rect.w < scrollarea->layout()->rect.w;
    // bottom->layout()->visible = hs->layout()->visible;
    
    view_item::update();
}

void panel_view::postlayout()
{
    ((scrollbar_view*)v_scroll.get())->set_size(content()->layout()->rect.h, scrollarea->layout()->rect.h);
    ((scrollbar_view*)h_scroll.get())->set_size(content()->layout()->rect.w, scrollarea->layout()->rect.w);
}

void panel_view::update_scrollbars()
{
    mouse_wheel(0, 0);
}