#ifndef MINIMAP_VIEW_H
#define MINIMAP_VIEW_H

#include "scrollbar.h"
#include "view.h"

struct minimap_view : view_item {
    minimap_view();

    void render() override;
    bool mouse_click(int x, int y, int button) override;

    int scroll_y;
    int spacing;

    int start_y;
    int end_y;
    int render_h;
};

#endif // MINIMAP_VIEW_H