#include "editor_view.h"
#include "damage.h"
#include "hash.h"
#include "system.h"
#include "text.h"
#include "gutter.h"

editor_view_t::editor_view_t()
    : rich_text_t()
    , scroll_to(-1)
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
    return true;
}

bool editor_view_t::handle_key_text(event_t& event)
{
    editor->pushOp(INSERT, event.text);
    editor->runAllOps();
    update_blocks();
    ensure_visible_cursor();
    return true;
}

bool editor_view_t::handle_mouse_down(event_t& event)
{
    layout_item_ptr lo = layout();

    point_t p = { event.x, event.y };
    view_ptr text_block;
    for (auto c : subcontent->children) {
        layout_item_ptr clo = c->layout();
        if (!clo->visible)
            continue;
        rect_t r = clo->render_rect;
        r.w = lo->render_rect.w;
        if (point_in_rect(p, r)) {
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
        if (point_in_rect(p, span->render_rect)) {
            pos = span->start + (p.x - span->render_rect.x) / (span->render_rect.w / span->length);
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
    return true;
}
bool editor_view_t::handle_mouse_move(event_t& event)
{
    if (event.button) {
        int dx = mouse_x - event.x;
        int dy = mouse_y - event.y;
        int drag_distance = dx * dx + dy * dy;
        if (drag_distance >= 100) {
            handle_mouse_down(event);
        }
    }
    return true;
}

void editor_view_t::ensure_visible_cursor()
{
    layout_item_ptr lo = layout();
    layout_item_ptr slo = scrollarea->layout();

    cursor_t cursor = editor->document.cursor();
    int cursor_screen_y = cursor.block()->lineNumber * block_height + slo->scroll_y;
    point_t p = { lo->render_rect.x + lo->render_rect.w / 2, cursor_screen_y + block_height / 2 };
    rect_t r = lo->render_rect;

    bool visible = false;
    for (auto c : subcontent->children) {
        layout_item_ptr clo = c->layout();
        if (!clo->visible)
            break;
        rich_text_block_t* tb = (rich_text_block_t*)c.get();
        if (tb->block == cursor.block()) {
            visible = point_in_rect({ clo->render_rect.x + 1, clo->render_rect.y + font()->height }, lo->render_rect);
            break;
        }
    }

    if (!visible) {
        scroll_to_cursor(cursor);
    }
}

void editor_view_t::scroll_to_cursor(cursor_t cursor)
{
    layout_item_ptr lo = layout();
    layout_item_ptr slo = scrollarea->layout();
    int prev = slo->scroll_y;
    scroll_to = -cursor.block()->lineNumber * block_height;

    if (prev > scroll_to) {
        scroll_to += lo->render_rect.h / 4;
        // block_ptr block = cursor.block();
        // int c = visible_blocks;
        // while(block && c > 0) {
        //     scroll_to -= (block->lineCount - 1) * font()->height;
        //     c -= block->lineCount;
        //     block = block->previous();
        // }
        // printf("%d\n", scroll_to);
    }

    slo->scroll_y = scroll_to;
}

view_ptr editor_view_t::gutter()
{
    if (!_gutter) {
        view_ptr container = children[0];
        _gutter = std::make_shared<gutter_t>();
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
        _minimap = std::make_shared<view_t>();
        _minimap->layout()->order = 15;
        container->add_child(_minimap);
        layout_sort(container->layout());
    }
    return _minimap;
}

void editor_view_t::update()
{
    // if (scroll_to != -1) {
    //     layout_item_ptr slo = scrollarea->layout();
    //     float diff = scroll_to - slo->scroll_y;
    //     if (diff * diff < 100) {
    //         scroll_to = -1;
    //         diff = 0;
    //     }
    //     slo->scroll_y += diff/40;

    //     layout_compute_absolute_position(layout());
    //     _state_hash = 0;
    // }

    panel_t::update();
}