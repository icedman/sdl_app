#ifndef SCROLLBAR_VIEW_H
#define SCROLLBAR_VIEW_H

#include "view.h"
#include "scrollarea.h"

struct scrollbar_view : scrollarea_view {
    scrollbar_view();

    void prelayout() override;
    void render() override;
    
    bool mouse_down(int x, int y, int button, int clicks = 0) override { return true; };
    bool mouse_up(int x, int y, int button) override { return true; };
    bool mouse_move(int x, int y, int button) override { return true; };

    bool mouse_drag_start(int x, int y) override;
    bool mouse_drag_end(int x, int y) override;
    bool mouse_drag(int x, int y) override;
    bool mouse_click(int x, int y, int button) override;
    bool on_scroll() override;

    void set_index(int idx);
    void set_size(int c, int w);

    void _scroll(int pos);

    int _thumbSize();
    int _barSize();
    int _scrollPos();

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
    }
};

struct hscrollbar_view : scrollbar_view {
    hscrollbar_view() : scrollbar_view()
    {
        layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
        layout()->height = 18;
    }
};

#endif // SCROLLBAR_VIEW_H