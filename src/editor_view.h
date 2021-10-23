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

    int estimated_cursor_y(cursor_t cursor);
    
    void ensure_visible_cursor();
    void scroll_to_cursor(cursor_t cursor);

    void request_highlight(block_ptr block);
    void cleanup();

    view_ptr gutter();
    view_ptr minimap();

    view_ptr _gutter;
    view_ptr _minimap;

    int mouse_x;
    int mouse_y;
    int scroll_to;

    task_ptr hl_task;
};

#endif EDITOR_VIEW_H