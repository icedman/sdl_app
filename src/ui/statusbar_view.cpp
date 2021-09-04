#include "statusbar_view.h"
#include "renderer.h"

#include "statusbar.h"

statusbar_view::statusbar_view()
    : horizontal_container()
{
    layout()->height = 24;
    layout()->fit_children = false;
    layout()->justify = LAYOUT_JUSTIFY_FLEX_END;

    status = std::make_shared<text_view>("status");
    
    add_child(status);
    add_child(std::make_shared<view_item>("spacer"));

    for(int i=0;i<5;i++) {
        std::string t = "# ";
        t += '1' + i;
        view_item_ptr item = std::make_shared<text_view>(t);
        add_child(item);
    }
}

void statusbar_view::update()
{
    if (statusbar_t::instance()) {
        // ((text_view*)(status.get()))->text = statusbar_t::instance()->status;
        // status->prelayout();
        // status->layout()->rect.w = status->layout()->width;
    }
}