#ifndef SCROLLBAR_VIEW_H
#define SCROLLBAR_VIEW_H

#include "view.h"
#include "scrollarea.h"

struct scrollbar_view : scrollarea_view {
    scrollbar_view();

    bool mouse_wheel(int x, int y) override;
    bool mouse_drag_start(int x, int y) override;
    bool mouse_drag_end(int x, int y) override;
    bool mouse_drag(int x, int y) override;
    bool mouse_click(int x, int y, int button) override;

    void on_scroll();
    
    int index;
    int window;
    int count;

    int drag_offset_x;
    int drag_offset_y;
};

#endif // SCROLLBAR_VIEW_H