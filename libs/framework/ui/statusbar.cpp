#include "statusbar.h"
#include "renderer.h"
#include "text.h"

statusbar_t::statusbar_t()
    : horizontal_container_t()
{
    layout()->margin = 4;
    layout()->height = font()->height + layout()->margin * 2;

    left = std::make_shared<horizontal_container_t>();
    left->layout()->justify = LAYOUT_JUSTIFY_FLEX_START;
    right = std::make_shared<horizontal_container_t>();
    right->layout()->justify = LAYOUT_JUSTIFY_FLEX_END;

    add_child(left);
    add_child(right);
}

view_ptr statusbar_t::add_status(std::string text, int pos, int order)
{
    view_ptr p = std::make_shared<text_t>(text);
    p->layout()->order = order;
    if (p == 0) {
        left->add_child(p);
    } else {
        right->add_child(p);
    }
    return p;
}


void statusbar_t::remove_status(view_ptr view)
{
    left->remove_child(view);
    right->remove_child(view);
}