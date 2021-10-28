#ifndef PROMPT_VIEW_H
#define PROMPT_VIEW_H

#include "editor.h"
#include "popup.h"

struct editor_view_t;
struct prompt_view_t : popup_t {
    prompt_view_t(editor_view_t* e);

    DECLAR_VIEW_TYPE(CUSTOM, popup_t)
    std::string type_name() override { return "prompt_view"; }

    virtual void render(renderer_t* renderer) override;
    virtual bool handle_key_sequence(event_t& event);
    virtual bool handle_key_text(event_t& event);

    bool update_data(std::string text);
    virtual bool commit();

    view_ptr input;

    editor_view_t* editor;
    cursor_t current_cursor;
};

struct prompt_filename_t : prompt_view_t {
    prompt_filename_t(editor_view_t* e)
        : prompt_view_t(e)
    {
    }

    virtual bool commit() override;
};

#endif // PROMPT_VIEW__H