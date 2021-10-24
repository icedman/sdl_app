#include "rich_text.h"
#include "damage.h"
#include "hash.h"
#include "scrollarea.h"
#include "system.h"
#include "text.h"

#define VISIBLE_BLOCKS_PAD 4

rich_text_block_t::rich_text_block_t()
    : text_block_t()
{
}

rich_text_t::rich_text_t()
    : panel_t()
    , visible_blocks(0)
    , wrapped(false)
    , draw_cursors(false)
    , defer_relayout(DEFER_LAYOUT_FRAMES)
{
    editor = std::make_shared<editor_t>();
    layout()->name = "rich_text";
    layout()->margin_top = 4;

    layout()->prelayout = [this](layout_item_t* item) {
        this->prelayout();
        return true;
    };

    on(EVT_STAGE_IN, [this](event_t& event) {
        if (event.source == this) {
            this->defer_relayout = DEFER_LAYOUT_FRAMES;
        }
        return true;
    });
}

view_ptr rich_text_t::create_block()
{
    view_ptr item = std::make_shared<rich_text_block_t>();
    item->cast<rich_text_block_t>()->set_text("BLOCK TEMPLATE");
    return item;
}

void rich_text_t::update_block(view_ptr item, block_ptr block)
{
    rich_text_block_t* editor_block = item->cast<rich_text_block_t>();
    editor_block->wrapped = wrapped;

    if (!block) {
        block = std::make_shared<block_t>();
    }

    editor_block->block = block;
    editor_block->set_text(block->text() + " ");

    if (!block->data) {
        block->data = std::make_shared<blockdata_t>();
        block->data->dirty = true;
    }
    if (block->data->dirty && editor->highlighter.lang) {
        editor->highlight(block->lineNumber, 1);
    }

    editor_block->_text_spans.clear();

    // default color
    if (color_is_set(fg)) {
        text_span_t ts = {
            .start = 0,
            .length = block->length(),
            .fg = fg,
            .bg = { 0, 0, 0, 0 },
            .bold = false,
            .italic = false,
            .underline = false,
            .caret = 0
        };
        editor_block->_text_spans.push_back(ts);
    }

    for (auto s : block->data->spans) {
        text_span_t ts;
        memcpy(&ts, &s, sizeof(text_span_t));

        ts.caret = false;
        editor_block->_text_spans.push_back(ts);
    }

    if (draw_cursors) {
        document_t* doc = &editor->document;
        cursor_list cursors = doc->cursors;
        cursor_t mainCursor = doc->cursor();

        for (int pos = 0; pos < block->length(); pos++) {
            bool hl = false;
            bool ul = false;
            int cr = 0;

            for (auto c : cursors) {
                if (pos == c.position() && block == c.block()) {
                    cr = 1;
                    hl = c.hasSelection();

                    if (hl && !c.isSelectionNormalized()) {
                        cr = 2;
                    }
                    break;
                }
                if (!c.hasSelection())
                    continue;

                cursor_position_t start = c.selectionStart();
                cursor_position_t end = c.selectionEnd();

                if (block->lineNumber < start.block->lineNumber || block->lineNumber > end.block->lineNumber)
                    continue;
                if (block == start.block && pos < start.position)
                    continue;
                if (block == end.block && pos > end.position)
                    continue;

                hl = true;
                break;
            }

            if (hl || ul || cr) {
                text_span_t ts = {
                    .start = pos,
                    .length = 1,
                    .fg = { 0, 0, 0, 0 },
                    .bg = { 0, 0, 0, 0 },
                    .bold = false,
                    .italic = false,
                    .underline = ul,
                    .caret = cr
                };
                if (hl) {
                    ts.bg = sel;
                }
                editor_block->_text_spans.push_back(ts);
            }
        }
    }
}

void rich_text_t::update_block(block_ptr block)
{
    for (auto v : subcontent->children) {
        rich_text_block_t* tb = (rich_text_block_t*)v.get();
        if (tb->block == block) {
            update_block(v, block);
        }
    }
}

void rich_text_t::update_blocks()
{
    for (auto c : subcontent->children) {
        update_block(c, c->cast<rich_text_block_t>()->block);
    }
}

void rich_text_t::prerender()
{
    if (defer_relayout > 0) {
        relayout_virtual_blocks();
        defer_relayout--;

        update_scrollbars();
        relayout_virtual_blocks();
    }

    panel_t::prerender();
}

