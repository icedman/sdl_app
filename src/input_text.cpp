#include "input_text.h"
#include "system.h"

input_text_t::input_text_t()
    : rich_text_t()
    , mouse_x(0)
    , mouse_y(0)
{
    can_focus = true;
    draw_cursors = true;
    editor->singleLineEdit = true;

    editor->highlighter.lang = std::make_shared<language_info_t>();
    editor->highlighter.lang->grammar = nullptr;
    editor->highlighter.theme = nullptr;
    editor->pushOp("OPEN", "");
    editor->runAllOps();

    remove_child(v_scroll);
    remove_child(bottom);

    v_scroll->disabled = true;

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
    on(EVT_FOCUS_IN, [this](event_t& event) {
        if (event.source == this) {
            this->draw_cursors = true;
            this->update_blocks();
        }
        return true;
    });
    on(EVT_FOCUS_OUT, [this](event_t& event) {
        if (event.source == this) {
            this->draw_cursors = false;
            this->update_blocks();
        }
        return true;
    });
}

void input_text_t::prelayout()
{
    layout()->margin = 4;
    layout()->height = font()->height + layout()->margin * 4;
    rich_text_t::prelayout();
}

void input_text_t::render(renderer_t* renderer)
{
    render_frame(renderer);
    rich_text_t::render(renderer);
}

bool input_text_t::handle_key_sequence(event_t& event)
{
    operation_e op = operationFromKeys(event.text);

    if (!editor) {
        return false;
    }

    cursor_t cursor = editor->document.cursor();

    // printf("%s %s\n", nameFromOperation(op).c_str(), event.text.c_str());

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

    update_blocks();
    // ensure_visible_cursor();
    relayout_virtual_blocks();
    return true;
}

bool input_text_t::handle_key_text(event_t& event)
{
    int mods = system_t::instance()->key_mods();
    if (mods & K_MOD_CTRL || mods & K_MOD_ALT)
        return true;

    editor->pushOp(INSERT, event.text);
    editor->runAllOps();
    update_blocks();
    // ensure_visible_cursor();
    relayout_virtual_blocks();

    return true;
}

bool input_text_t::handle_mouse_down(event_t& event)
{
    layout_item_ptr lo = layout();

    point_t p = { event.x, event.y };
    view_ptr text_block;
    for (auto c : subcontent->children) {
        layout_item_ptr clo = c->layout();
        if (!clo->visible)
            continue;
        rect_t r = clo->render_rect;
        if (r.w < lo->render_rect.w) {
            r.w = lo->render_rect.w;
        }
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
    // ensure_visible_cursor();
    relayout_virtual_blocks();
    return true;
}

bool input_text_t::handle_mouse_move(event_t& event)
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

std::string input_text_t::value()
{
    return editor->document.blocks.back()->text();
}

void input_text_t::set_value(std::string value)
{
    editor->document.blocks.back()->setText(value);
}