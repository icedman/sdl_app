#ifndef PANEL_VIEW_H
#define PANEL_VIEW_H

#include "view.h"

struct panel_t : vertical_container_t {
    panel_t();

    DECLAR_VIEW_TYPE(PANEL, vertical_container_t)

    view_ptr v_scroll;
    view_ptr h_scroll;
    view_ptr scrollarea;
    view_ptr resizer;
    view_ptr bottom;

    virtual view_ptr content() override;

    void update() override;
    void _validate();

    virtual void postlayout() override;
    virtual bool handle_mouse_wheel(event_t& event);
    virtual bool handle_scrollbar_move(event_t& event);

    void scroll_to_top();
    void scroll_to_bottom();

    void update_scrollbars();

    int wheel_x;
    int wheel_y;
};

#endif // PANEL_VIEW_H