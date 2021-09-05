#ifndef PANEL_VIEW_H
#define PANEL_VIEW_H

#include "view.h"

struct panel_view : vertical_container {
    panel_view();

    view_item_ptr v_scroll;
    view_item_ptr h_scroll;
    view_item_ptr scrollarea;
    view_item_ptr resizer;

    view_item_ptr content();

    view_item_ptr _hscrollbar_container;

    void _validate();

    virtual void update() override;
    virtual bool mouse_wheel(int x, int y) override;
    virtual bool scrollbar_move() override;
};

#endif // PANEL_VIEW_H