#ifndef BUTTON_H
#define BUTTON_H

#include "view.h"

struct button_t : horizontal_container_t {
    button_t(std::string text = "");

    DECLAR_VIEW_TYPE(view_type_e::BUTTON, horizontal_container_t)

    view_ptr text;

    void render(renderer_t* renderer) override;
};

#endif BUTTON_H