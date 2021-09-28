#include "tabbar.h"
#include "renderer.h"

#include "button.h"
#include "image.h"
#include "scrollbar.h"

#include "style.h"

tabbar_view::tabbar_view()
    : list_view()
{
    // layout()->margin_top = 8;
    layout()->height = 24;
    layout()->width = 0;
    content()->layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;

    if (Renderer::instance()->is_terminal()) {
        layout()->margin_top = 0;
        layout()->height = 1;
    }

    scrollbar_view* vs = view_item::cast<scrollbar_view>(v_scroll);
    scrollbar_view* hs = view_item::cast<scrollbar_view>(h_scroll);
    vs->disabled = true;
    hs->disabled = true;

    remove_child(bottom);
}
