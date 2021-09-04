#ifndef EXPLORER_VIEW_H
#define EXPLORER_VIEW_H

#include "view.h"
#include "panel.h"

struct explorer_view : panel_view {
    explorer_view();
    
    view_item_ptr vscroll;
    view_item_ptr hscroll;

    void prelayout() override;
    void postlayout() override;
    void update() override;
    bool on_scroll() override;
    bool mouse_wheel(int x, int y) override;

    void _validate();
};

#endif // EXPLORER_VIEW_H