#ifndef GITTER_H
#define GITTER_H

#include "renderer.h"
#include "view.h"

struct gutter_t : view_t {
    gutter_t();

    DECLAR_VIEW_TYPE(view_type_e::CUSTOM, view_t)

    // void render(renderer_t* renderer) override;
};

#endif GITTER_H