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

    on(EVT_MOUSE_CLICK, [](event_t e) {
        printf("clicked!\n");
        return true;
    });
}

void minimap_view::postlayout()
{
	editor_view* ev = (editor_view*)(parent->parent);
    scrollbar_view *vs = view_item::cast<scrollbar_view>(ev->v_scroll);
	editor_ptr editor = ev->editor;
}

void minimap_view::render()
{
    layout_item_ptr lo = layout();

    state_save();

    set_clip_rect({ lo->render_rect.x,
        lo->render_rect.y,
        lo->render_rect.w,
        lo->render_rect.h });

    editor_view* ev = (editor_view*)(parent->parent);    
    editor_ptr editor = ev->editor;

    if (is_hovered()) {
        ren_draw_rect({
            lo->render_rect.x,
            lo->render_rect.y,
            lo->render_rect.w,
            ev->rows * 3,
        },
            { 255, 255, 255, 150 },
            false, 1
        );
    }

    int hl = 0;

    block_list& snapBlocks = editor->snapshots[0].snapshot;

    block_list::iterator it = editor->document.blocks.begin();
    int l=0;
    while(it != editor->document.blocks.end() && l < lo->render_rect.h) {
        block_ptr block = *it;
        it ++;

        blockdata_t *blockData = block->data.get();

        if (!blockData && block->lineNumber < snapBlocks.size()) {
            block_ptr sblock = snapBlocks[block->lineNumber];
            blockData = sblock->data.get();
        }

        if (!blockData) {
            editor->highlight(block->lineNumber, 4);
            if (hl++>4) break;
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

        l+=2;
    }

    state_restore();
}