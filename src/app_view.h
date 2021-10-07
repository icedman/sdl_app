#ifndef APP_VIEW_H
#define APP_VIEW_H

#include "editor.h"
#include "view.h"

struct app_view : vertical_container {
    app_view();

    DECLAR_VIEW_TYPE(CUSTOM, vertical_container)
    std::string type_name() override { return "app"; }

    view_item_ptr tabbar;
    view_item_ptr tabcontent;
    view_item_ptr explorer;
    view_item_ptr main;
    view_item_ptr menu;
    view_item_ptr statusbar;
    view_item_ptr explorer_main_splitter;

    view_item_ptr search;
    view_item_ptr commands;
    view_item_ptr popups;

    void prelayout() override;
    bool input_sequence(std::string sequence) override;
    void update(int millis) override;
    void create_editor_view(editor_ptr editor);
    void destroy_editor_view(editor_ptr editor);

    void show_editor(editor_ptr editor, bool sole = true);
    void close_popups();

    void setup_style();
};

#endif // APP_VIEW_H