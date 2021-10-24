#ifndef INPUTTEXT_H
#define INPUTTEXT_H

#include "editor.h"
#include "rich_text.h"

struct input_text_t : rich_text_t {
    input_text_t();

    DECLAR_VIEW_TYPE(CUSTOM, rich_text_t)
    std::string type_name() override { return "input"; }

    virtual bool handle_key_sequence(event_t& event);
    virtual bool handle_key_text(event_t& event);
    virtual bool handle_mouse_down(event_t& event);
    virtual bool handle_mouse_move(event_t& event);

    // int estimated_cursor_y(cursor_t cursor);

    // void ensure_visible_cursor();
    // void scroll_to_cursor(cursor_t cursor);

    void prelayout() override;
    void render(renderer_t* renderer) override;

    int mouse_x;
    int mouse_y;

    std::string value();
    void set_value(std::string value);
};

#endif INPUTTEXT_H