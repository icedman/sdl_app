#include "editor_view.h"
#include "completer_view.h"
#include "damage.h"
#include "gutter.h"
#include "hash.h"
#include "indexer.h"
#include "minimap.h"
#include "popup.h"
#include "system.h"
#include "text.h"

#define SCROLL_Y_TOP_PAD 2
#define SCROLL_Y_BOTTOM_PAD (SCROLL_Y_TOP_PAD + 4)
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
            break;
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
    , scroll_to(-1)
{
    start_tasks();

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
    hl_task = std::make_shared<highlighter_task_t>(this);
    tasks_manager_t::instance()->enroll(hl_task);
}

void editor_view_t::stop_tasks()
{
    hl_task->stop();
    tasks_manager_t::instance()->withdraw(hl_task);
}

bool _move_cursor(editor_view_t* ev, cursor_t& cursor, int dir)
{
    block_ptr block = cursor.block();
    blockdata_ptr data = block->data;

    if (!data)
        return false;

    // find span
    bool found = false;
    span_info_t ss;
    int pos;

    return false;
}

bool editor_view_t::handle_key_sequence(event_t& event)
{
    operation_e op = operationFromKeys(event.text);

    if (!editor) {
        return false;
    }

    cursor_t cursor = editor->document.cursor();

    bool single_block = (editor->document.cursors.size() == 1 && !editor->document.cursor().isMultiBlockSelection());

    // printf("%s %s\n", nameFromOperation(op).c_str(), event.text.c_str());

    // special navigation - move this to editor?
    switch (op) {
    case MOVE_CURSOR_UP:
    case MOVE_CURSOR_DOWN: {
        block_ptr block = cursor.block();
        if (wrapped && block->lineCount > 1) {
            if (_move_cursor(this, cursor, op == MOVE_CURSOR_UP ? -1 : 1)) {
                //     text = "";
                //     std::ostringstream ss;
                //     ss << (block->lineNumber + 1);
                //     ss << ":";
                //     ss << cursor.position();
                //     int mods = Renderer::instance()->key_mods();
                //     editor->pushOp((mods & K_MOD_CTRL) ? MOVE_CURSOR_ANCHORED : MOVE_CURSOR, ss.str());
            }
        }
        break;
    }

    case MOVE_CURSOR_NEXT_PAGE:
    case MOVE_CURSOR_NEXT_PAGE_ANCHORED: {
        op = operation_e::UNKNOWN;
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
            int mods = system_t::instance()->key_mods();
            editor->pushOp((mods & K_MOD_SHIFT) ? MOVE_CURSOR_ANCHORED : MOVE_CURSOR, ss.str());
        }
        break;
    }
    case MOVE_CURSOR_PREVIOUS_PAGE:
    case MOVE_CURSOR_PREVIOUS_PAGE_ANCHORED: {
        op = operation_e::UNKNOWN;
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
            int mods = system_t::instance()->key_mods();
            editor->pushOp((mods & K_MOD_SHIFT) ? MOVE_CURSOR_ANCHORED : MOVE_CURSOR, ss.str());
        }
        break;
    }
    }

    if (op != operation_e::UNKNOWN) {
        editor->input(-1, event.text);
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

int editor_view_t::cursor_x(cursor_t cursor)
{
    return cursor.position() * font()->width;
}

int editor_view_t::cursor_y(cursor_t cursor)
{
    int lineNumber = cursor.block()->lineNumber;
    lineNumber -= (visible_blocks / 4);
    if (lineNumber < 0) {
        lineNumber = 0;
    }

    block_ptr block = editor->document.blockAtLine(lineNumber);
    if (!block) {
        return 0;
    }

    int y = block->lineNumber * block_height;
    for (int i = 0; i < visible_blocks && block; i++) {
        if (block == cursor.block()) {
            break;
        }
        if (!block->lineCount) {
            block->lineCount = 1;
        }
        y += (block->lineCount * block_height);
        block = block->next();
    }

    return y;
}

point_t editor_view_t::cursor_xy(cursor_t cursor)
{
    return { cursor_x(cursor), cursor_y(cursor) };
}

void editor_view_t::ensure_visible_cursor()
{
    layout_item_ptr lo = layout();
    layout_item_ptr slo = scrollarea->layout();

    cursor_t cursor = editor->document.cursor();

    int cursor_screen_y = cursor_y(cursor);
    cursor_screen_y += slo->scroll_y;

    point_t p = { lo->render_rect.x + lo->render_rect.w / 2, cursor_screen_y };
    rect_t r = lo->render_rect;
    r.y -= block_height;

    if (slo->scroll_y < 0)
        r.y += (SCROLL_Y_TOP_PAD * block_height);
    r.h -= (SCROLL_Y_BOTTOM_PAD + 1) * block_height;

    if (point_in_rect(p, r)) {
        return;
    }

    scroll_to_cursor(cursor);
}

void editor_view_t::scroll_to_cursor(cursor_t cursor)
{
    layout_item_ptr lo = layout();
    layout_item_ptr slo = scrollarea->layout();
    int prev = slo->scroll_y;

    scroll_to = -cursor_y(cursor);
    if (slo->scroll_y < 0)
        scroll_to += (SCROLL_Y_TOP_PAD * block_height);

    if (scroll_to > 0) {
        scroll_to = 0;
    }

    if (prev > scroll_to) {
        int y = lo->render_rect.h - (block_height * SCROLL_Y_BOTTOM_PAD);
        block_ptr block = cursor.block();
        while (block && y > 0) {
            if (block->lineCount == 0) {
                block->lineCount = 1;
            }
            scroll_to += block->lineCount * block_height;
            y -= block->lineCount * block_height;
            block = block->previous();
        }
    }

    slo->scroll_y = scroll_to;

    event_t evt;
    evt.sx = 0;
    evt.sy = 0;
    handle_mouse_move(evt);
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

void editor_view_t::request_highlight(block_ptr block)
{
    ((highlighter_task_t*)(hl_task.get()))->hl.push_back(block);
}
