#include "editor_view.h"
#include "scrollbar.h"

#include "app.h"
#include "renderer.h"
#include "render_cache.h"
#include "view.h"

extern std::map<int, color_info_t> colorMap;

// TODO move highlight out of render
void editor_view::render()
{
    if (!editor) {
        return;
    }

    RenFont *_font = ren_font((char*)font.c_str());

    state_save();

    scrollarea_view *area = view_item::cast<scrollarea_view>(scrollarea);
    layout_item_ptr alo = area->layout();
    layout_item_ptr lo = content()->layout();

    int fw, fh;
    ren_get_font_extents(_font, &fw, &fh, NULL, 1, true);
    cols = (area->layout()->render_rect.w / fw);
    rows = (area->layout()->render_rect.h / fh) + 1;

    set_clip_rect({
        alo->render_rect.x,
        alo->render_rect.y,
        alo->render_rect.w,
        alo->render_rect.h
    });

    int start = (-area->layout()->scroll_y / fh) ;
    // printf(">?%d\n", start);
    if (start < 0) {
        start = 0;
    }
    if (start >= editor->document.blocks.size() - (rows/2)) {
        start = editor->document.blocks.size() - (1 + rows/2);
    }
    start_row = start;

    document_t *doc = &editor->document;
    cursor_t cursor = doc->cursor();
    block_ptr block = doc->blockAtLine(start);
    cursor_list cursors = doc->cursors;
    cursor_t mainCursor = doc->cursor();

    bool hlMainCursor = cursors.size() == 1 && !mainCursor.hasSelection();

    block_list::iterator it = doc->blocks.begin();
    if (start >= doc->blocks.size()) {
        start = (doc->blocks.size() - 1);
    }
    it += start;

    // printf(">>%d %d\n", start_row, rows);

    int view_height = rows;
    int hl_prior = view_height/4;
    int hl_start = start - hl_prior;
    int hl_length = view_height + hl_prior;
    editor->highlight(hl_start, hl_length);

    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;

    color_info_t fg = colorMap[app_t::instance()->fg];
    color_info_t sel = colorMap[app_t::instance()->selBg];

    bool has_focus = is_focused();

    int l=0;
    while(it != doc->blocks.end() && l<hl_length) {
        block_ptr block = *it++;

        struct blockdata_t* blockData;
        if (block->data) {
            blockData = block->data.get();
        }
        if (!blockData) {
            // draw_text(NULL, (char*)block->text().c_str(), 
            //     lo->render_rect.x,
            //     lo->render_rect.y + (l*fh), 
            //     { (uint8_t)fg.red,(uint8_t)fg.green,(uint8_t)fg.blue },
            //     false, false, true);

            // l++;
            // continue;
            return;
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

                if (hl && has_focus) {
                    RenRect cr = {
                        alo->render_rect.x + (pos * fw),
                        alo->render_rect.y + (l * fh),
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
                    draw_rect(cr, { (uint8_t)cur.red, (uint8_t)cur.green, (uint8_t)cur.blue, 125 }, true, 1.0f);
                    if (ul) {
                        cr.y += fh - 2;
                        cr.height = 1;
                        draw_rect(cr, { (uint8_t)clr.red, (uint8_t)clr.green, (uint8_t)clr.blue }, true, 1.0f);
                    }
                }
            }

            s.x = alo->render_rect.x + (s.start * fw);
            s.y = alo->render_rect.y + (l*fh);
            
            draw_rect({
                s.x,
                s.y,
                fw * s.length,
                fh
            }, { (uint8_t)clr.red, (uint8_t)clr.green, (uint8_t)clr.blue, 125 }, false, 1.0f);
    
            draw_text(_font, (char*)span_text.c_str(), 
                s.x,
                s.y, 
                { (uint8_t)clr.red,(uint8_t)clr.green,(uint8_t)clr.blue },
                s.bold, s.italic, true);
        }


        l++;
    }

    state_restore();
}

