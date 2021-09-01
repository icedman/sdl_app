#include "editor_view.h"
#include "scrollbar.h"

#include "app.h"
#include "renderer.h"
#include "render_cache.h"
#include "view.h"

extern std::map<int, color_info_t> colorMap;

static void render_editor(editor_view *ev)
{
    state_save();

    layout_item_ptr lo = ev->layout();
    scrollbar_view *sv = (scrollbar_view*)ev->_views[1].get();

    // printf(">>%d\n", sv->index);

    int fw, fh;
    ren_get_font_extents(NULL, &fw, &fh, NULL, 1, true);
    int cols = (lo->render_rect.w / fw);
    int rows = (lo->render_rect.h / fh) + 1;

    set_clip_rect({
        lo->render_rect.x,
        lo->render_rect.y,
        lo->render_rect.w - 18,
        lo->render_rect.h
    });

    ev->rows = rows;
    ev->cols = cols;
    editor_ptr editor = app_t::instance()->currentEditor;

    int start = ev->start_row;
    if (start < 0) {
        start = 0;
    }
    if (start >= editor->document.blocks.size() - (rows/2)) {
        start = editor->document.blocks.size() - (1 + rows/2);
    }
    // sv->set_index(start);
    // sv->set_size(editor->document.blocks.size() - (rows/2), rows);
    ev->start_row = start;

    document_t *doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_ptr block = doc->blockAtLine(start);
    cursor_list cursors = doc->cursors;
    cursor_t mainCursor = doc->cursor();

    bool hlMainCursor = cursors.size() == 1 && !mainCursor.hasSelection();

    block_list::iterator it = doc->blocks.begin();
    it += start;

    int view_height = rows;
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
                    color_info_t cur = sel;
                    if (hlMainCursor) {
                        int cursor_pad = 4;
                        cr.width = 2;
                        cr.y -= cursor_pad;
                        cr.height += cursor_pad * 2;
                        cur = clr;
                    }
                    draw_rect(cr, { (int)cur.red, (int)cur.green, (int)cur.blue, 125 }, true, 1.0f);
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

    state_restore();
}

editor_view::editor_view() 
        : view_item("editor")
        , start_row(0)
{
    layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
    layout()->fit_children = false;

    vscroll = std::make_shared<scrollbar_view>();
    vscroll->layout()->width = 18;
    can_focus = true;

    view_item_ptr spacer = std::make_shared<view_item>();
    add_child(spacer);
    add_child(vscroll);

    view_set_focused(this);
}

void editor_view::render()
{
    render_editor(this);
}

void editor_view::update()
{    
    scrollbar_view *sv = (scrollbar_view*)_views[1].get();

    editor_ptr editor = app_t::instance()->currentEditor;
    int _start = start_row;
    sv->set_index(start_row);
    sv->set_size(editor->document.blocks.size() - (rows/2), rows);
    start_row = _start;

    if (editor->document.columns != cols || editor->document.rows != rows) {
        editor->document.setColumns(cols);
        editor->document.setRows(rows);
        layout_request();
    }
}

bool editor_view::mouse_wheel(int x, int y)
{
    scrollbar_view *scrollbar =  ((scrollbar_view*)vscroll.get());
    start_row -= y;
    rencache_invalidate();
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

    block_list::iterator it = editor->document.blocks.begin();
    it += start_row;

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

bool editor_view::on_scroll()
{
    int idx = ((scrollbar_view*)vscroll.get())->index;
    if (scrollbar_index != idx) {
        scrollbar_index = idx;
        rencache_invalidate();
        start_row = idx;
    }
    return true;
}

bool editor_view::input_key(int k) {
    ensure_visible_cursor();
    return true;
}

bool editor_view::input_text(std::string text) {
    editor_ptr editor = app_t::instance()->currentEditor;
    editor->pushOp(INSERT, text);
    editor->runAllOps();
    ensure_visible_cursor();
    // printf("text\n");
    return true;
}

bool editor_view::input_sequence(std::string text) {
    editor_ptr editor = app_t::instance()->currentEditor;
    editor->input(-1, text);
    editor->runAllOps();
    ensure_visible_cursor();
    // printf("sequence %s\n", text.c_str());
    return true;
}

void editor_view::scroll_to_cursor(cursor_t c, bool animate, bool centered)
{
    block_ptr block = c.block();
    int l = block->lineNumber - 1;

    int cols = block->document->columns;
    int rows = block->document->rows;
    if (start_row > l) {
        start_row = l;
    }
    if (start_row + rows - 4 < l) {
        start_row = l - (rows - 4);
    }

    // printf("(%d %d) %d - %d : %d\n", cols, rows, start_row, start_row + block->document->rows, l);
}

void editor_view::ensure_visible_cursor(bool animate)
{
    layout_item_ptr lo = layout();

    editor_ptr editor = app_t::instance()->currentEditor;
    document_t *doc = &editor->document;

    int fw, fh;
    ren_get_font_extents(NULL, &fw, &fh, NULL, 1, true);

    int cols = lo->render_rect.w / fw;
    int rows = lo->render_rect.h / fh;
    doc->setColumns(cols);
    doc->setRows(rows);
    
    cursor_t mainCursor = doc->cursor();
    scroll_to_cursor(mainCursor);
}