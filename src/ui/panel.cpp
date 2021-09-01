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
}

view_item_ptr panel_view::content()
{
    return ((scrollarea_view*)scrollarea.get())->content;
}

bool panel_view::on_scroll()
{
    // printf("%d\n", (int)rand());
    rencache_invalidate();
    return true;
}

void panel_view::update()
{
    ((scrollbar_view*)v_scroll.get())->set_size(100, 10);
    ((scrollbar_view*)h_scroll.get())->set_size(100, 10);
}
