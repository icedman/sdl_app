#include "editor_view.h"
#include "completer_view.h"
#include "gutter_view.h"
#include "minimap_view.h"

#include "app.h"
#include "indexer.h"
#include "renderer.h"
#include "search.h"
#include "style.h"
#include "view.h"

#include "scrollbar.h"
#include <algorithm>
#include <set>

static std::vector<span_info_t> split_span(span_info_t si, const std::string& str)
{
    static std::set<char> delimiters = {
        '.', ',', ';', ':',
        '-', '+', '*', '/', '%', '=',
        '"', ' ', '\'', '\\',
        '(', ')', '[', ']', '<', '>',
        '&', '!', '?', '_', '~', '@'
    };

    std::vector<span_info_t> result;

    char const* line = str.c_str();
    char const* pch = str.c_str();
    char const* start = pch;
    for (; *pch; ++pch) {
        if (delimiters.find(*pch) != delimiters.end()) {
            span_info_t s = si;
            s.start = start - line;
            s.length = pch - start;
            result.push_back(s);
            start = pch;

            // std::string t(line + s.start, s.length);
            // printf(">%d %d >%s<\n", s.start, s.length, t.c_str());
        }
    }

    span_info_t s = si;
    s.start = start - line;
    s.length = pch - start;
    if (start == pch) {
        s.length = 1;
    }
    result.push_back(s);

    return result;
}

bool compareSpan(span_info_t a, span_info_t b)
{
    return a.start < b.start;
}

