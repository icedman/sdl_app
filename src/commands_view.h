#ifndef COMMAND_PVIEW_
#define COMMAND_PVIEW_

#include "popup.h"
#include "editor.h"

struct editor_view_t;
struct commands_t : popup_t
{
    commands_t();

    DECLAR_VIEW_TYPE(CUSTOM, popup_t)
    std::string type_name() override { return "command_palette"; }

    bool update_data();
    void render(renderer_t *renderer) override;
    virtual bool handle_key_sequence(event_t& event);
    virtual bool handle_key_text(event_t& event);
    virtual bool handle_item_select(event_t& event);

    view_ptr input;
    view_ptr list;
};

#endif // COMMAND_VIEW_H