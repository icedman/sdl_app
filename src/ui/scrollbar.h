#ifndef SCROLLBAR_VIEW_H
#define SCROLLBAR_VIEW_H

#include "view.h"
#include "scrollarea.h"

struct scrollbar_view : scrollarea_view {
    scrollbar_view();

    void prelayout() override;
    void postlayout() override;
    void render() override;
    
    bool mouse_drag_start(int x, int y) override;
    bool mouse_drag_end(int x, int y) override;
    bool mouse_drag(int x, int y) override;
    bool mouse_click(int x, int y, int button) override;

    void propagate_scrollbar_event();

    void set_index(int idx);
    void set_size(int c, int w);

    void _scroll(int pos);

    void _validate();
    int _thumb_size();
    int _bar_size();
    int _scroll_pos();

    float percentage();

    int index;
    int window;
    int count;

    bool dragging;
    int drag_offset;
};

struct vscrollbar_view : scrollbar_view {
    vscrollbar_view() : scrollbar_view()
    {
        layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;
        layout()->width = 18;
        move_factor_x = 0;
        move_factor_y = 20;
    }
};

struct hscrollbar_view : scrollbar_view {
    hscrollbar_view() : scrollbar_view()
    {
        layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
        layout()->height = 18;
        move_factor_x = 20;
        move_factor_y = 0;
    }
};

#endif // SCROLLBAR_VIEW_H