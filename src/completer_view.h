#ifndef COMPLETER_H
#define COMPLETER_H

#include "editor.h"
#include "popup.h"

struct editor_view_t;
struct completer_t : popup_t {
    completer_t(editor_view_t* e);

    DECLAR_VIEW_TYPE(CUSTOM, popup_t)
    std::string type_name() override { return "completer"; }

    bool update_data();
    void render(renderer_t* renderer) override;
    virtual bool handle_key_sequence(event_t& event);
    virtual bool handle_key_text(event_t& event);
    virtual bool handle_item_select(event_t& event);

    view_ptr list;

    std::string selected_word;

    editor_view_t* editor;
    cursor_t current_cursor;
};

#endif // COMPLETER_H