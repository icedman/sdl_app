#include "statusbar_view.h"
#include "renderer.h"

#include "statusbar.h"

statusbar_view::statusbar_view()
    : horizontal_container()
{
    layout()->grow = 3;
    layout()->height = 24;
    layout()->fit_children = false;
    layout()->justify = LAYOUT_JUSTIFY_SPACE_AROUND;

    status = std::make_shared<text_view>("status");
    items = std::make_shared<horizontal_container>();
    layout()->grow = 2;
    
    add_child(status);
    add_child(items);

    for(int i=0;i<5;i++) {
        std::string t = "#";
        t += '1' + i;
        view_item_ptr item = std::make_shared<text_view>(t);
        items->add_child(item);
    }
}

void statusbar_view::update()
{
    statusbar_t *statusbar = statusbar_t::instance();
    if (statusbar) {
        text_view* text = view_item::cast<text_view>(status);
        text->text = statusbar->status;
        text->prelayout();
        text->layout()->rect.w = text->layout()->width;
        int i=0;
        for(auto v : items->_views) {
            text_view* text = view_item::cast<text_view>(v);
            text->text = statusbar->text[i++];
            text->prelayout();
            text->layout()->rect.w = text->layout()->width;
        }
    }

    view_item::update();
}