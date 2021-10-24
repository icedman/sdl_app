#ifndef STATUS_BAR_H
#define STATUS_BAR_H

#include "view.h"

struct statusbar_t : horizontal_container_t {
    statusbar_t();

    DECLAR_VIEW_TYPE(view_type_e::STATUSBAR, horizontal_container_t)

    view_ptr add_status(std::string text, int pos = 0, int order = 0);
    void remove_status(view_ptr view);

    virtual void render(renderer_t* renderer) override;

    view_ptr left;
    view_ptr right;
};

#endif // STATUS_BAR_H