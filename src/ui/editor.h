#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include "view.h"
#include "text.h"

struct editor_view : view_item {
    editor_view();

    bool mouse_wheel(int x, int y) override;
    bool on_scroll() override; // scrollbar events
    
    bool mouse_down(int x, int y, int button, int clicks = 0) override;
    bool mouse_up(int x, int y, int button) override;
    bool mouse_move(int x, int y, int button) override;
    void render() override;
    void precalculate() override;

    int start;

    view_item_ptr vscroll;
    view_item_ptr hscroll;

    int mouse_x;
    int mouse_y;
};


#endif // EDITOR_VIEW_H