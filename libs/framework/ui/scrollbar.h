#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include "scrollarea.h"
#include "view.h"

struct scrollbar_t : scrollarea_t {
    scrollbar_t();

    DECLAR_VIEW_TYPE(SCROLLBAR, scrollarea_t)

    void postlayout() override;
    void render(renderer_t* renderer) override;

    virtual bool handle_mouse_drag_start(event_t& event) override;
    virtual bool handle_mouse_drag_end(event_t& event) override;
    virtual bool handle_mouse_drag(event_t& event) override;
    virtual bool handle_mouse_click(event_t& event) override;

    void propagate_scrollbar_event();

    void set_index(int idx);
    void set_size(int c, int w);

    // internal data update
    void _scroll(int pos);
    void _validate();

    // visual
    int thumb_size();
    int bar_size();
    int scroll_pos();

    // data
    int index;
    int window;
    int count;

    bool dragging;
    int drag_offset;
};

struct vertical_scrollbar_t : scrollbar_t {
    vertical_scrollbar_t()
        : scrollbar_t()
    {
        layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;
        layout()->width = 18;
        scroll_factor_x = 0;
    }
};

struct horizontal_scrollbar_t : scrollbar_t {
    horizontal_scrollbar_t()
        : scrollbar_t()
    {
        layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
        layout()->height = 18;
        scroll_factor_y = 0;
    }
};

#endif // SCROLLBAR_H