#include "minimap_view.h"
#include "editor.h"
#include "editor_view.h"
#include "events.h"
#include "renderer.h"

#include "app.h"
#include "dots.h"
#include "style.h"

#define DRAW_SCALE 0.5f
#define TEXT_COMPRESS 5
#define TEXT_BUFFER 25

#define WORKER_HL_WINDOW 128

minimap_view::minimap_view()
    : view_item()
    , prev_scroll_y(-1)
    , prev_start_row(-1)
    , prev_end_row(-1)
    , render_y(0)
    , sliding_y(0)
{
    class_name = "minimap";
    interactive = true;

    spacing = 2;

    on(EVT_MOUSE_DRAG_START, [this](event_t& evt) {
        evt.cancelled = true;
        return this->scrollbar->mouse_drag_start(evt.x, evt.y);
    });
    on(EVT_MOUSE_DRAG, [this](event_t& evt) {
        evt.cancelled = true;
        return this->scrollbar->mouse_drag(evt.x, evt.y);
    });
    on(EVT_MOUSE_DRAG_END, [this](event_t& evt) {
        evt.cancelled = true;
        return this->scrollbar->mouse_drag_end(evt.x, evt.y);
    });

    on(EVT_MOUSE_CLICK, [this](event_t e) {
        e.cancelled = true;
        return this->mouse_click(e.x, e.y, e.button);
    });
}

bool minimap_view::mouse_click(int x, int y, int button)
{
    should_damage();

    layout_item_ptr lo = layout();
    int ry = y - lo->render_rect.y;
    if (ry > render_h)
        return true;

    int line = start_row + ((float)(end_row - start_row) * (float)ry / render_h);

    editor_view* ev = (editor_view*)(parent->parent);
    editor_ptr editor = ev->editor;

    if (line < 0) {
        line = 0;
    }
    if (line >= editor->document.blocks.size()) {
        line = editor->document.blocks.size() - 1;
    }

    cursor_t cursor;
    cursor.setPosition(editor->document.blockAtLine(line), 0);
    ev->scroll_to_cursor(cursor, true);

    Renderer::instance()->throttle_up_events(240);
    return true;
}

void minimap_view::update(int millis)
{
    view_item::update(millis);

    editor_view* ev = (editor_view*)(parent->parent);
    editor_ptr editor = ev->editor;
    document_t* doc = &editor->document;

    if (start_row > doc->blocks.size() || start_row < 0) {
        start_row = 0;
    }

    int d = render_y - sliding_y;
    if (d * d >= 36) {
        sliding_y += (float)d / 80;
    } else {
        sliding_y = render_y;
    }
}

bool minimap_view::worker(int ticks)
{
    if (!hl.size()) {
        return false;
    }

    // printf("%d %d\n", ticks, hl.size());

    while (hl.size() > WORKER_HL_WINDOW) {
        hl.erase(hl.begin());
    }

    editor_view* ev = (editor_view*)(parent->parent);
    editor_ptr editor = ev->editor;

    for (int i = 0; i < 4; i++) {
        block_ptr block = hl.front();
        hl.erase(hl.begin());
        editor->highlighter.highlightBlock(block);
        if (!hl.size())
            break;
    }

    should_damage();
    return true;
}