editor_view::editor_view() 
        : panel_view()
        , start_row(0)
{
    type = "editor";
    font = "editor";
    interactive = true;
    focusable = true;
    view_set_focused(this);

    scrollarea->disabled = true;

    gutter = std::make_shared<view_item>("gutter");
    gutter->layout()->width = 40;
    minimap = std::make_shared<view_item>("minimap");
    minimap->layout()->width = 80;

    view_item* container = (view_item*)scrollarea->parent;

    container->add_child(gutter);
    container->add_child(minimap);

    scrollarea->layout()->order = 2;
    v_scroll->layout()->order = 4;
    gutter->layout()->order = 1;
    minimap->layout()->order = 3;
    
    layout_sort(container->layout());

    on(EVT_MOUSE_DOWN, [this](event_t& evt) {
        evt.cancelled = true;
        return this->mouse_down(evt.x, evt.y, evt.button, evt.clicks);
    });
    on(EVT_MOUSE_UP, [this](event_t& evt) {
        evt.cancelled = true;
        return this->mouse_up(evt.x, evt.y, evt.button);
    });
    on(EVT_MOUSE_MOTION, [this](event_t& evt) {
        evt.cancelled = true;
        return this->mouse_move(evt.x, evt.y, evt.button);
    });
    on(EVT_KEY_DOWN, [this](event_t& evt) {
        evt.cancelled = true;
        return this->input_key(evt.key);
    });
    on(EVT_KEY_TEXT, [this](event_t& evt) {
        evt.cancelled = true;
        return this->input_text(evt.text);
    });
    on(EVT_KEY_SEQUENCE, [this](event_t& evt) {
        evt.cancelled = true;
        return this->input_sequence(evt.text);
    });
}

void editor_view::update()
{
    if (!editor) {
        return;
    }

    if (editor->document.columns != cols || editor->document.rows != rows) {
        editor->document.setColumns(cols);
        editor->document.setRows(rows);
        layout_request();
    }

    if (is_focused()) {
        app_t::instance()->currentEditor = editor;
    }

    view_item::update();
}

void editor_view::prelayout()
{
    scrollarea_view *area = view_item::cast<scrollarea_view>(scrollarea);
    // scrollbar_view *vs = view_item::cast<scrollbar_view>(v_scroll);
    // scrollbar_view *hs = view_item::cast<scrollbar_view>(h_scroll);

    int fw, fh;
    ren_get_font_extents(ren_font((char*)font.c_str()), &fw, &fh, NULL, 1, true);
    
    int count = editor->document.blocks.size();

    content()->layout()->width = area->layout()->render_rect.w;
    content()->layout()->height = (count + rows/4) * fh;
}

bool editor_view::mouse_down(int x, int y, int button, int clicks)
{
    if (!editor) {
        return false;
    }

    mouse_x = x;
    mouse_y = y;

    layout_item_ptr lo = content()->layout();

    cursor_t cursor = editor->document.cursor();

    if (start_row >= editor->document.blocks.size()) {
        start_row = editor->document.blocks.size() -  1;
    }

    block_list::iterator it = editor->document.blocks.begin();
    it += start_row;

    int fw, fh;
    ren_get_font_extents(ren_font((char*)font.c_str()), &fw, &fh, NULL, 1, true);

    int l = 0;
    while (it != editor->document.blocks.end()) {
        block_ptr block = *it++;
        if (!block->data) {
            break;
        }

        struct blockdata_t* blockData = block->data.get();
        std::string text = block->text() + "\n";
        const char *line = text.c_str();

        bool hitLine = false;
        bool hitSpan = false;
        int hitPos = 0;
        for(auto &s : blockData->spans) {
            layout_rect r = {
                s.x, 
                s.y,
                s.length * fw,
                fh
            };
            if (y > r.y && y <= r.y + r.h) {
                hitLine = true;
                int pos = (x - s.x) / fw;
                hitPos = pos + s.start;
                if (x > r.x && x <= r.x + r.w) {
                    // std::string span_text = text.substr(s.start, s.length);
                    break;
                } else {
                    hitPos = pos + s.start + s.length;
                }
            }
        }

        if (!hitSpan && hitLine) {
            hitSpan = true;
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

bool editor_view::input_key(int k) {
    ensure_visible_cursor();
    return true;
}

bool editor_view::input_text(std::string text) {
    if (!editor) {
        return false;
    }
    editor->pushOp(INSERT, text);
    editor->runAllOps();
    ensure_visible_cursor();
    return true;
}

bool editor_view::input_sequence(std::string text) {
    if (!editor) {
        return false;
    }
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

    int fw, fh;
    ren_get_font_extents(ren_font((char*)font.c_str()), &fw, &fh, NULL, 1, true);

    scrollarea_view *area = view_item::cast<scrollarea_view>(scrollarea);
    area->layout()->scroll_y = -start_row * fh;

    update_scrollbars();
}

void editor_view::ensure_visible_cursor(bool animate)
{
    scrollarea_view *area = view_item::cast<scrollarea_view>(scrollarea);
    layout_item_ptr lo = area->layout();

    document_t *doc = &editor->document;

    int fw, fh;
    ren_get_font_extents(ren_font((char*)font.c_str()), &fw, &fh, NULL, 1, true);

    int cols = lo->render_rect.w / fw;
    int rows = lo->render_rect.h / fh;
    doc->setColumns(cols);
    doc->setRows(rows);
    
    cursor_t mainCursor = doc->cursor();
    scroll_to_cursor(mainCursor);
}