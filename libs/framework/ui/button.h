#ifndef BUTTON_H
#define BUTTON_H

#include "view.h"

struct button_t : horizontal_container_t {
    button_t(std::string text = "");

    DECLAR_VIEW_TYPE(view_type_e::BUTTON, horizontal_container_t)

    void set_text(std::string text);
    void set_icon(std::string text);
    view_ptr text();
    view_ptr icon();

    view_ptr _text;
    view_ptr _icon;

    void render(renderer_t* renderer) override;
};

#endif BUTTON_H