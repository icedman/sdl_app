#ifndef SPLITTER_H
#define SPLITTER_H

#include "view.h"

struct splitter_t : view_t {
    splitter_t(view_t* target = NULL, view_t* container = NULL);

    DECLAR_VIEW_TYPE(SPLITTER, view_t)

    void render(renderer_t* renderer) override;

    virtual bool handle_mouse_drag_start(event_t& event);
    virtual bool handle_mouse_drag_end(event_t& event);
    virtual bool handle_mouse_drag(event_t& event);

    bool dragging;
    int drag_start_x;
    int drag_start_y;
    int start_width;
    int start_height;

    view_t* container;
    view_t* target;
};

struct vertical_splitter_t : splitter_t {
    vertical_splitter_t(view_t* target = nullptr, view_t* container = nullptr)
        : splitter_t(target, container)
    {
    }
};

struct horizontal_splitter_t : splitter_t {
    horizontal_splitter_t(view_t* target = nullptr, view_t* container = nullptr);
};

#endif // SPLITTER_H