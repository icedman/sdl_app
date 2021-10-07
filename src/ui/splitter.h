#ifndef SPLITTER_H
#define SPLITTER_H

#include "view.h"

struct splitter_view : view_item {
    splitter_view();

    DECLAR_VIEW_TYPE(SPLITTER, view_item)

    void prerender() override;
    void render() override;

    bool mouse_drag_start(int x, int y) override;
    bool mouse_drag_end(int x, int y) override;
    bool mouse_drag(int x, int y) override;

    bool dragging;
    int drag_start_x;
    int drag_start_y;
    int start_width;

    view_item_ptr container;
    view_item_ptr target;
};

#endif // SPLITTER_H