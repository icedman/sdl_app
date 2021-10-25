#ifndef SEARCH_VIEW_H
#define SEARCH_VIEW_H

#include "editor.h"
#include "popup.h"

struct editor_view_t;
struct search_view_t : popup_t {
    search_view_t(editor_view_t* e);

    DECLAR_VIEW_TYPE(CUSTOM, popup_t)
    std::string type_name() override { return "search_view"; }

    void render(renderer_t* renderer) override;
    virtual bool handle_key_sequence(event_t& event);
    virtual bool handle_key_text(event_t& event);

    bool update_data(std::string text);

    view_ptr input;

    editor_view_t* editor;
    cursor_t current_cursor;
};

#endif // SEARCH_VIEW_H