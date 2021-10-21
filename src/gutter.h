#ifndef GUTTER_H
#define GUTTER_H

#include "renderer.h"
#include "view.h"

struct editor_view_t;
struct gutter_t : vertical_container_t {
    gutter_t(editor_view_t* editor);

    DECLAR_VIEW_TYPE(view_type_e::CUSTOM, vertical_container_t)
    virtual std::string type_name() { return "gutter"; }

    void render(renderer_t* renderer) override;
    int content_hash(bool peek) override;

    editor_view_t* editor;
};

#endif GUTTER_H