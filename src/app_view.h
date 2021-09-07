#ifndef APP_VIEW_H
#define APP_VIEW_H

#include "view.h"
#include "editor.h"

struct app_view : vertical_container {
    app_view();

    view_item_ptr tabbar;
    view_item_ptr tabcontent;
    view_item_ptr explorer;
    view_item_ptr main;
    view_item_ptr menu;
    view_item_ptr statusbar;

    bool input_sequence(std::string sequence) override;
    void update() override;
    void create_editor_view(editor_ptr editor);
    void destroy_editor_view(editor_ptr editor);

    void show_editor(editor_ptr editor, bool sole = true);
};

#endif // APP_VIEW_H