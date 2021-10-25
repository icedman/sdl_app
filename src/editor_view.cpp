#include "editor_view.h"
#include "completer_view.h"
#include "damage.h"
#include "gutter.h"
#include "hash.h"
#include "indexer.h"
#include "minimap.h"
#include "popup.h"
#include "search_view.h"
#include "system.h"
#include "text.h"

#define PRE_VISIBLE_HL 20
#define POST_VISIBLE_HL 100
#define MAX_HL_BUCKET ((PRE_VISIBLE_HL + POST_VISIBLE_HL) * 4)

highlighter_task_t::highlighter_task_t(editor_view_t* editor)
    : task_t()
    , editor(editor)
{
}

bool highlighter_task_t::run(int limit)
{
    if (!editor || !editor->editor || !running)
        return false;

    if (hl.size() > MAX_HL_BUCKET) {
        hl.erase(hl.begin(), hl.begin() + (hl.size() - MAX_HL_BUCKET));
    }

    int hltd = 0;
    while (hl.size()) {
        block_ptr block = hl.front();
        hl.erase(hl.begin(), hl.begin() + 1);
        if (!block->data || block->data->dirty) {
            editor->editor->highlight(block->lineNumber, 1);

            if (editor->editor->indexer) {
                editor->editor->indexer->indexBlock(block);
            }

            hltd++;
            if (hltd > 4) {
                break;
            }
        }
    }

    int first = editor->first_visible;
    block_list::iterator it = editor->editor->document.blocks.begin();
    first -= PRE_VISIBLE_HL;
    if (first >= editor->editor->document.blocks.size()) {
        first = editor->editor->document.blocks.size() - 1;
    }
    if (first < 0) {
        first = 0;
    }

    int idx = 0;
    it += first;
    while (it != editor->editor->document.blocks.end()) {
        block_ptr block = *it++;

        if (!block->data || block->data->dirty) {
            hl.push_back(block);
        }

        if (idx++ > editor->visible_blocks + POST_VISIBLE_HL)
            break;
    }
    return hltd > 0;
}

editor_view_t::editor_view_t()
    : rich_text_t()
{
    can_focus = true;
    draw_cursors = true;
    on(EVT_KEY_SEQUENCE, [this](event_t& event) {
        event.cancelled = true;
        this->handle_key_sequence(event);
        return true;
    });
    on(EVT_KEY_TEXT, [this](event_t& event) {
        event.cancelled = true;
        this->handle_key_text(event);
        return true;
    });
    on(EVT_MOUSE_DOWN, [this](event_t& event) {
        event.cancelled = true;
        this->handle_mouse_down(event);
        return true;
    });
    on(EVT_MOUSE_DRAG, [this](event_t& event) {
        event.cancelled = true;
        this->handle_mouse_move(event);
        return true;
    });

    layout()->name = "editor";
    set_font(system_t::instance()->renderer.font("editor"));
}

void editor_view_t::start_tasks()
{
    if (!task) {
        task = std::make_shared<highlighter_task_t>(this);
        tasks_manager_t::instance()->enroll(task);
    }
}

void editor_view_t::stop_tasks()
{
    if (task) {
        task->stop();
        tasks_manager_t::instance()->withdraw(task);
        task = nullptr;
    }
}

bool _move_cursor(editor_view_t* ev, cursor_t& cursor, int dir)
{
    block_ptr block = cursor.block();
    blockdata_ptr data = block->data;

    if (!data)
        return false;

    // find rich text
    rich_text_block_t* ctb = NULL;
    rich_text_block_t* pctb = NULL;
    for (auto c : ev->subcontent->children) {
        rich_text_block_t* tb = (rich_text_block_t*)c.get();
        if (tb->block == block) {
            ctb = tb;
            break;
        }
        pctb = tb;
    }

    if (!ctb)
        return false;

    // find span
    layout_text_span_t* cts = 0;
    for (auto span : ctb->layout()->children) {
        layout_text_span_t* text_span = (layout_text_span_t*)span.get();
        if (!text_span->visible)
            continue;
        if (cursor.position() >= text_span->start && cursor.position() < text_span->start + text_span->length) {
            cts = text_span;
            break;
        }
    }

    if (!cts)
        return false;

    // if move up to previous block
    int loop = 1;
    if (dir == -1 && cts->rect.y == 0 && pctb && pctb->block->lineCount > 1) {
        ctb = pctb;
        loop = 2;
    }

    if (!ctb)
        return false;

    // find adjacent span
    layout_text_span_t* target = 0;
    int offset = (cursor.position() - cts->start);
    int x = cts->render_rect.x + (offset * ev->font()->width);
    int y = cts->render_rect.y;
    if (dir < 0) {
        y -= (ev->block_height * 0.5f);
    } else {
        y += (ev->block_height * 1.5f);
    }

    // find a hit
    for (int i = 0; i < loop; i++) {
        point_t p = { x, y };
        for (auto span : ctb->layout()->children) {
            layout_text_span_t* text_span = (layout_text_span_t*)span.get();
            if (!text_span->visible)
                continue;
            if (point_in_rect(p, text_span->render_rect)) {
                target = text_span;
                break;
            }
        }
        if (target)
            break;
        y -= ev->block_height;
    }

    if (!target) {
        int x = (cts->render_rect.x - ev->scrollarea->layout()->render_rect.x) / ev->font()->width;
        if (dir == -1 && block->previous()) {
            cursor.setPosition(block->previous(), offset + x);
            return true;
        }
        if (dir == 1 && block->next()) {
            cursor.setPosition(block->next(), offset + x);
            return true;
        }
        return true;
    }

    offset = (target->render_rect.x - x) / ev->font()->width;
    cursor.setPosition(ctb->block, target->start - offset);
    return true;
}

