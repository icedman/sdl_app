#include "gutter.h"
#include "editor_view.h"
#include "hash.h"
#include "text_block.h"

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

int text_width(int i, font_t* font)
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
    if (!editor->subcontent)
        return;

    while (children.size() < editor->subcontent->children.size()) {
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

    view_list::iterator it = children.begin();
    view_list::iterator sit = editor->subcontent->children.begin();
    it = children.begin();
    while (it != children.end()) {
        view_ptr c = *it++;
        view_ptr sc = *sit++;
        rich_text_block_t* scb = sc->cast<rich_text_block_t>();

        c->layout()->y = sc->layout()->render_rect.y - layout()->render_rect.y;
        c->layout()->visible = false;

        if (!scb->block || !sc->layout()->visible)
            break;

        if (scb->block) {
            c->cast<text_block_t>()->set_text(std::to_string(scb->block->lineNumber + 1));
            c->layout()->x = layout()->width - text_width((scb->block->lineNumber + 1) * 10, font());
            c->layout()->visible = true;

            text_span_t ts = {
                .start = 0,
                .length = c->cast<text_block_t>()->text().length(),
                .fg = clr,
                .bg = { 0, 0, 0, 0 },
                .bold = false,
                .italic = false,
                .underline = false,
                .caret = 0
            };
            c->cast<text_block_t>()->_text_spans.clear();
            c->cast<text_block_t>()->_text_spans.push_back(ts);
        }

        if (sit == editor->subcontent->children.end())
            break;
    }

    while (it != children.end()) {
        view_ptr c = *it++;
        c->layout()->visible = false;
    }
}

int gutter_t::content_hash(bool peek)
{
    struct gutter_hash_data_t {
        int scroll_x;
        int scroll_y;
    };

    gutter_hash_data_t hash_data = {
        editor->scrollarea->layout()->scroll_x,
        editor->scrollarea->layout()->scroll_y
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

    // printf(">>%x\n", hash);
    return hash;
}
