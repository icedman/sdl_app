#include "gutter_view.h"
#include "renderer.h"

#include "app.h"
#include "editor_view.h"
#include "style.h"

gutter_view::gutter_view()
    : view_item("gutter")
{
}

void gutter_view::render()
{
    editor_view* ev = (editor_view*)(parent->parent);
    editor_ptr editor = ev->editor;

    app_t* app = app_t::instance();
    view_style_t vs = view_style_get("gutter");

    layout_item_ptr lo = layout();

    if (Renderer::instance()->is_terminal()) {
        // do something else
    } else {
        Renderer::instance()->draw_rect({ lo->render_rect.x, lo->render_rect.y, lo->render_rect.w, lo->render_rect.h },
            { (uint8_t)vs.bg.red, (uint8_t)vs.bg.green, (uint8_t)vs.bg.blue }, true);
    }

    int fw, fh;
    Renderer::instance()->get_font_extents(Renderer::instance()->font((char*)vs.font.c_str()), &fw, &fh, NULL, 1);

    int start = ev->start_row;
    int view_height = ev->rows;

    document_t* doc = &editor->document;
    block_list::iterator it = doc->blocks.begin();
    if (start >= doc->blocks.size()) {
        start = (doc->blocks.size() - 1);
    }
    it += start;

    block_list& snapBlocks = editor->snapshots[0].snapshot;

    int l = 0;
    while (it != doc->blocks.end() && l < view_height) {
        block_ptr block = *it++;

        blockdata_t* blockData = 0;
        if (block->data) {
            blockData = block->data.get();
        }

        if (!blockData && block->lineNumber < snapBlocks.size()) {
            block_ptr sb = snapBlocks[block->lineNumber];
            if (sb->data) {
                blockData = sb->data.get();
            }
        }

        if (!blockData || block->y == -1) {
            Renderer::instance()->throttle_up();
            return;
        }

        int linc = block->lineCount;
        int y = block->y;

        std::string ln = std::to_string(block->lineNumber + 1);
        // std::string ln = std::to_string(block->length());

        Renderer::instance()->draw_text(Renderer::instance()->font((char*)vs.font.c_str()), ln.c_str(),
            lo->render_rect.x + lo->render_rect.w - ((ln.length() + 1) * fw),
            y,
            { (uint8_t)vs.fg.green, (uint8_t)vs.fg.green, (uint8_t)vs.fg.blue,
                (uint8_t)(Renderer::instance()->is_terminal() ? vs.fg.index : 125) });

        // log(">line:%d %d %d\n", block->lineNumber + 1, lo->render_rect.x + lo->render_rect.w - ((ln.length() + 1) * fw), y);
        l += linc;
    }
}