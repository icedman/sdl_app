#ifndef APP_VIEW_H
#define APP_VIEW_H

#include "editor.h"
#include "view.h"

struct app_view_t : view_t {
    app_view_t();

    DECLAR_VIEW_TYPE(CUSTOM, view_t)
    std::string type_name() override { return "app"; }

    void configure(int argc, char** argv);
    void update() override;

    virtual bool handle_key_sequence(event_t& event);

    void show_editor(editor_ptr editor);
    void create_editor(editor_ptr editor);
    void destroy_editor(editor_ptr editor);

    void update_tabs();
    void show_actions();
    void show_files();

    view_ptr sidebar;
    view_ptr tabs;
    view_ptr statusbar;
    view_ptr cmd_actions;
    view_ptr cmd_files;

    view_ptr fps;
};

#endif // APP_VIEW_H