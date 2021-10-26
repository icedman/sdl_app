#include "gutter.h"
#include "editor_view.h"
#include "hash.h"
#include "text_block.h"

#define GUTTER_CURRENT_ALPHA 75

gutter_t::gutter_t(editor_view_t* editor)
    : vertical_container_t()
    , editor(editor)
{
    layout()->width = 1;
    layout()->justify = LAYOUT_JUSTIFY_FLEX_END;

    layout()->prelayout = [this](layout_item_t* item) {
        this->prelayout();
        return true;
    };

    layout()->name = "gutter";
}

static inline int text_width(int i, font_ptr font)
{
    return std::to_string(i).length() * font->width;
}

void gutter_t::prelayout()
{
    int block_count = editor->editor->document.blocks.size();
    int width = text_width(block_count * 100, font());
    layout()->width = width;
}

void gutter_t::render(renderer_t* renderer)
{
    view_ptr subcontent = editor->subcontent;
    if (!subcontent)
        return;

    layout_item_ptr lo = layout();
    // renderer->draw_rect(lo->render_rect, editor->bg, true);

    cursor_t cursor = editor->editor->document.cursor();
    block_ptr current_block = cursor.block();

    while (children.size() < subcontent->children.size()) {
        view_ptr item = std::make_shared<text_block_t>();
        item->layout()->stack = true;
        add_child(item);
    }

    color_info_t t_clr;
    style_t comment = editor->editor->highlighter.theme->styles_for_scope("comment");
    t_clr = comment.foreground;
    color_t clr = { t_clr.red * 255, t_clr.green * 255, t_clr.blue * 255 };
    if (!color_is_set(clr)) {
        clr = editor->fg;
    }

    color_t sel = editor->sel;
    sel.a = GUTTER_CURRENT_ALPHA;

    for (auto c : subcontent->children) {
        rich_text_block_t* cb = c->cast<rich_text_block_t>();
        if (!cb->is_visible() || !cb->block)
            break;
        std::string text = std::to_string(cb->block->lineNumber + 1);

        if (cb->block == current_block && current_block) {
            rect_t r = { lo->render_rect.x, c->layout()->render_rect.y, lo->render_rect.w, font()->height };
            renderer->draw_rect(r, sel);
        }

        renderer->draw_text(font().get(), (char*)text.c_str(),
            lo->render_rect.x + lo->render_rect.w - (text.length() + 1) * font()->width,
            c->layout()->render_rect.y, clr);
    }
}

int gutter_t::content_hash(bool peek)
{
    cursor_t cursor = editor->editor->document.cursor();
    block_ptr current_block = cursor.block();

    struct gutter_hash_data_t {
        int scroll_x;
        int scroll_y;
        size_t line;
    };

    gutter_hash_data_t hash_data = {
        editor->scrollarea->layout()->scroll_x,
        editor->scrollarea->layout()->scroll_y,
        current_block ? current_block->lineNumber : 0
    };

    int hash = murmur_hash(&hash_data, sizeof(gutter_hash_data_t), CONTENT_HASH_SEED);
    for (auto c : editor->subcontent->children) {
        rich_text_block_t* scb = c->cast<rich_text_block_t>();
        if (!scb->block)
            break;
        hash += scb->block->lineNumber;
    }

    if (!peek) {
        _content_hash = hash;
    }

    return hash;
}
