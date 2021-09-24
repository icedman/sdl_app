#ifndef APP_VIEW_H
#define APP_VIEW_H

#include "editor.h"
#include "view.h"

struct app_view : vertical_container {
    app_view();

    view_item_ptr tabbar;
    view_item_ptr tabcontent;
    view_item_ptr explorer;
    view_item_ptr main;
    view_item_ptr menu;
    view_item_ptr statusbar;

    view_item_ptr search;
    view_item_ptr commands;
    view_item_ptr popups;

    bool input_sequence(std::string sequence) override;
    void update() override;
    void create_editor_view(editor_ptr editor);
    void destroy_editor_view(editor_ptr editor);

    void show_editor(editor_ptr editor, bool sole = true);

    void setup_style();
};

#endif // APP_VIEW_H