void minimap_view::render()
{
    if (Renderer::instance()->is_terminal()) {
        render_terminal();
        return;
    }

    layout_item_ptr lo = layout();

    app_t* app = app_t::instance();
    view_style_t vs = style;

    Renderer::instance()->draw_rect({ lo->render_rect.x, lo->render_rect.y, lo->render_rect.w, lo->render_rect.h },
        { (uint8_t)vs.bg.red, (uint8_t)vs.bg.green, (uint8_t)vs.bg.blue }, true);

    editor_view* ev = (editor_view*)(parent->parent);
    editor_ptr editor = ev->editor;
    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();

    block_ptr current_block = cursor.block();

    int start = ev->start_row;
    int count = doc->blocks.size() + (ev->rows / 3);
    if (count <= 0)
        return;

    float p = (float)start / count;
    scroll_y = 0;

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

        if (!block->data || block->data->dirty) {
            hl.push_back(block);
        }

        blockdata_ptr blockData = block->data;

        uint8_t alpha = 80;
        if (block == current_block) {
            alpha = 250;
        }

        if (!blockData) {
            RenRect r = {
                lo->render_rect.x,
                lo->render_rect.y + l,
                (int)(block->length() * DRAW_SCALE),
                1,
            };
            if (r.width > 0) {
                Renderer::instance()->draw_rect(r,
                    { 255, 255, 255, 150 },
                    false, 1);
            }
            blockData = 0;
        }

        if (blockData) {
            for (auto s : blockData->spans) {

                color_info_t clr = Renderer::instance()->color_for_index(s.colorIndex);

                int start = s.start / 3;
                int length = s.length * DRAW_SCALE;
                RenRect r = {
                    lo->render_rect.x + start + 2,
                    lo->render_rect.y + l,
                    length,
                    1,
                };

                if (render_h < l) {
                    render_h = l;
                }

                if (start + r.width + 2 > lo->render_rect.w) {
                    r.width = lo->render_rect.w - (start + 2);
                }

                if (r.width > 0) {
                    Renderer::instance()->draw_rect(r,
                        { (uint8_t)clr.red, (uint8_t)clr.green, (uint8_t)clr.blue, (uint8_t)alpha },
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

    if (is_hovered()) {
        Renderer::instance()->draw_rect(
            {
                lo->render_rect.x,
                sliding_y,
                lo->render_rect.w,
                ev->rows * spacing,
            },
            { 255, 255, 255, 10 }, true, 1, 4);
    }

    Renderer::instance()->draw_rect(
        {
            lo->render_rect.x,
            render_current_y - spacing,
            lo->render_rect.w,
            1,
        },
        { 255, 255, 255, 40 }, true, 1, 0);
}

void minimap_view::buildUpDotsForBlock(block_ptr block, float textCompress, int bufferWidth)
{
    if (!block->data) {
        block->data = std::make_shared<blockdata_t>();
        block->data->dirty = true;
    }

    blockdata_ptr blockData = block->data;
    if (!blockData || blockData->dots) {
        return;
    }

    // log("minimap %d", block->lineNumber);

    std::string line1;
    std::string line2;
    std::string line3;
    std::string line4;

    block_ptr it = block;
    line1 = it->text();
    if (it->next()) {
        it = it->next();
        line2 = it->text();
        if (it->next()) {
            it = it->next();
            line3 = it->text();
            if (it->next()) {
                it = it->next();
                line4 = it->text();
            }
        }
    }

    blockData->dots = buildUpDotsForLines(
        (char*)line1.c_str(),
        (char*)line2.c_str(),
        (char*)line3.c_str(),
        (char*)line4.c_str(),
        textCompress,
        bufferWidth);
}

void minimap_view::render_terminal()
{
    layout_item_ptr lo = layout();

    app_t* app = app_t::instance();
    view_style_t vs = style;

    editor_view* ev = (editor_view*)(parent->parent);
    editor_ptr editor = ev->editor;
    document_t* doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_t& block = *cursor.block();

    int start = ev->start_row;
    int count = editor->document.blocks.size() + (ev->rows / 3);
    if (count <= 0)
        return;

    int window = ev->rows;

    float p = (float)start / count;
    scroll_y = 0;

    if (editor->document.blocks.size() * spacing > lo->render_rect.h) {
        scroll_y = (p * editor->document.blocks.size() * spacing);
        scroll_y -= (p * lo->render_rect.h * 3 / 4);
    }
    if (scroll_y < 0) {
        scroll_y = 0;
    }

    int sy = (scroll_y / spacing);
    int y = 0;
    for (int idx = sy; idx < doc->blocks.size(); idx += 4) {
        auto& b = doc->blocks[idx];

        if (y == 0) {
            start_row = b->lineNumber;
        }
        end_row = b->lineNumber;

        buildUpDotsForBlock(b, TEXT_COMPRESS, TEXT_BUFFER);

        int textCompress = TEXT_COMPRESS;
        block_list& snapBlocks = editor->snapshots[0].snapshot;

        int ci = vs.bg.index;

        if (b->lineNumber >= start && b->lineNumber < start + window) {
            ci = vs.fg.index;
        }

        for (int x = 0; x < TEXT_BUFFER; x++) {
            if (b->data && b->data->dots) {
                Renderer::instance()->draw_wtext(NULL, wcharFromDots(b->data->dots[x]),
                    lo->render_rect.x + x,
                    lo->render_rect.y + y,
                    { 255, 255, 255, (uint8_t)ci });
            }

            if (x >= lo->render_rect.w - 2) {
                break;
            }
        }

        y++;
        if (y >= lo->render_rect.h) {
            break;
        }
    }
}

void minimap_view::prerender()
{
    view_item::prerender();
    if (prev_scroll_y != scroll_y || render_y != sliding_y || prev_start_row != start_row || prev_end_row != end_row) {
        prev_start_row = start_row;
        prev_end_row = end_row;
        prev_scroll_y = scroll_y;
        if (render_y != sliding_y && is_hovered()) {
            Renderer::instance()->throttle_up_events(240);
        }
        damage();
    }
}
