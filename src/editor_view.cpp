#include "editor_view.h"
#include "completer_view.h"
#include "gutter_view.h"
#include "minimap_view.h"

#include "app.h"
#include "indexer.h"
#include "renderer.h"
#include "search.h"
#include "style.h"
#include "utf8.h"
#include "view.h"

#include "scrollbar.h"
#include <unistd.h>

bool _span_from_cursor(cursor_t& cursor, span_info_t& span, int &span_pos);

void editor_view::render()
{
    if (!editor) {
        return;
    }

    RenFont* _font = Renderer::instance()->font((char*)font.c_str());

    app_t* app = app_t::instance();
    view_style_t vs = style;

    bool wrap = app->lineWrap && !editor->singleLineEdit;
    int indent = app->tabSize;

    editor->document.wrap = wrap;
    editor->document.wrapIndent = indent;

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
    it += pre_line;

    theme_ptr theme = app->theme;

    color_info_t fg = Renderer::instance()->color_for_index(app_t::instance()->fg);
    color_info_t sel = Renderer::instance()->color_for_index(app_t::instance()->selBg);

    bool has_focus = is_focused();

    block_ptr prev_longest = longest_block;
    if (longest_block && !longest_block->isValid()) {
        longest_block = 0;
    }

    int offset_y = (pre_line + 1) * fh;
    if (!Renderer::instance()->is_terminal()) {
        offset_y += 8;
    }

    bool offscreen = false;
    int lineNumber = pre_line;
    int l = 0;
    while (it != doc->blocks.end() && l < rows * 2) {
        block_ptr block = *it++;
        block->lineNumber = lineNumber++;

        blockdata_ptr blockData;
        if (!block->data || block->data->dirty) {
            editor->highlighter.highlightBlock(block);
        }

        if (block->data) {
            blockData = block->data;
        }

        if (!blockData) { // } || blockData->dirty) {
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
        }

        if (!longest_block) {
            longest_block = block;
        }
        if (longest_block->length() < block->length()) {
            longest_block = block;
        }

        std::string text = block->text() + " ";

        blockData->rendered_spans = block->layoutSpan(cols, wrap, indent);
        block->lineHeight = fh;
        block->y = offset_y;

        offscreen = (block->y + alo->scroll_y > fh * rows || block->y + (block->lineCount * fh) < -alo->scroll_y);
        offset_y += block->lineCount * fh;

        for (auto& s : blockData->rendered_spans) {
            if (s.length == 0)
                continue;

            color_info_t clr = Renderer::instance()->color_for_index(s.colorIndex);
            if (s.start + s.length >= utf8_length(text)) {
                s.length = utf8_length(text) - s.start;
                if (s.length <= 0) {
                    s.length = 0;
                    continue;
                }
            }

            std::string span_text = utf8_substr(text, s.start, s.length);

            // cursors
            if (!offscreen) {
                for (int pos = s.start; pos < s.start + s.length; pos++) {
                    bool hl = false;
                    bool ul = false;
                    bool hasSelection = false;

                    RenRect cr = {
                        alo->render_rect.x + (pos * fw),
                        block->y + alo->scroll_y,
                        fw, fh
                    };

                    // bracket
                    if (editor->cursorBracket1.bracket != -1 && editor->cursorBracket2.bracket != -1) {
                        if ((pos == editor->cursorBracket1.position && block->lineNumber == editor->cursorBracket1.line) || (pos == editor->cursorBracket2.position && block->lineNumber == editor->cursorBracket2.line)) {
                            Renderer::instance()->draw_underline(cr, { (uint8_t)clr.red, (uint8_t)clr.green, (uint8_t)clr.blue });
                        }
                    }

                    for (auto& c : cursors) {
                        if (pos == c.position() && block == c.block()) {
                            hasSelection |= c.hasSelection();
                            hl = true && (has_focus || c.hasSelection());
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

                    if (hl || hasSelection) {

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
                                if (hasSelection) {
                                    cr.width = fw;
                                } else {
                                    cr.width = 2;
                                    cr.y -= cursor_pad;
                                    cr.height += cursor_pad * 2;
                                }
                            } else {
                                if (hasSelection) {
                                    ul = true;
                                }
                            }
                            cur = clr;
                        }
                        cr.x += alo->scroll_x;

                        if (hl || hasSelection) {
                            Renderer::instance()->draw_rect(cr, { (uint8_t)cur.red, (uint8_t)cur.green, (uint8_t)cur.blue, 125 }, true, 1.0f);
                        }

                        if (ul) {
                            Renderer::instance()->draw_underline(cr, { (uint8_t)clr.red, (uint8_t)clr.green, (uint8_t)clr.blue });
                        }
                    }
                }
            }

            s.x = alo->render_rect.x + (s.start * fw);
            s.x += alo->scroll_x;
            s.y = block->y;

            if (s.line > 0) {
                s.x -= s.start * fw;
                s.x += s.line_x * fw;
            }

            s.y += (s.line * fh);

            if (s.line == 0) {
                block->x = alo->render_rect.x;
            }

            if (!offscreen) {
    #if 0
                Renderer::instance()->draw_rect({ s.x,
                                                    s.y + alo->scroll_y,
                                                    fw * s.length,
                                                    fh },
                    { (uint8_t)clr.red, (uint8_t)clr.green, (uint8_t)clr.blue, 50 }, false, 1.0f);
    #endif
                Renderer::instance()->draw_text(_font, span_text.c_str(),
                    s.x,
                    s.y + alo->scroll_y,
                    { (uint8_t)clr.red, (uint8_t)clr.green, (uint8_t)clr.blue,
                        (uint8_t)(Renderer::instance()->is_terminal() ? clr.index : 255) },
                    s.bold, s.italic);
            }
        }

        l++;
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

    // compute document height
    computed_lines = editor->document.blocks.size();
    int pre = rows / 2;
    int s = start_row - pre;
    if (s < 0) {
        s = 0;
    }
    pre_line = s;

    int c = rows * 2;
    block_ptr b = editor->document.blockAtLine(pre_line);
    while(b && c-->0) {
        computed_lines += b->lineCount - 1;
        b = b->next();
    }

    // app_t::log("%d %d", computed_lines, editor->document.blocks.size());
}

editor_view::editor_view()
    : panel_view()
    , start_row(0)
    , pre_line(0)
    , computed_lines(0)
{
    type = "editor";
    font = "editor";
    interactive = true;
    focusable = true;
    view_set_focused(this);

    popups = std::make_shared<popup_manager>();
    completer = std::make_shared<completer_view>();

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

    view_item::cast<minimap_view>(minimap)->scrollbar = v_scroll;

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

void editor_view::update(int millis)
{
    if (!editor) {
        return;
    }

    document_t* doc = &editor->document;

    if (doc->columns != cols || doc->rows != rows) {
        // todo
        doc->setColumns(cols);
        doc->setRows(rows);
        layout_request();
    }

    if (is_focused() && !editor->singleLineEdit) {
        app_t::instance()->currentEditor = editor;
    }

    panel_view::update(millis);

    editor->matchBracketsUnderCursor();
}

void editor_view::prelayout()
{
    scrollarea_view* area = view_item::cast<scrollarea_view>(scrollarea);

    int fw, fh;
    Renderer::instance()->get_font_extents(Renderer::instance()->font((char*)font.c_str()), &fw, &fh, NULL, 1);

    int lines = editor->document.blocks.size();
    if (computed_lines != 0) {
        lines = computed_lines;
    }

    int ww = area->layout()->render_rect.w - gutter->layout()->render_rect.w;

    bool wrap = app_t::instance()->lineWrap && !editor->singleLineEdit;
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

    scrollarea_view* area = view_item::cast<scrollarea_view>(scrollarea);
    layout_item_ptr alo = area->layout();
    layout_item_ptr lo = content()->layout();

    mouse_x = x;
    mouse_y = y;

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
        std::string text = block->text();
        const char* line = text.c_str();

        bool hitLine = false;
        bool hitSpan = false;
        int hitPos = 0;
        for (auto& s : blockData->rendered_spans) {
            layout_rect r = {
                s.x,
                s.y + alo->scroll_y,
                s.length * fw,
                fh
            };
            if (y > r.y && y <= r.y + r.h) {
                hitLine = true;
                int pos = (x - s.x) / fw;
                hitPos = pos + s.start;
                if (x > r.x && x <= r.x + r.w) {
                    // std::string span_text = utf8_substr(text, s.start, s.length);
                    // printf("%s\n", span_text.c_str());
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
            if (mods & K_MOD_CTRL) {
                editor->pushOp(ADD_CURSOR_AND_MOVE, ss.str());
            } else if (clicks == 0 || mods & K_MOD_SHIFT) {
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

    if (!editor->singleLineEdit) {
        view_item::cast<completer_view>(completer)->show_completer(editor);
    }

    return true;
}

bool _span_from_cursor(cursor_t& cursor, span_info_t& span, int &span_pos)
{
    block_ptr block = cursor.block();
    blockdata_ptr data = block->data;

    if (!data) return false;

    // find span
    bool found = false;
    span_info_t ss;
    int pos;

    for (auto s : data->rendered_spans) {
        if (cursor.position() >= s.start && cursor.position() < s.start + s.length) {
            ss = s;
            pos = cursor.position();
            if (s.line > 0) {
                pos -= s.start;
                pos += s.line_x;
            }
            found = true;
            break;
        }
    }

    span = ss;
    span_pos = pos;
    return found;
}

bool _move_cursor(cursor_t& cursor, int dir)
{
    block_ptr block = cursor.block();
    blockdata_ptr data = block->data;

    if (!data)
        return false;

    // find span
    bool found = false;
    span_info_t ss;
    int pos;

    found = _span_from_cursor(cursor, ss, pos);
    // for (auto s : data->rendered_spans) {
    //     if (cursor.position() >= s.start && cursor.position() < s.start + s.length) {
    //         ss = s;
    //         pos = cursor.position();
    //         if (s.line > 0) {
    //             pos -= s.start;
    //             pos += s.line_x;
    //         }
    //         found = true;
    //         break;
    //     }
    // }

    if (!found)
        return false;

    for (auto s : data->rendered_spans) {
        if (s.line == ss.line + dir) {
            if (s.line == 0)
                s.line_x = s.start;
            if (pos >= s.line_x && pos < s.line_x + s.length) {
                cursor.cursor.position = s.start + pos - s.line_x;
                return true;
            }
        }
    }

    return false;
}

bool editor_view::input_sequence(std::string text)
{
    operation_e op = operationFromKeys(text);

    if (!editor->singleLineEdit) {
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
    }

    if (!editor) {
        return false;
    }

    // navigate wrapped lines
    switch (op) {
    case MOVE_CURSOR_UP:
    case MOVE_CURSOR_DOWN: {
        cursor_t cursor = editor->document.cursor();
        block_ptr block = cursor.block();
        if (block->lineCount > 1) {
            if (_move_cursor(cursor, op == MOVE_CURSOR_UP ? -1 : 1)) {
                text = "";
                std::ostringstream ss;
                ss << (block->lineNumber + 1);
                ss << ":";
                ss << cursor.position();
                int mods = Renderer::instance()->key_mods();
                editor->pushOp((mods & K_MOD_CTRL) ? MOVE_CURSOR_ANCHORED : MOVE_CURSOR, ss.str());
            }
        }
    } break;
    }

    editor->input(-1, text);
    editor->runAllOps();

    if (op != UNKNOWN) {
        ensure_visible_cursor();
    }

    switch (op) {
    case UNDO:
    case CUT:
    case PASTE:
        Renderer::instance()->throttle_up_events();
        break;

    case MOVE_CURSOR_NEXT_PAGE:
    case MOVE_CURSOR_PREVIOUS_PAGE:
        Renderer::instance()->throttle_up_events();
        break;
    }

#if 0
    switch (op) {
    case UNDO:
    case CUT:
    case PASTE:
    case ENTER:
    case BACKSPACE:
    case DELETE:
    case DUPLICATE_LINE:
    case DUPLICATE_SELECTION:
    case DELETE_SELECTION:
    case INSERT:
        if (!editor->document.lineNumberingIntegrity()) {
            log("line numbering error\n");
        }
        break;
    }
#endif

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

    // if (start_row > l) {
    //     start_row = l;
    // }
    // if (start_row + rows - 4 < l) {
    //     start_row = l - (rows - 4);
    // }

    // if (centered) {
    //     l -= rows / 2;
    //     if (l < 0)
    //         l = 0;
    //     start_row = l;
    // }

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

    span_info_t ss;
    int pos;

    if (_span_from_cursor(c, ss, pos)) {
        int uy = -(c.block()->y - (ss.line + 2) * fh);
        if (area->layout()->scroll_y < uy
            ) {
            area->layout()->scroll_y = uy;
        }

        int ly = -(c.block()->y - (ss.line + 2) * fh) + ((rows - 2) * fh);
        if (area->layout()->scroll_y > ly) {
            area->layout()->scroll_y = ly;
        }
    }

    // area->layout()->scroll_y = -start_row * fh;
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