bool editor_view_t::handle_key_sequence(event_t& event)
{
    operation_e op = operationFromKeys(event.text);

    if (!editor) {
        return false;
    }
    std::string text = event.text;

    cursor_t cursor = editor->document.cursor();

    bool single_block = (editor->document.cursors.size() == 1 && !editor->document.cursor().isMultiBlockSelection());

    printf("%s %s\n", nameFromOperation(op).c_str(), event.text.c_str());

    // special navigation - move this to editor?
    switch (op) {
    case POPUP_SEARCH:
    case POPUP_SEARCH_LINE: {
        search_view_t* srch = search()->cast<search_view_t>();
        std::string text = "";
        if (op == POPUP_SEARCH_LINE) {
            text = ":";
        } else if (cursor.hasSelection() && !cursor.isMultiBlockSelection()) {
            text = cursor.selectedText();
        }
        if (srch->update_data(text)) {
            view_ptr _pm = popup_manager_t::instance();
            popup_manager_t* pm = _pm->cast<popup_manager_t>();
            pm->clear();

            point_t pos = {
                scrollarea->layout()->render_rect.x + scrollarea->layout()->render_rect.w,
                scrollarea->layout()->render_rect.y
            };
            rect_t rect = { pos.x, pos.y, font()->width, font()->height };
            pm->push_at(search(), rect, POPUP_DIRECTION_LEFT);
        }
        return true;
    }

    case MOVE_LINE_DOWN: {
        scroll_down();
        return true;
    }
    case MOVE_LINE_UP: {
        scroll_up();
        return true;
    }
    case MOVE_CURSOR_UP_ANCHORED:
    case MOVE_CURSOR_DOWN_ANCHORED:
    case MOVE_CURSOR_UP:
    case MOVE_CURSOR_DOWN: {
        if (wrapped) {
            bool up = (op == MOVE_CURSOR_UP || op == MOVE_CURSOR_UP_ANCHORED);
            block_ptr block = cursor.block();
            block_ptr prev = block->previous();
            bool nav_wrapped = block->lineCount > 1 || (prev && prev->lineCount > 1 && up);
            if (nav_wrapped && _move_cursor(this, cursor, up ? -1 : 1)) {
                std::ostringstream ss;
                ss << (cursor.block()->lineNumber + 1);
                ss << ":";
                ss << cursor.position();
                printf(">>%s\n", ss.str().c_str());
                bool anchor = (op == MOVE_CURSOR_UP_ANCHORED || op == MOVE_CURSOR_DOWN_ANCHORED);
                editor->pushOp(anchor ? MOVE_CURSOR_ANCHORED : MOVE_CURSOR, ss.str());
                text = "";
                op = operation_e::UNKNOWN;
            }
        }
        break;
    }

    case MOVE_CURSOR_NEXT_PAGE:
    case MOVE_CURSOR_NEXT_PAGE_ANCHORED: {
        view_ptr last;
        for (auto c : subcontent->children) {
            if (c->layout()->visible) {
                last = c;
            }
        }

        if (last) {
            rich_text_block_t* lb = last->cast<rich_text_block_t>();
            std::ostringstream ss;
            ss << lb->block->lineNumber;
            ss << ":";
            ss << cursor.position();
            bool anchor = op == MOVE_CURSOR_NEXT_PAGE_ANCHORED;
            editor->pushOp(anchor ? MOVE_CURSOR_ANCHORED : MOVE_CURSOR, ss.str());
        }
        text = "";
        op = operation_e::UNKNOWN;
        break;
    }
    case MOVE_CURSOR_PREVIOUS_PAGE:
    case MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED: {
        view_ptr first;
        for (auto c : subcontent->children) {
            if (c->layout()->visible) {
                first = c;
                break;
            }
        }

        if (first) {
            rich_text_block_t* lb = first->cast<rich_text_block_t>();
            int line = lb->block->lineNumber - 8;
            if (line < 0)
                line = 0;
            std::ostringstream ss;
            ss << line;
            ss << ":";
            ss << cursor.position();
            bool anchor = op == MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED;
            editor->pushOp(anchor ? MOVE_CURSOR_ANCHORED : MOVE_CURSOR, ss.str());
        }
        text = "";
        op = operation_e::UNKNOWN;
        break;
    }
    }

    if (op != operation_e::UNKNOWN) {
        editor->input(-1, text);
    }
    editor->runAllOps();

    switch (op) {
    case UNDO:
    case CUT:
    case PASTE:
        break;
    }

#if 0
    switch (op) {
    case TOGGLE_FOLD:
    case UNDO:
    case CUT:
    case PASTE:
    case ENTER:
    case BACKSPACE:
    case DEL:
    case DUPLICATE_LINE:
    case DUPLICATE_SELECTION:
    case DELETE_SELECTION:
    case INSERT:
        if (!editor->document.lineNumberingIntegrity()) {
            printf("line numbering error\n");
        }
        break;
    }
#endif

    update_blocks();
    ensure_visible_cursor();
    relayout_virtual_blocks();
    return true;
}

