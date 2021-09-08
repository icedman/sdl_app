#include "gutter_view.h"
#include "renderer.h"
#include "render_cache.h"

#include "app.h"
#include "editor_view.h"

extern std::map<int, color_info_t> colorMap;

gutter_view::gutter_view()
    : view_item("gutter")
{}

void gutter_view::prelayout()
{
    if (!editor) return;

    editor_view *ev = (editor_view*)(parent->parent);

    int fw, fh;
    ren_get_font_extents(ren_font((char*)ev->font.c_str()), &fw, &fh, NULL, 1, true);

    block_ptr block = editor->document.lastBlock();
    std::string lineNo = std::to_string(1 + block->lineNumber);
    layout()->width = (lineNo.length() + 3) * fw;
}

void gutter_view::render()
{
    if (!editor) return;

    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");
    fg_index = comment.foreground.index;

    view_item::render();

    layout_item_ptr lo = layout();
    editor_view *ev = (editor_view*)(parent->parent);

    int fw, fh;
    ren_get_font_extents(ren_font((char*)(ev->font.c_str())), &fw, &fh, NULL, 1, true);

    int start = ev->start_row;
    int view_height = ev->rows;

    document_t* doc = &editor->document;
    block_list::iterator it = doc->blocks.begin();
    if (start >= doc->blocks.size()) {
        start = (doc->blocks.size() - 1);
    }
    it += start;

    color_info_t fg = colorMap[fg_index];

    int l = 0;
    while (it != doc->blocks.end() && l < view_height) {
        block_ptr block = *it++;

        struct blockdata_t* blockData;
        if (block->data) {
            blockData = block->data.get();
        }
        if (!blockData) {
            // draw_text(NULL, (char*)block->text().c_str(),
            //     lo->render_rect.x,
            //     lo->render_rect.y + (l*fh),
            //     { (uint8_t)fg.red,(uint8_t)fg.green,(uint8_t)fg.blue },
            //     false, false, true);

            // l++;
            // continue;
            return;
        }

        span_info_t s = blockData->spans[0];

        std::string ln = std::to_string(block->lineNumber + 1);
        ren_draw_text(ren_font("editor"), ln.c_str(), 
            lo->render_rect.x + lo->render_rect.w - ((ln.length() + 1) * fw),
            s.y, { (uint8_t)fg.red, (uint8_t)fg.green, (uint8_t)fg.blue, 125 });

        l++;
    }
}