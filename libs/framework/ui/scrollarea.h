#ifndef SCROLLAREA_H
#define SCROLLAREA_H

#include "view.h"

struct scrollarea_t : view_t {
    scrollarea_t();

    DECLAR_VIEW_TYPE(SCROLLAREA, view_t)

    virtual view_ptr content() override { return _content; }

    void render(renderer_t* renderer) override;

    virtual bool handle_mouse_wheel(event_t& event);

    float scroll_factor_x;
    float scroll_factor_y;

    view_ptr _content;
};

#endif // SCROLLAREA_H