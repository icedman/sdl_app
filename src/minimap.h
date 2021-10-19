#ifndef MINIMAP_H
#define MINIMAP_H

#include "renderer.h"
#include "view.h"

struct editor_view_t;
struct minimap_t : view_t {
    minimap_t(editor_view_t* editor);

    DECLAR_VIEW_TYPE(view_type_e::CUSTOM, view_t)
    virtual std::string type_name() { return "minimap"; }

    void render(renderer_t* renderer) override;

    editor_view_t* editor;
};

#endif // MINIMAP_H