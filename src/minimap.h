#ifndef MINIMAP_H
#define MINIMAP_H

#include "renderer.h"
#include "view.h"

struct editor_view_t;
struct minimap_t : view_t {
    minimap_t(editor_view_t* editor);

    DECLAR_VIEW_TYPE(view_type_e::CUSTOM, view_t)
    std::string type_name() override { return "minimap"; }

    void render(renderer_t* renderer) override;

    bool handle_mouse_click(event_t& event) override;

    editor_view_t* editor;

    int start_row;
    int end_row;

    float sliding_y;
    int render_y;
    int render_h;
};

#endif // MINIMAP_H