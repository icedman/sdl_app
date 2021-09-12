#include "tabbar.h"
#include "renderer.h"
#include "render_cache.h"

#include "app.h"
#include "app_view.h"
#include "scrollbar.h"
#include "button.h"
#include "image.h"

#include "style.h"

tabbar_view::tabbar_view()
    : list_view()
{
    layout()->margin_top = 8;
    layout()->height = 32;
    layout()->width = 0;
    content()->layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;

    scrollbar_view* vs = view_item::cast<scrollbar_view>(v_scroll);
    scrollbar_view* hs = view_item::cast<scrollbar_view>(h_scroll);
    vs->disabled = true;
    hs->disabled = true;

    remove_child(bottom);
}
