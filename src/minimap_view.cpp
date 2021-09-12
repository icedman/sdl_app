#include "minimap_view.h"
#include "editor_view.h"
#include "editor.h"
#include "render_cache.h"
#include "renderer.h"
#include "events.h"

extern std::map<int, color_info_t> colorMap;

minimap_view::minimap_view()
	: view_item("minimap")
{
    interactive = true;

    spacing = 2;

    on(EVT_MOUSE_DRAG, [this](event_t e) {
        e.cancelled = true;
        return this->mouse_click(e.x, e.y, e.button);
    });

    on(EVT_MOUSE_CLICK, [this](event_t e) {
        e.cancelled = true;
        return this->mouse_click(e.x, e.y, e.button);
    });
}

bool minimap_view::mouse_click(int x, int y, int button)
{
    layout_item_ptr lo = layout();
    int ry = y - lo->render_rect.y;
    if (ry > end_y * spacing) return true;

    int line = (end_y + start_y) * (float)ry / render_h;

    editor_view* ev = (editor_view*)(parent->parent);    
    editor_ptr editor = ev->editor;

    if (line > editor->document.blocks.size() / 2) {
        line += ev->rows/3;
    } else {
        line -= ev->rows/3;
    }
    if (line < 0) {
        line = 0;
    }
    if (line >= editor->document.blocks.size()) {
        line = editor->document.blocks.size()-1;
    }

    // printf(">>%d\n", line);

    cursor_t cursor;
    cursor.setPosition(editor->document.blockAtLine(line), 0);
    ev->scroll_to_cursor(cursor);

    return true;
}

void minimap_view::render()
{
    layout_item_ptr lo = layout();

    editor_view* ev = (editor_view*)(parent->parent);    
    editor_ptr editor = ev->editor;

    int start = ev->start_row;
    int count = editor->document.blocks.size() + (ev->rows/3);
    if (count <= 0) return;

    float p = (float)start / count;
    scroll_y = 0;

    if (editor->document.blocks.size() * spacing > lo->render_rect.h) {
        scroll_y = (p * editor->document.blocks.size() * spacing);
        scroll_y -= (p * lo->render_rect.h * 3 / 4);
    }
    if (scroll_y < 0) {
        scroll_y = 0;
    }

    // printf("%f\n", p);

    if (is_hovered()) {
        ren_draw_rect({
            lo->render_rect.x,
            lo->render_rect.y + (p * editor->document.blocks.size() * spacing) - scroll_y,
            lo->render_rect.w,
            ev->rows * spacing,
        },
            { 255, 255, 255, 10 },
            true, 1, 4
        );
    }

    int hl = 0;

    block_list& snapBlocks = editor->snapshots[0].snapshot;

    block_list::iterator it = editor->document.blocks.begin();
    it += (scroll_y / spacing);

    // printf(">%f %d\n", p, scroll_y);

    start_y = -1;
    end_y = 0;
    render_h = 0;

    int l=0;
    while(it != editor->document.blocks.end() && l < lo->render_rect.h) {
        block_ptr block = *it;
        it ++;

        blockdata_t *blockData = block->data.get();

        // if (!blockData && block->lineNumber < snapBlocks.size()) {
        //     block_ptr sblock = snapBlocks[block->lineNumber];
        //     blockData = sblock->data.get();
        //     if (blockData && blockData->dirty) {
        //         blockData = 0;
        //     }
        // }

        if (!blockData) {
            editor->highlight(block->lineNumber, 8);
            if (hl++>2) break;
            blockData = block->data.get();
            ren_listen_quick();
        }

        if (!blockData) break;

        for(auto s : blockData->spans) {

            color_info_t clr = colorMap[s.colorIndex];

            int start = s.start / 3;
            int length = s.length / 2;
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
                ren_draw_rect(r,
                    { clr.red, clr.green, clr.blue, 150 },
                    false, 1
                );
            }
        }

        if (start_y == -1) {
            start_y = block->lineNumber;
        }
        end_y = block->lineNumber;
        

        l+=spacing;
    }
}