#include "gutter.h"
#include "editor_view.h"
#include "text_block.h"

gutter_t::gutter_t(editor_view_t* editor)
    : vertical_container_t()
    , editor(editor)
{
    layout()->width = 1;
    layout()->justify = LAYOUT_JUSTIFY_FLEX_END;
}

int text_width(int i, font_t* font)
{
    return std::to_string(i).length() * font->width;
}

void gutter_t::render(renderer_t* renderer)
{
    int block_count = editor->editor->document.blocks.size();
    int width = text_width(block_count * 100, font());
    layout()->width = width;

    if (!editor->subcontent) return;

    while(children.size() < editor->subcontent->children.size()) {
        view_ptr item = std::make_shared<text_block_t>();
        item->layout()->stack = true;
        add_child(item);
    }

    view_list::iterator it = children.begin();
    view_list::iterator sit = editor->subcontent->children.begin();
    it = children.begin();
    while(it != children.end()) {
        view_ptr c = *it++;
        view_ptr sc = *sit++;
        rich_text_block_t* scb = sc->cast<rich_text_block_t>();
        c->layout()->y = sc->layout()->render_rect.y;
        c->layout()->visible = false;

        if (!scb->block || !sc->layout()->visible) break;

        if (scb->block) {
            c->cast<text_block_t>()->set_text(std::to_string(scb->block->lineNumber + 1));
            c->layout()->x = layout()->width - text_width((scb->block->lineNumber + 1) * 10, font());
            c->layout()->visible = true;
        }

        if (sit == editor->subcontent->children.end()) break;
    }

    while(it != children.end()) {
        view_ptr c = *it++;
        c->layout()->visible = false;
    }
}