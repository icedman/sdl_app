#include "minimap.h"
#include "editor_view.h"

minimap_t::minimap_t(editor_view_t* editor)
    : view_t()
    , editor(editor)
{
}

void minimap_t::render(renderer_t* renderer)
{
    render_frame(renderer);
}