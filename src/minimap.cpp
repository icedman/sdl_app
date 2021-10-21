#include "minimap.h"
#include "editor_view.h"

#define DRAW_SCALE 0.75f

minimap_t::minimap_t(editor_view_t* editor)
    : view_t()
    , editor(editor)
{
    can_hover = true;

    on(EVT_MOUSE_DRAG_START, [editor](event_t& evt) {
        evt.cancelled = true;
        return editor->v_scroll->handle_mouse_drag_start(evt);
    });
    on(EVT_MOUSE_DRAG, [editor](event_t& evt) {
        evt.cancelled = true;
        return editor->v_scroll->handle_mouse_drag(evt);
    });
    on(EVT_MOUSE_DRAG_END, [editor](event_t& evt) {
        evt.cancelled = true;
        return editor->v_scroll->handle_mouse_drag_end(evt);
    });
    on(EVT_MOUSE_CLICK, [this](event_t& evt) {
        evt.cancelled = true;
        return this->handle_mouse_click(evt);
    });
}

void minimap_t::render(renderer_t* renderer)
{
    // render_frame(renderer);
    layout_item_ptr lo = layout();
    int rows = lo->render_rect.h / font()->height;

    int spacing = 2;

    editor_view_t* ev = (editor_view_t*)(this->editor);
    editor_ptr editor = ev->editor;
    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();

    block_ptr current_block = cursor.block();

    int start = ev->first_visible;
    int count = doc->blocks.size() + (rows / 3);
    if (count <= 0)
        return;

    float p = (float)start / count;
    int scroll_y = 0;

    if (doc->blocks.size() * spacing > lo->render_rect.h) {
        scroll_y = (p * doc->blocks.size() * spacing);
        scroll_y -= (p * lo->render_rect.h * 3 / 4);
    }
    if (scroll_y < 0) {
        scroll_y = 0;
    }

    // printf("%f\n", p);

    block_list& snapBlocks = editor->snapshots[0].snapshot;

    block_list::iterator it = doc->blocks.begin();
    it += (scroll_y / spacing);

    // printf(">%f %d\n", p, scroll_y);

    start_row = -1;
    end_row = 0;
    render_h = 0;

    render_y = 0;
    int render_current_y = 0;

    int l = 0;
    while (it != doc->blocks.end() && l < lo->render_rect.h) {
        block_ptr block = *it;
        it++;

        uint8_t alpha = 80;

        if (!block->data || block->data->dirty) {
            ev->request_highlight(block);
        }

        blockdata_ptr blockData = block->data;

        if (block == current_block) {
            alpha = 250;
        }

        if (!blockData) {
            rect_t r = {
                lo->render_rect.x,
                lo->render_rect.y + l,
                (int)(block->length() * DRAW_SCALE),
                1,
            };
            if (r.w > 0) {
                renderer->draw_rect(r,
                    { 255, 255, 255, 150 },
                    false, 1);
            }
            blockData = 0;
        }

        if (blockData) {
            for (auto s : blockData->spans) {

                color_t clr = { s.fg.r, s.fg.g, s.fg.b, 150 };
                if (!color_is_set(clr)) {
                    clr = ev->fg;
                    clr.a = 150;
                }

                int start = s.start / 3;
                int length = s.length * DRAW_SCALE;
                rect_t r = {
                    lo->render_rect.x + start + 2,
                    lo->render_rect.y + l,
                    length,
                    1,
                };

                if (render_h < l) {
                    render_h = l;
                }

                if (start + r.w + 2 > lo->render_rect.w) {
                    r.w = lo->render_rect.w - (start + 2);
                }

                if (r.w > 0) {
                    renderer->draw_rect(r,
                        clr,
                        false, 1);
                }
            }
        }

        if (start_row == -1) {
            start_row = block->lineNumber;
        }
        if (block->lineNumber == start) {
            render_y = lo->render_rect.y + l;
        }
        if (block == current_block) {
            render_current_y = lo->render_rect.y + l;
        }

        end_row = block->lineNumber;

        l += spacing;
    }

    if (is_hovered(this)) {
        sliding_y = render_y;
        color_t clr = ev->sel;
        clr.a = 50;
        renderer->draw_rect(
            {
                lo->render_rect.x,
                sliding_y,
                lo->render_rect.w,
                rows * spacing,
            },
            clr, true, 0, {}, 4);
    }

    renderer->draw_rect(
        {
            lo->render_rect.x,
            render_current_y - spacing,
            lo->render_rect.w,
            2,
        },
        { 255, 255, 255, 40 }, true, 0);
}

bool minimap_t::handle_mouse_click(event_t& event)
{
    layout_item_ptr lo = layout();
    int ry = event.y - lo->render_rect.y;
    if (ry > render_h)
        return true;

    int line = start_row + ((float)(end_row - start_row) * (float)ry / render_h);

    editor_view_t* ev = this->editor;
    editor_ptr editor = ev->editor;

    if (line < 0) {
        line = 0;
    }
    if (line >= editor->document.blocks.size()) {
        line = editor->document.blocks.size() - 1;
    }

    cursor_t cursor;
    cursor.setPosition(editor->document.blockAtLine(line), 0);
    ev->scroll_to_cursor(cursor);

    return true;
}