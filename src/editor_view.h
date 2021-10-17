#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include "editor.h"
#include "rich_text.h"

struct editor_view_t : rich_text_t {
    editor_view_t();

    DECLAR_VIEW_TYPE(CUSTOM, rich_text_t)

    virtual bool handle_key_sequence(event_t& event);
    virtual bool handle_key_text(event_t& event);
    virtual bool handle_mouse_down(event_t& event);
    virtual bool handle_mouse_move(event_t& event);

    void update() override;
    void ensure_visible_cursor();
    void scroll_to_cursor(cursor_t cursor);

    view_ptr gutter();
    view_ptr minimap();

    view_ptr _gutter;
    view_ptr _minimap;

    int mouse_x;
    int mouse_y;
    int scroll_to;
};

#endif EDITOR_VIEW_H