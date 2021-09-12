#include "gutter_view.h"
#include "render_cache.h"
#include "renderer.h"

#include "app.h"
#include "style.h"
#include "editor_view.h"

gutter_view::gutter_view()
    : view_item("gutter")
{
}

void gutter_view::render()
{
    return;

    editor_view* ev = (editor_view*)(parent->parent);
    editor_ptr editor = ev->editor;

    app_t* app = app_t::instance();
    view_style_t vs = view_style_get("gutter");

    layout_item_ptr lo = layout();

    // view_item::render();
    ren_draw_rect({
        lo->render_rect.x, lo->render_rect.y, lo->render_rect.w, lo->render_rect.h
    } , { (uint8_t)vs.bg.red, (uint8_t)vs.bg.green, (uint8_t)vs.bg.blue }, true);

    int fw, fh;
    ren_get_font_extents(ren_font((char*)vs.font.c_str()), &fw, &fh, NULL, 1, true);

    int start = ev->start_row;
    int view_height = ev->rows;

    document_t* doc = &editor->document;
    block_list::iterator it = doc->blocks.begin();
    if (start >= doc->blocks.size()) {
        start = (doc->blocks.size() - 1);
    }
    it += start;

    int l = 0;
    while (it != doc->blocks.end() && l < view_height) {
        block_ptr block = *it++;

        struct blockdata_t* blockData;
        if (block->data) {
            blockData = block->data.get();
        }
        if (!blockData) {
            return;
        }

        int linc = 0;
        int y = 0;
        for(auto s : blockData->rendered_spans) {
            if (s.line > linc) {
                linc = s.line;
            }
            if (s.line == 0) {
                y = s.y;
            }
        }

        std::string ln = std::to_string(block->lineNumber + 1);
        ren_draw_text(ren_font((char*)vs.font.c_str()), ln.c_str(),
            lo->render_rect.x + lo->render_rect.w - ((ln.length() + 1) * fw),
            y,
            { (uint8_t)vs.fg.green, (uint8_t)vs.fg.green, (uint8_t)vs.fg.blue, 125 });

        l+=linc;
    }
}