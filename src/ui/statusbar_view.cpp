#include "statusbar_view.h"
#include "renderer.h"

#include "statusbar.h"

statusbar_view::statusbar_view()
    : horizontal_container()
{
    layout()->height = 24;

    status = std::make_shared<text_view>("..");
    items = std::make_shared<horizontal_container>();
    items->layout()->fit_children = true;

    add_child(status);
    add_child(items);
}

void statusbar_view::update()
{
    if (statusbar_t::instance()) {
        ((text_view*)(status.get()))->text = statusbar_t::instance()->status;
        status->prelayout();
        status->layout()->rect.w = status->layout()->width;
    }
}