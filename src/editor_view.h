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

    void request_highlight(block_ptr block);
    void start_tasks();
    void stop_tasks();

    view_ptr gutter();
    view_ptr minimap();
    view_ptr completer();
    view_ptr search();

    view_ptr _gutter;
    view_ptr _minimap;
    view_ptr _completer;
    view_ptr _search;

    point_t mouse_xy;

    task_ptr task;
};

#endif EDITOR_VIEW_H