bool editor_view_t::handle_key_text(event_t& event)
{
    int mods = system_t::instance()->key_mods();
    if (mods & K_MOD_CTRL || mods & K_MOD_ALT)
        return true;

    editor->pushOp(INSERT, event.text);
    editor->runAllOps();
    update_blocks();
    ensure_visible_cursor();
    relayout_virtual_blocks();

    if (editor->document.cursors.size() == 1) {
        cursor_t cur = editor->document.cursor();
        if (!cur.hasSelection()) {
            completer_t* com = completer()->cast<completer_t>();
            if (com->update_data()) {
                view_ptr _pm = popup_manager_t::instance();
                popup_manager_t* pm = _pm->cast<popup_manager_t>();
                pm->clear();

                point_t pos = cursor_xy(cur);
                pos.x += scrollarea->layout()->render_rect.x;
                pos.x -= com->selected_word.length() * font()->width;
                pos.y += scrollarea->layout()->render_rect.y;
                pos.y += scrollarea->layout()->scroll_y;
                rect_t rect = { pos.x, pos.y, font()->width, font()->height };
                pm->push_at(completer(), rect,
                    pos.y - (block_height * 4) > scrollarea->layout()->render_rect.h / 2
                        ? POPUP_DIRECTION_UP
                        : POPUP_DIRECTION_DOWN);
            }
        }
    }

    return true;
}

bool editor_view_t::handle_mouse_down(event_t& event)
{
    layout_item_ptr lo = layout();

    mouse_xy = { event.x, event.y };
    view_ptr text_block;
    for (auto c : subcontent->children) {
        layout_item_ptr clo = c->layout();
        if (!clo->visible)
            continue;
        rect_t r = clo->render_rect;
        if (r.w < lo->render_rect.w) {
            r.w = lo->render_rect.w;
        }
        if (point_in_rect(mouse_xy, r)) {
            text_block = c;
            break;
        }
    }

    if (!text_block) {
        return true;
    }

    block_ptr block = text_block->cast<rich_text_block_t>()->block;
    int pos = block->length();
    for (auto l : text_block->layout()->children) {
        layout_text_span_t* span = (layout_text_span_t*)l.get();
        if (!span->visible)
            continue;
        if (point_in_rect(mouse_xy, span->render_rect)) {
            pos = span->start + (mouse_xy.x - span->render_rect.x) / (span->render_rect.w / span->length);
            break;
        }
    }

    std::ostringstream ss;
    ss << (block->lineNumber + 1);
    ss << ":";
    ss << pos;
    int clicks = event.clicks;
    int mods = system_t::instance()->key_mods();
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

    editor->runAllOps();
    update_blocks();
    ensure_visible_cursor();
    relayout_virtual_blocks();
    return true;
}

bool editor_view_t::handle_mouse_move(event_t& event)
{
    if (event.button) {
        int dx = mouse_xy.x - event.x;
        int dy = mouse_xy.y - event.y;
        int drag_distance = dx * dx + dy * dy;
        if (drag_distance >= 100) {
            handle_mouse_down(event);
        }
    }
    return true;
}

void editor_view_t::refresh()
{
    update_blocks();
    relayout_virtual_blocks();
}

view_ptr editor_view_t::gutter()
{
    if (!_gutter) {
        view_ptr container = children[0];
        _gutter = std::make_shared<gutter_t>(this);
        _gutter->layout()->order = 5;
        container->add_child(_gutter);
        layout_sort(container->layout());
    }
    return _gutter;
}

view_ptr editor_view_t::minimap()
{
    if (!_minimap) {
        view_ptr container = children[0];
        _minimap = std::make_shared<minimap_t>(this);
        _minimap->layout()->order = 15;
        container->add_child(_minimap);
        layout_sort(container->layout());
    }
    return _minimap;
}

view_ptr editor_view_t::completer()
{
    if (!_completer) {
        _completer = std::make_shared<completer_t>(this);
    }
    return _completer;
}

view_ptr editor_view_t::search()
{
    if (!_search) {
        _search = std::make_shared<search_view_t>(this);
    }
    return _search;
}

void editor_view_t::request_highlight(block_ptr block)
{
    if ((wheel_y > 100 || wheel_y < -100) || is_dragged(v_scroll.get()) || is_dragged(v_scroll->content().get()) || is_dragged(_minimap.get())) {
        // printf("busy..\n");
        return;
    }
    ((highlighter_task_t*)(task.get()))->hl.push_back(block);
}
