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
};

#endif // EXPLORER_VIEW_H