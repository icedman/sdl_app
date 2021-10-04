#ifndef SCROLLAREA_VIEW_H
#define SCROLLAREA_VIEW_H

#include "view.h"

struct scrollarea_view : view_item {
    scrollarea_view();

    DECLAR_VIEW_TYPE(SCROLLAREA, view_item)

    virtual void prerender() override;
    virtual bool mouse_wheel(int x, int y) override;

    view_item_ptr content;
    float move_factor_x;
    float move_factor_y;

    int prev_scroll_x;
    int prev_scroll_y;
};

#endif // SCROLLAREA_VIEW_H