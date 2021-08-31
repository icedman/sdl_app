#include "editor.h"
#include "scrollbar.h"

#include "app.h"
#include "renderer.h"

extern std::map<int, color_info_t> colorMap;

static void render_editor(editor_view *ev)
{
    layout_item_ptr lo = ev->layout();
    scrollbar_view *sv = (scrollbar_view*)ev->_views[1].get();

    // printf(">>%d\n", sv->index);

    int fw, fh;
    ren_get_font_extents(NULL, &fw, &fh, NULL, 1, true);

    editor_ptr editor = app_t::instance()->currentEditor;

    int start = ev->start;
    if (start < 0) {
        start = 0;
    }
    if (start >= editor->document.blocks.size() - (38/2)) {
        start = editor->document.blocks.size() - (1 + 38/2);
    }
    sv->set_index(start);
    sv->set_size(editor->document.blocks.size() - (38/2), 38);
    ev->start = start;

    document_t *doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_ptr block = doc->blockAtLine(start);
    cursor_list cursors = doc->cursors;
    cursor_t mainCursor = doc->cursor();

    bool hlMainCursor = cursors.size() == 1 && !mainCursor.hasSelection();

    block_list::iterator it = doc->blocks.begin();
    it += start;

    int view_height = 38;
    int hl_prior = view_height/4;
    int hl_start = start - hl_prior;
    int hl_length = view_height + hl_prior;
    editor->highlight(hl_start, hl_length);

    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;

    color_info_t fg = colorMap[app_t::instance()->fg];
    color_info_t sel = colorMap[app_t::instance()->selBg];

    int l=0;
    while(it != doc->blocks.end() && l<view_height) {
        block_ptr block = *it++;
        
        struct blockdata_t* blockData;
        if (block->data) {
            blockData = block->data.get();
        }
        if (!blockData) {
            draw_text(NULL, (char*)block->text().c_str(), 
                lo->render_rect.x,
                lo->render_rect.y + (l*fh), 
                { (int)fg.red,(int)fg.green,(int)fg.blue },
                false, false, true);

            l++;
            continue;
        }

        std::string text = block->text() + "\n";
        const char *line = text.c_str();

        for(auto &s : blockData->spans) {
            color_info_t clr = colorMap[s.colorIndex];

            std::string span_text = text.substr(s.start, s.length);

            // cursors
            for (int pos = s.start; pos < s.start+s.length; pos++) {
                bool hl = false;
                bool ul = false;
                for (auto& c : cursors) {
                    if (pos == c.position() && block == c.block()) {
                        hl = true;
                        ul = c.hasSelection();
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

                if (hl) {
                    RenRect cr = {
                        lo->render_rect.x + (pos * fw),
                        lo->render_rect.y + (l * fh),
                        fw, fh
                    };
                    draw_rect(cr, { (int)sel.red, (int)sel.green, (int)sel.blue, 125 }, true, 1.0f);
                    if (ul) {
                        cr.y += fh - 2;
                        cr.height = 1;
                        draw_rect(cr, { (int)clr.red, (int)clr.green, (int)clr.blue }, true, 1.0f);
                    }
                }
            }

            draw_text(NULL, (char*)span_text.c_str(), 
                lo->render_rect.x + (s.start * fw),
                lo->render_rect.y + (l*fh), 
                { (int)clr.red,(int)clr.green,(int)clr.blue },
                false, false, true);
        }


        l++;
    }

    // char tmp[32];
    // sprintf(tmp, "%d %d\n", cursor.block()->lineNumber, cursor.position());
    // draw_text(NULL, tmp, 
    //     item->render_rect.x,
    //     item->render_rect.y, 
    //     { 255,255,255 },
    //     false, false, true);
}

editor_view::editor_view() 
        : view_item("editor")
        , start(0)
{
    layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
    vscroll = std::make_shared<scrollbar_view>();
    vscroll->layout()->width = 18;

    can_scroll = true;

    view_item_ptr spacer = std::make_shared<view_item>();
    add_child(spacer);
    add_child(vscroll);
}

void editor_view::render()
{
    render_editor(this);
}

void editor_view::precalculate()
{
    scrollbar_view *sv = (scrollbar_view*)_views[1].get();


    editor_ptr editor = app_t::instance()->currentEditor;
    int _start = start;
    sv->set_index(start);
    sv->set_size(editor->document.blocks.size() - (38/2), 38);
    start = _start;
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
            if (clicks == 0 || mods & K_MOD_SHIFT) {
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
        if (l > 38) break;
    }
    return true;
}

bool editor_view::mouse_up(int x, int y, int button)
{
    return true;
}

bool editor_view::mouse_move(int x, int y, int button)
{
    if (button && is_pressed()) {
        int dx = mouse_x - x;
        int dy = mouse_y - y;
        int drag_distance = dx * dx + dy *dy;
        if (drag_distance >= 100) {
            mouse_down(x, y, 0, 0);
        }
    }
    return true;
}