void editor_view::render()
{
    if (!editor) {
        return;
    }

    RenFont* _font = Renderer::instance()->font((char*)font.c_str());

    app_t* app = app_t::instance();
    view_style_t vs = style;

    bool wrap = app->lineWrap;
    int indent = app->tabSize;

    layout_item_ptr plo = layout();

    if (Renderer::instance()->is_terminal()) {
        // do something else
    } else {
        Renderer::instance()->draw_rect({ plo->render_rect.x,
                                            plo->render_rect.y,
                                            plo->render_rect.w,
                                            plo->render_rect.h },
            { (uint8_t)vs.bg.red, (uint8_t)vs.bg.green, (uint8_t)vs.bg.blue }, true);
    }

    Renderer::instance()->state_save();

    // for(auto r: previous_cursor_rects) {
    //     Renderer::instance()->draw_rect(r, { 255,0,0 }, false, 2);
    // }
    previous_cursor_rects.clear();

    scrollarea_view* area = view_item::cast<scrollarea_view>(scrollarea);
    layout_item_ptr alo = area->layout();
    layout_item_ptr lo = content()->layout();

    int fw, fh;
    Renderer::instance()->get_font_extents(_font, &fw, &fh, NULL, 1);
    cols = (area->layout()->render_rect.w / fw);
    rows = (area->layout()->render_rect.h / fh) + 1;

    Renderer::instance()->set_clip_rect({ alo->render_rect.x,
        alo->render_rect.y,
        alo->render_rect.w,
        alo->render_rect.h });

    int start = (-area->layout()->scroll_y / fh);
    // printf(">?%d\n", start);

    if (start >= editor->document.blocks.size() - (rows / 2)) {
        start = editor->document.blocks.size() - (1 + rows / 2);
    }
    if (start < 0) {
        start = 0;
    }

    start_row = start;

    document_t* doc = &editor->document;
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

    theme_ptr theme = app->theme;

    color_info_t fg = Renderer::instance()->color_for_index(app_t::instance()->fg);
    color_info_t sel = Renderer::instance()->color_for_index(app_t::instance()->selBg);

    bool has_focus = is_focused();

    block_ptr prev_longest = longest_block;
    if (longest_block && !longest_block->isValid()) {
        longest_block = 0;
    }

    int l = 0;
    while (it != doc->blocks.end() && l < rows) {
        block_ptr block = *it++;

        blockdata_ptr blockData;
        if (!block->data || block->data->dirty) {
            editor->highlighter.highlightBlock(block);
            editor->highlighter.resume();
        }
        if (block->data) {
            blockData = block->data;
        }

        if (!blockData || blockData->dirty) {
            blockData = std::make_shared<blockdata_t>();
            span_info_t span = {
                start : 0,
                length : (int)block->text().length(),
                colorIndex : app_t::instance()->fg,
                bold : false,
                italic : false,
                state : BLOCK_STATE_UNKNOWN,
                scope : ""
            };
            blockData->spans.clear();
            blockData->spans.push_back(span);
            // break;
        }

        if (!blockData) {
            break;
        }

        if (!longest_block) {
            longest_block = block;
        }
        if (longest_block->length() < block->length()) {
            longest_block = block;
        }

        std::string text = block->text() + " \n";
        const char* line = text.c_str();

        std::wstring wtext = block->wide_text() + L" \n";
        const wchar_t* wline = wtext.c_str();

        // wrap
        blockData->rendered_spans = blockData->spans;
        if (wrap && text.length() > cols) {
            blockData->rendered_spans.clear();
            for (auto& s : blockData->spans) {
                if (s.length == 0)
                    continue;
                std::string span_text = text.substr(s.start, s.length);
                std::vector<span_info_t> ss = split_span(s, span_text);
                for (auto _s : ss) {
                    _s.start += s.start;
                    blockData->rendered_spans.push_back(_s);
                }
            }

            std::sort(blockData->rendered_spans.begin(), blockData->rendered_spans.end(), compareSpan);

            int line = 0;
            int line_x = 0;
            for (auto& _s : blockData->rendered_spans) {
                if (_s.start - (line * cols) + _s.length > cols) {
                    line++;
                    line_x = 0;
                }
                _s.line = line;
                if (_s.line > 0) {
                    _s.line_x = indent + line_x;
                    line_x += _s.length;
                }

                std::string span_text = text.substr(_s.start, _s.length);
            }
        }

        block->lineCount = 1;
        block->lineHeight = fh;
        int linc = 0;
        for (auto& s : blockData->rendered_spans) {
            if (s.length == 0)
                continue;

            color_info_t clr = Renderer::instance()->color_for_index(s.colorIndex);

            if (s.start + s.length >= wtext.length()) {
                s.length = wtext.length() - s.start;
                if (s.length <= 0) {
                    s.length = 0;
                    continue;
                }
            }

            std::string span_text = text.substr(s.start, s.length);
            std::wstring span_wtext = wtext.substr(s.start, s.length);

            if (linc < s.line) {
                linc = s.line;
                block->lineCount = 1 + linc;
            }

            // cursors
            for (int pos = s.start; pos < s.start + s.length; pos++) {
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

                    if (s.line > 0) {
                        cr.x -= pos * fw;
                        cr.x += s.line_x * fw;
                        cr.x += (pos - s.start) * fw;
                    }

                    cr.y += (s.line * fh);
                    color_info_t cur = sel;
                    if (hlMainCursor) {
                        int cursor_pad = 4;
                        cr.width = 1;
                        if (!Renderer::instance()->is_terminal()) {
                            cr.width = 2;
                            cr.y -= cursor_pad;
                            cr.height += cursor_pad * 2;
                        }
                        cur = clr;
                    }
                    cr.x += alo->scroll_x;

                    Renderer::instance()->draw_rect(cr, { (uint8_t)cur.red, (uint8_t)cur.green, (uint8_t)cur.blue, 125 }, true, 1.0f);
                    if (ul && !Renderer::instance()->is_terminal()) {
                        cr.y += fh - 2;
                        cr.height = 1;
                        Renderer::instance()->draw_rect(cr, { (uint8_t)clr.red, (uint8_t)clr.green, (uint8_t)clr.blue }, true, 1.0f);
                    }

                    previous_cursor_rects.push_back(cr);
                }
            }

            s.x = alo->render_rect.x + (s.start * fw);
            s.x += alo->scroll_x;
            s.y = alo->render_rect.y + (l * fh);

            if (s.line > 0) {
                s.x -= s.start * fw;
                s.x += s.line_x * fw;
            }

            s.y += (s.line * fh);

            if (s.line == 0) {
                block->x = alo->render_rect.x;
                block->y = s.y;
            }

#if 0
            Renderer::instance()->draw_rect({ s.x,
                                                s.y,
                                                fw * s.length,
                                                fh },
                { (uint8_t)clr.red, (uint8_t)clr.green, (uint8_t)clr.blue, 50 }, false, 1.0f);
#endif

            Renderer::instance()->draw_wtext(_font, (wchar_t*)span_wtext.c_str(),
                s.x,
                s.y,
                { (uint8_t)clr.red, (uint8_t)clr.green, (uint8_t)clr.blue,
                    (uint8_t)(Renderer::instance()->is_terminal() ? clr.index : 255) },
                s.bold, s.italic);
        }

        l++;
        l += linc;
    }

    Renderer::instance()->state_restore();

    // duplicated from ::prelayout
    int ww = area->layout()->render_rect.w;
    if (!wrap && longest_block) {
        ww = longest_block->length() * fw;
    }
    content()->layout()->width = ww;

    if (prev_longest != longest_block) {
        layout_request();
    }
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

    popups = std::make_shared<popup_manager>();
    completer = std::make_shared<completer_view>();
    completer->on(EVT_ITEM_SELECT, [this](event_t& e) {
        e.cancelled = true;
        list_item_view* item = (list_item_view*)e.target;
        return commit_completer(item->data.value);
    });
    add_child(popups);

    // disable, otherwise dragging will not work
    scrollarea->disabled = true;

    view_item* container = (view_item*)scrollarea->parent;

    gutter = std::make_shared<gutter_view>();
    gutter->layout()->width = 60;
    gutter->layout()->order = 1;
    container->add_child(gutter);

    minimap = std::make_shared<minimap_view>();
    minimap->layout()->width = 80;
    minimap->layout()->order = 3;

    container->add_child(minimap);

    scrollarea->layout()->order = 2;
    v_scroll->layout()->order = 4;
    layout_sort(container->layout());

    content()->layout()->stack = true;

    if (Renderer::instance()->is_terminal()) {
        gutter->layout()->width = 4;
        minimap->layout()->width = 8;
    }

    on(EVT_MOUSE_DOWN, [this](event_t& evt) {
        if (evt.source != this) {
            return false;
        }
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

    if (!editor->highlighter.callback) {
        editor->highlighter.callback = [this](int line) {
            if (line >= this->start_row && line < this->start_row + this->rows) {
                Renderer::instance()->wake();
            }
            return true;
        };
    }

    editor->runAllOps();
    document_t* doc = &editor->document;

    if (doc->columns != cols || doc->rows != rows) {
        // todo
        doc->setColumns(cols);
        doc->setRows(rows);
        layout_request();
    }

    if (is_focused()) {
        app_t::instance()->currentEditor = editor;
    }

    panel_view::update();

    block_list::iterator it = doc->blocks.begin();

    int view_height = rows;
    int hl_prior = 16;
    int hl_start = start_row - hl_prior;
    int hl_length = view_height + hl_prior * 2;

    if (hl_start < 0)
        hl_start = 0;
    it += hl_start;
    for (int i = 0; i < hl_length && it != doc->blocks.end(); i++) {
        block_ptr b = *it++;
        if (!b->data || b->data->dirty) {
            editor->highlighter.requestHighlightBlock(b);
        }
    }
}

void editor_view::prelayout()
{
    scrollarea_view* area = view_item::cast<scrollarea_view>(scrollarea);
    // scrollbar_view *vs = view_item::cast<scrollbar_view>(v_scroll);
    // scrollbar_view *hs = view_item::cast<scrollbar_view>(h_scroll);

    int fw, fh;
    Renderer::instance()->get_font_extents(Renderer::instance()->font((char*)font.c_str()), &fw, &fh, NULL, 1);

    int lines = editor->document.blocks.size();

    int ww = area->layout()->render_rect.w - gutter->layout()->render_rect.w;

    bool wrap = app_t::instance()->lineWrap;
    if (!wrap && longest_block) {
        ww = longest_block->length() * fw;
    }

    content()->layout()->width = ww;
    content()->layout()->height = (lines + rows / 4) * fh;

    // gutter
    block_ptr block = editor->document.lastBlock();
    std::string lineNo = std::to_string(1 + block->lineNumber);
    gutter->layout()->width = (lineNo.length() + 3) * fw;
}

bool editor_view::mouse_down(int x, int y, int button, int clicks)
{
    if (!editor || popups->_views.size()) {
        return false;
    }

    mouse_x = x;
    mouse_y = y;

    layout_item_ptr lo = content()->layout();

    cursor_t cursor = editor->document.cursor();

    if (start_row >= editor->document.blocks.size()) {
        start_row = editor->document.blocks.size() - 1;
    }

    block_list::iterator it = editor->document.blocks.begin();
    it += start_row;

    int fw, fh;
    Renderer::instance()->get_font_extents(Renderer::instance()->font((char*)font.c_str()), &fw, &fh, NULL, 1);

    int l = 0;
    while (it != editor->document.blocks.end()) {
        block_ptr block = *it++;
        if (!block->data) {
            break;
        }

        blockdata_ptr blockData = block->data;
        std::string text = block->text() + "\n";
        const char* line = text.c_str();

        bool hitLine = false;
        bool hitSpan = false;
        int hitPos = 0;
        for (auto& s : blockData->rendered_spans) {
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
                    std::string span_text = text.substr(s.start, s.length);
                    printf("%s\n", span_text.c_str());
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
            int mods = Renderer::instance()->key_mods();
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
        if (l > 38)
            break;
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
        int drag_distance = dx * dx + dy * dy;
        if (drag_distance >= 100) {
            mouse_down(x, y, 0, 0);
        }
    }
    return true;
}

bool editor_view::input_key(int k)
{
    ensure_visible_cursor();
    return true;
}

bool editor_view::input_text(std::string text)
{
    if (!editor) {
        return false;
    }

    editor->pushOp(INSERT, text);
    editor->runAllOps();
    ensure_visible_cursor();

    show_completer();
    return true;
}

bool editor_view::input_sequence(std::string text)
{
    operation_e op = operationFromKeys(text);

    popup_manager* pm = view_item::cast<popup_manager>(popups);
    completer_view* cv = view_item::cast<completer_view>(completer);
    list_view* list = view_item::cast<list_view>(cv->list);
    if (pm->_views.size()) {
        switch (op) {
        case MOVE_CURSOR_UP:
        case MOVE_CURSOR_DOWN:
        case ENTER:
            return list->input_sequence(text);
        default:
            pm->clear();
            break;
        }
    }

    if (!editor) {
        return false;
    }
    editor->input(-1, text);
    editor->runAllOps();

    if (op != UNKNOWN) {
        ensure_visible_cursor();
    }

    // printf("sequence %s\n", text.c_str());

    switch (op) {
    case MOVE_CURSOR_NEXT_PAGE:
    case MOVE_CURSOR_PREVIOUS_PAGE:
        Renderer::instance()->throttle_up_events();
        break;
    case UNDO:
        Renderer::instance()->wake();
        Renderer::instance()->throttle_up_events();
        break;
    }

    return true;
}

void editor_view::scroll_to_cursor(cursor_t c, bool centered)
{
    block_ptr block = c.block();
    int l = block->lineNumber - 1;

    int fw, fh;
    Renderer::instance()->get_font_extents(Renderer::instance()->font((char*)font.c_str()), &fw, &fh, NULL, 1);

    scrollarea_view* area = view_item::cast<scrollarea_view>(scrollarea);
    layout_item_ptr alo = area->layout();

    int cols = alo->render_rect.w / fw;
    int rows = block->document->rows;

    if (start_row > l) {
        start_row = l;
    }
    if (start_row + rows - 4 < l) {
        start_row = l - (rows - 4);
    }

    if (centered) {
        l -= rows / 2;
        if (l < 0)
            l = 0;
        start_row = l;
    }

    int start_col = c.position();
    int scroll_x_col = alo->scroll_x / fw;
    if (start_col + scroll_x_col > cols - 4) {
        scroll_x_col = start_col - cols + 4;
        area->layout()->scroll_x = -scroll_x_col * fw;
    }

    int lead = 4;
    if (start_col + scroll_x_col - lead < 0) {
        scroll_x_col = -start_col + lead;
        area->layout()->scroll_x = scroll_x_col * fw;
    }

    area->layout()->scroll_y = -start_row * fh;

    // printf("scroll: %d %d\n", area->layout()->scroll_y, l);

    update_scrollbars();
}

void editor_view::ensure_visible_cursor()
{
    scrollarea_view* area = view_item::cast<scrollarea_view>(scrollarea);
    layout_item_ptr alo = area->layout();
    layout_item_ptr lo = area->layout();

    document_t* doc = &editor->document;

    int fw, fh;
    Renderer::instance()->get_font_extents(Renderer::instance()->font((char*)font.c_str()), &fw, &fh, NULL, 1);

    int cols = lo->render_rect.w / fw;
    int rows = lo->render_rect.h / fh;
    doc->setColumns(cols);
    doc->setRows(rows);

    cursor_t mainCursor = doc->cursor();
    scroll_to_cursor(mainCursor);
}

// move to completer object
void editor_view::show_completer()
{
    popup_manager* pm = view_item::cast<popup_manager>(popups);
    pm->clear();

    struct document_t* doc = &editor->document;
    if (doc->cursors.size() > 1) {
        return;
    }

    std::string prefix;

    cursor_t cursor = doc->cursor();
    struct block_t& block = *cursor.block();
    if (cursor.position() < 3)
        return;

    if (cursor.moveLeft(1)) {
        cursor.selectWord();
        prefix = cursor.selectedText();
        cursor.cursor = cursor.anchor;
        completer_cursor = cursor;
    }

    if (prefix.length() < 2) {
        return;
    }

    completer_view* cm = view_item::cast<completer_view>(completer);
    list_view* list = view_item::cast<list_view>(cm->list);
    list->data.clear();

    int completerItemsWidth = 0;
    std::vector<std::string> words = editor->indexer->findWords(prefix);
    for (auto w : words) {
        if (w.length() <= prefix.length()) {
            continue;
        }
        int score = levenshtein_distance((char*)prefix.c_str(), (char*)(w.c_str()));

        if (completerItemsWidth < w.length()) {
            completerItemsWidth = w.length();
        }

        list_item_data_t item = {
            text : w,
            value : w
        };
        list->data.push_back(item);
        list->value = "";
    }

    if (!pm->_views.size() && list->data.size()) {
        int fw, fh;
        Renderer::instance()->get_font_extents(Renderer::instance()->font((char*)font.c_str()), &fw, &fh, NULL, 1);

        blockdata_ptr blockData = completer_cursor.block()->data;
        span_info_t s = spanAtBlock(blockData.get(), completer_cursor.position(), true);
        scrollarea_view* area = view_item::cast<scrollarea_view>(scrollarea);

        int list_size = list->data.size();
        if (list_size > 8)
            list_size = 8;
        completer->layout()->width = completerItemsWidth * fw;
        completer->layout()->height = list_size * fh;

        if (Renderer::instance()->is_terminal()) {
            completer->layout()->width += 2;
        } else {
            completer->layout()->height += 14;
        }

        list->focus_next();
        for (int i = 0; i < 2; i++) {
            if (!list->ensure_visible_cursor())
                break;
        }

        pm->push_at(completer,
            {
                (uint8_t) ((completer_cursor.position() * fw) + scrollarea->layout()->render_rect.x + scrollarea->layout()->scroll_x - pm->layout()->render_rect.x),
                (uint8_t) (s.y - scrollarea->layout()->render_rect.y),
                (uint8_t) (fw * prefix.length()),
                (uint8_t) fh
            },
                s.y > area->layout()->render_rect.h / 3 ? POPUP_DIRECTION_UP : POPUP_DIRECTION_DOWN
            );

        // printf(">%d %d\n", s.y, scrollarea->layout()->render_rect.y);

        layout_request();
        Renderer::instance()->throttle_up_events();
    }
}

bool editor_view::commit_completer(std::string text)
{
    cursor_t cur = editor->document.cursor();
    std::ostringstream ss;
    ss << (completer_cursor.block()->lineNumber + 1);
    ss << ":";
    ss << completer_cursor.position();
    editor->pushOp(MOVE_CURSOR, ss.str());
    ss.str("");
    ss.clear();
    ss << (cur.block()->lineNumber + 1);
    ss << ":";
    ss << cur.position();
    editor->pushOp(MOVE_CURSOR_ANCHORED, ss.str());
    editor->pushOp(INSERT, text);
    editor->runAllOps();
    popup_manager* pm = view_item::cast<popup_manager>(popups);
    pm->clear();

    return true;
}
