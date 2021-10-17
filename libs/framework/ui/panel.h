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

    view_ptr content();

    void _validate();

    virtual void postlayout() override;
    virtual bool handle_mouse_wheel(event_t& event);
    virtual bool handle_scrollbar_move(event_t& event);

    void update_scrollbars();
};

#endif // PANEL_VIEW_H