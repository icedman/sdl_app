#include "editor.h"
#include "scrollbar.h"

#include "app.h"
#include "renderer.h"

editor_view::editor_view() 
        : view_item("editor")
        , start(0)
{
    layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
    vscroll = std::make_shared<scrollbar_view>();
    vscroll->layout()->width = 24;

    can_scroll = true;

    view_item_ptr spacer = std::make_shared<view_item>();
    add_child(spacer);
    add_child(vscroll);
}

bool editor_view::mouse_wheel(int x, int y)
{
    scrollbar_view *scrollbar =  ((scrollbar_view*)vscroll.get());
    start -= y;
    return true;
}

bool editor_view::on_scroll()
{
    start = ((scrollbar_view*)vscroll.get())->index;
    return true;
}

bool editor_view::mouse_down(int x, int y, int button, int clicks)
{
    mouse_x = x;
    mouse_y = y;

    layout_item_ptr lo = layout();
    editor_ptr editor = app_t::instance()->currentEditor;

    x -= lo->render_rect.x;
    y -= lo->render_rect.y;

    // printf("%d %d\n", x, y);

    cursor_t cursor = editor->document.cursor();
    block_ptr block = editor->document.blockAtLine(start);

    block_list::iterator it = editor->document.blocks.begin();
    it += start;

    int fw, fh;
    ren_get_font_extents(NULL, &fw, &fh, NULL, 1, true);

    int l = 0;
    while (it != editor->document.blocks.end()) {
        block_ptr block = *it++;
        if (!block->data) {
            break;
        }

        struct blockdata_t* blockData = block->data.get();
        std::string text = block->text() + "\n";
        const char *line = text.c_str();

        bool hitSpan = false;
        int hitPos = 0;
        for(auto &s : blockData->spans) {
            layout_rect r = {
                s.start * fw, l * fh, s.length * fw, fh
            };
            if ((x > r.x && x <= r.x + r.w) &&
                (y > r.y && y <= r.y + r.h)) {

                int pos = (x - (s.start * fw)) / fw;

                // std::string span_text = text.substr(s.start, s.length);
                // printf("%s %d\n", span_text.c_str(), pos);

                hitSpan = true;
                hitPos = pos + s.start;
                break;
            }
        }

        if (!hitSpan) {
            if (y >= l * fh && y < (l + 1) * fh) {
                hitSpan = true;
                hitPos = text.length()-1;
            }
        }

        if (hitSpan) {
            std::ostringstream ss;
            ss << (block->lineNumber + 1);
            ss << ":";
            ss << hitPos;
            int mods = view_input_key_mods();
            if (clicks == 0 || (mods == 1073742049)) {
                editor->pushOp(MOVE_CURSOR_ANCHORED, ss.str());
            } else if (clicks == 1) {
                editor->pushOp(MOVE_CURSOR, ss.str());
            } else {
                editor->pushOp(MOVE_CURSOR, ss.str());
                editor->pushOp(SELECT_WORD, "");
            }
            return true;
        }

        l++;
    }
    return true;
}

bool editor_view::mouse_up(int x, int y, int button)
{
    return true;
}

bool editor_view::mouse_click(int x, int y, int button)
{
    return true;
}

bool editor_view::mouse_move(int x, int y, int button)
{
    if (button) {
        int dx = mouse_x - x;
        int dy = mouse_y - y;
        int drag_distance = dx * dx + dy *dy;
        if (drag_distance >= 4) {
            mouse_down(x, y, 0, 0);
        }
    }
    return true;
}