void rich_text_t::prelayout()
{
    fg = { 255, 255, 255, 0 };
    sel = { 150, 150, 150, 125 };

    if (editor->highlighter.theme) {
        color_info_t t_clr;
        color_t clr;
        editor->highlighter.theme->theme_color("editor.foreground", t_clr);
        clr = { t_clr.red * 255, t_clr.green * 255, t_clr.blue * 255 };
        if (color_is_set(clr)) {
            fg = clr;
        }
        editor->highlighter.theme->theme_color("editor.background", t_clr);
        clr = { t_clr.red * 255, t_clr.green * 255, t_clr.blue * 255 };
        if (color_is_set(clr)) {
            bg = clr;
        }
        editor->highlighter.theme->theme_color("editor.selectionBackground", t_clr);
        clr = { t_clr.red * 255, t_clr.green * 255, t_clr.blue * 255 };
        if (color_is_set(clr)) {
            sel = clr;
            sel.a = 125;
        }
        system_t::instance()->renderer.foreground = fg;
        system_t::instance()->renderer.background = bg;
    }

    if (!subcontent) {
        lead_spacer = std::make_shared<view_t>();
        tail_spacer = std::make_shared<view_t>();

        subcontent = std::make_shared<view_t>();
        subcontent->layout()->fit_children_x = !wrapped;
        subcontent->layout()->fit_children_y = true;

        content()->add_child(lead_spacer);
        content()->add_child(subcontent);
        content()->add_child(tail_spacer);
    }
}

void rich_text_t::render(renderer_t* renderer)
{
    panel_t::render(renderer);
}

void rich_text_t::relayout_virtual_blocks()
{
    // moving this code to prelayout.. is explensive (as relayout is often called multiple times)
    layout_item_ptr lo = layout();

    int blocks_count = editor->document.blocks.size();
    block_height = font()->height;

    visible_blocks = lo->render_rect.h / block_height;
    visible_blocks += VISIBLE_BLOCKS_PAD;
    lead_spacer->layout()->height = 1;

    while (subcontent->children.size() < visible_blocks) {
        view_ptr item = create_block();
        subcontent->add_child(item);
    }

    tail_spacer->layout()->height = (blocks_count - visible_blocks) * block_height;

    layout_item_ptr slo = scrollarea->layout();

    scrollarea->cast<scrollarea_t>()->scroll_factor_x = font()->width;
    scrollarea->cast<scrollarea_t>()->scroll_factor_y = font()->height * 1.25f;

    first_visible = -slo->scroll_y / block_height;

    view_list::iterator vit = subcontent->children.begin();
    block_list::iterator it = editor->document.blocks.begin();
    if (first_visible >= editor->document.blocks.size()) {
        first_visible = editor->document.blocks.size() - 1;
    }
    it += first_visible;

    bool dirty_layout = false;
    int i = 0;
    vit = subcontent->children.begin();
    while (it != editor->document.blocks.end() && vit != subcontent->children.end()) {
        view_ptr v = *vit++;
        block_ptr block = *it++;

        v->layout()->wrap = wrapped;
        v->layout()->visible = true;
        if (wrapped) {
            v->layout()->width = lo->render_rect.w;
        }

        if (v->cast<rich_text_block_t>()->block == block) {
            if (i++ > visible_blocks)
                break;
            continue;
        }

        dirty_layout = true;
        update_block(v, block);

        if (i++ > visible_blocks)
            break;
    }

    while (vit != subcontent->children.end()) {
        view_ptr v = *vit++;
        v->layout()->visible = false;
    }

    int vc = 0;
    for (auto c : subcontent->children) {
        if (c->layout()->visible) {
            vc++;
        }

        // update line count
        if (c->cast<rich_text_block_t>()->block) {
            int lc = c->layout()->render_rect.h / block_height;
            c->cast<rich_text_block_t>()->block->lineCount = lc;
        }
    }

    lead_spacer->layout()->height = (first_visible * block_height);
    lead_spacer->layout()->visible = lead_spacer->layout()->height > 1;
    tail_spacer->layout()->height = 8 * block_height;

    int computed = lead_spacer->layout()->height + tail_spacer->layout()->height + (vc * block_height);
    int total_height = (blocks_count + 8) * block_height;
    if (computed < total_height) {
        tail_spacer->layout()->height += total_height - computed;
    }
    tail_spacer->layout()->visible = tail_spacer->layout()->height > 1;

    layout_clear_hash(layout(), 6);
    relayout();
    relayout();
}

bool rich_text_t::handle_mouse_wheel(event_t& event)
{
    if (event.x != -1 && event.y != -1) {
        // if x or y equals to -1, .. synthetic..
        defer_relayout = DEFER_LAYOUT_FRAMES;
    }
    return panel_t::handle_mouse_wheel(event);
}

bool rich_text_t::handle_scrollbar_move(event_t& event)
{
    defer_relayout = DEFER_LAYOUT_FRAMES;
    return panel_t::handle_scrollbar_move(event);
}
