#include "app_view.h"
#include "renderer.h"

#include "app.h"
#include "operation.h"
#include "statusbar.h"

#include "editor_view.h"
#include "explorer_view.h"
#include "statusbar_view.h"
#include "tabbar_view.h"

#include "style.h"

extern color_info_t darker(color_info_t c, int x);

app_view::app_view()
    : vertical_container()
{
    app_t* app = app_t::instance();
    app->view = this;

    view_item_ptr content = std::make_shared<horizontal_container>();

    interactive = true;

    layout_item_ptr lo = layout();
    // lo->margin = 0;

    explorer = std::make_shared<explorer_view>();

    main = std::make_shared<vertical_container>();
    tabbar = std::make_shared<app_tabbar_view>();

    tabcontent = std::make_shared<horizontal_container>();

    main->add_child(tabbar);
    main->add_child(tabcontent);

    content->add_child(explorer);
    content->add_child(main);

    menu = std::make_shared<horizontal_container>();
    menu->layout()->height = 24;

    if (Renderer::instance()->is_terminal()) {
        menu->layout()->height = 1;
    }

    statusbar = std::make_shared<statusbar_view>();

    // add_child(menu);
    add_child(content);
    add_child(statusbar);

    view_set_root(this);

    on(EVT_KEY_SEQUENCE, [this](event_t& evt) {
        if (this->input_sequence(evt.text)) {
            evt.cancelled = true;
        }
        return true;
    });

    setup_style();
}

bool app_view::input_sequence(std::string keySequence)
{
    operation_e op = operationFromKeys(keySequence);

    app_t* app = app_t::instance();

    app->log("keySequence %s", keySequence.c_str());

    switch (op) {
    case QUIT:
        Renderer::instance()->quit();
        return true;

    case NEW_TAB: {
        app->newEditor(); // focus
        return true;
    }

    case MOVE_FOCUS_LEFT:
        view_set_focused(explorer.get());
        return true;
    case MOVE_FOCUS_UP:
        view_set_focused(tabbar.get());
        return true;
    case MOVE_FOCUS_RIGHT:
    case MOVE_FOCUS_DOWN:
        show_editor(app_t::instance()->currentEditor);
        return true;

    case CANCEL:
        show_editor(app_t::instance()->currentEditor);
        break;

    case TAB_1:
    case TAB_2:
    case TAB_3:
    case TAB_4:
    case TAB_5:
    case TAB_6:
    case TAB_7:
    case TAB_8:
    case TAB_9: {
        int tab = op - TAB_1;
        if (tab < app->editors.size()) {
            show_editor(app->editors[tab], true);
        }
        return true;
    }

    case SAVE: {
        if (statusbar_t::instance()) {
            statusbar_t::instance()->setStatus("saved...");
            app_t::instance()->currentEditor->document.save();
        }
        return true;
    }
    case CLOSE:
        destroy_editor_view(app->currentEditor);
        return true;
    }

    // printf("%s\n", keySequence.c_str());
    return false;
}

void app_view::update()
{
    app_t* app = app_t::instance();
    for (auto e : app->editors) {
        if (!e->view) {
            create_editor_view(e);
        }
    }

    view_item::update();
}

void app_view::create_editor_view(editor_ptr editor)
{
    app_t* app = app_t::instance();
    view_item_ptr ev = std::make_shared<editor_view>();
    ((editor_view*)ev.get())->editor = editor;
    editor->view = ev.get();
    tabcontent->add_child(ev);

    layout_request();
    view_set_focused((view_item*)editor->view);
}

void app_view::destroy_editor_view(editor_ptr editor)
{
    if (!editor) {
        return;
    }

    editor->highlighter.pause();

    app_t* app = app_t::instance();

    view_set_focused(0);
    for (auto tab : tabcontent->_views) {
        editor_view* ev = ((editor_view*)(tab.get()));
        if (ev->editor == editor) {
            tabcontent->remove_child(tab);
            break;
        }
        if (ev->layout()->visible) {
            view_set_focused(ev);
        }
    }

    app->closeEditor(editor);
    layout_request();

    if (!app->editors.size()) {
        Renderer::instance()->quit();
        return;
    }

    if (!view_get_focused()) {
        for (auto tab : tabcontent->_views) {
            editor_view* ev = ((editor_view*)(tab.get()));
            if (ev->layout()->visible) {
                show_editor(ev->editor);
                break;
            }
        }
        if (!view_get_focused()) {
            editor_view* ev = (editor_view*)(app->editors[0]->view);
            ev->layout()->visible = true;
            show_editor(ev->editor);
        }
    }
}

void app_view::show_editor(editor_ptr editor, bool sole)
{
    if (sole) {
        for (auto e : app_t::instance()->editors) {
            view_item* v = (view_item*)(e->view);
            if (v) {
                v->layout()->visible = false;
            }
        }
    }

    if (editor->view) {
        view_set_focused((view_item*)(editor->view));
        ((view_item*)(editor->view))->layout()->visible = true;
        app_t::instance()->currentEditor = editor;
        layout_request();
    }
}

void app_view::setup_style()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    // std::string font;
    // bool italic;
    // bool bold;
    // color_info_t fg;
    // color_info_t bg;
    // bool filled;
    // int border;
    // int border_radius;

    view_style_t vs_default = {
        font : "editor",
        italic : false,
        bold : false,
        fg : Renderer::instance()->color_for_index(comment.foreground.index),
        bg : Renderer::instance()->color_for_index(app->bgApp),
        filled : false,
        border : 0,
        border_radius : 0,

    };
    view_style_t vs = vs_default;
    view_style_register(vs_default, "default");

    view_style_register(vs, "gutter");

    vs.bg = darker(Renderer::instance()->color_for_index(app->bgApp), 5);
    view_style_register(vs, "explorer");
}