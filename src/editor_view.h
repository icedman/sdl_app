#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include "editor.h"
#include "rich_text.h"
#include "tasks.h"

struct editor_view_t;
struct highlighter_task_t : task_t {
    highlighter_task_t(editor_view_t* editor);

    bool run(int limit) override;

    editor_view_t* editor;
    block_list hl;
};

struct editor_view_t : rich_text_t {
    editor_view_t();

    DECLAR_VIEW_TYPE(CUSTOM, rich_text_t)
    std::string type_name() override { return "editor"; }

    virtual bool handle_key_sequence(event_t& event);
    virtual bool handle_key_text(event_t& event);
    virtual bool handle_mouse_down(event_t& event);
    virtual bool handle_mouse_move(event_t& event);

    int cursor_x(cursor_t cursor);
    int cursor_y(cursor_t cursor);
    point_t cursor_xy(cursor_t cursor);

    void ensure_visible_cursor();
    void scroll_to_cursor(cursor_t cursor);

    void request_highlight(block_ptr block);
    void start_tasks();
    void stop_tasks();

    view_ptr gutter();
    view_ptr minimap();
    view_ptr completer();

    view_ptr _gutter;
    view_ptr _minimap;
    view_ptr _completer;

    point_t mouse_xy;
    int scroll_to_x;
    int scroll_to_y;

    task_ptr task;
};

#endif EDITOR_VIEW_H