#include "minimap.h"
#include "editor_view.h"
#include "hash.h"

#define DRAW_SCALE 0.80f
#define DRAW_SPACING 3
#define DRAW_HEIGHT 1
#define DRAW_ALPHA 200
#define DRAW_MARGIN_LEFT 8

minimap_t::minimap_t(editor_view_t* editor)
    : view_t()
    , editor(editor)
    , hltd(0)
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

    layout()->width = 80;
    layout()->name = "minimap";
}

static size_t countIndentSize(std::string s)
{
    for (int i = 0; i < s.length(); i++) {
        if (s[i] != ' ') {
            return i;
        }
    }
    return 0;
}

void minimap_t::render(renderer_t* renderer)
{
    int alpha = DRAW_ALPHA;

    // render_frame(renderer);
    layout_item_ptr lo = layout();
    int rows = lo->render_rect.h / font()->height;

    renderer->draw_rect(lo->render_rect, this->editor->bg, true);

    int spacing = DRAW_SPACING;

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

    block_list::iterator it = doc->blocks.begin();
    it += (scroll_y / spacing);

    start_row = -1;
    end_row = 0;
    render_h = 0;

    render_y = 0;
    int render_current_y = 0;

    int l = 0;
    while (it != doc->blocks.end() && l < lo->render_rect.h) {
        block_ptr block = *it;
        it++;

        uint8_t alpha = DRAW_ALPHA;

        if (!block->data || block->data->dirty) {
            ev->request_highlight(block);
            hltd++;
        }

        blockdata_ptr blockData = block->data;

        if (block == current_block) {
            alpha = 250;
        }

        int ind = countIndentSize(block->text());

        if (!blockData) {
            rect_t r = {
                lo->render_rect.x + (ind * DRAW_SCALE) + DRAW_MARGIN_LEFT,
                lo->render_rect.y + l,
                ((int)(block->length() - ind) * DRAW_SCALE),
                DRAW_HEIGHT,
            };
            if (r.w > 0) {
                color_t clr = ev->fg;
                clr.a = alpha * 0.5f;
                renderer->draw_rect(r,
                    clr,
                    true, 0);
            }
        }

        if (blockData) {
            for (auto s : blockData->spans) {

                color_t clr = { s.fg.r, s.fg.g, s.fg.b, alpha };
                if (!color_is_set(clr)) {
                    clr = ev->fg;
                    clr.a = alpha;
                }

                int start = s.start * DRAW_SCALE;
                int length = s.length * DRAW_SCALE;
                if (length == 0 && s.length > 0) {
                    length = 1;
                    clr.a = alpha / 2;
                }
                rect_t r = {
                    lo->render_rect.x + start + 2 + DRAW_MARGIN_LEFT,
                    lo->render_rect.y + l,
                    length,
                    DRAW_HEIGHT,
                };

                if (render_h < l) {
                    render_h = l;
                }

                if (start + r.w + 2 > lo->render_rect.w) {
                    r.w = lo->render_rect.w - (start + 2);
                }

                if (r.w > 0 && s.start >= ind) {
                    renderer->draw_rect(r,
                        clr,
                        true, 0);
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

    // renderer->draw_rect(
    //     {
    //         lo->render_rect.x,
    //         render_current_y - spacing,
    //         lo->render_rect.w,
    //         2,
    //     },
    //     { 255, 255, 255, 40 }, true, 0);
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
    ev->relayout_virtual_blocks();
    rerender();

    return true;
}

int minimap_t::content_hash(bool peek)
{
    struct minimap_hash_data_t {
        int start_row;
        int end_row;
        float sliding_y;
        int render_y;
        int render_h;
        int scroll_y;
        int hltd;
    };

    minimap_hash_data_t hash_data = {
        start_row,
        end_row,
        sliding_y,
        render_y,
        render_h,
        editor->scrollarea->layout()->scroll_y,
        hltd
    };

    hltd = 0;

    int hash = murmur_hash(&hash_data, sizeof(minimap_hash_data_t), CONTENT_HASH_SEED);
    if (!peek) {
        _content_hash = hash;
    }

    return hash;
}