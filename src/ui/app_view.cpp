#include "app_view.h"
#include "renderer.h"

#include "app.h"
#include "operation.h"
#include "statusbar.h"

#include "editor_view.h"
#include "explorer_view.h"
#include "statusbar_view.h"
#include "tabbar.h"

app_view::app_view()
    : vertical_container()
{
    app_t *app = app_t::instance();
    app->view = this;

    view_item_ptr content = std::make_shared<horizontal_container>();

    interactive = true;

    layout_item_ptr lo = layout();
    lo->margin = 0;

    explorer = std::make_shared<explorer_view>();  

    main = std::make_shared<vertical_container>();    
    tabbar = std::make_shared<tabbar_view>();

    tabcontent = std::make_shared<horizontal_container>();

    main->add_child(tabbar);
    main->add_child(tabcontent);

    content->add_child(explorer);
    content->add_child(main);

    menu = std::make_shared<horizontal_container>();
    menu->layout()->height = 24;

    statusbar = std::make_shared<statusbar_view>();

    add_child(menu);
    add_child(content);
    add_child(statusbar);

    view_set_root(this);

    on(EVT_KEY_SEQUENCE, [this](event_t& evt) {
        if (this->input_sequence(evt.text)) {
            evt.cancelled = true;
        }
        return true;
    });
}

bool app_view::input_sequence(std::string keySequence)
{
    operation_e op = operationFromKeys(keySequence);

    app_t *app = app_t::instance();

    switch(op) {
    case QUIT:
        ren_quit();
        return true;
    case NEW_TAB: {
        app->newEditor(); // focus
        return true;
    }
    case SAVE: {
        if (statusbar_t::instance()) {
            statusbar_t::instance()->setStatus("saved...");
        }
        return true;
    }
    case CLOSE:
        destroy_editor_view(app->currentEditor);
        return true;
    }

    printf("%s\n", keySequence.c_str());
    return false;
}

void app_view::update()
{
    app_t *app = app_t::instance();
    for(auto e : app->editors) {
        if (!e->view) {
            create_editor_view(e);
            layout_request();
        }
    }

    view_item::update();
}

void app_view::create_editor_view(editor_ptr editor)
{
    app_t *app = app_t::instance();
    view_item_ptr ev = std::make_shared<editor_view>();  
    ((editor_view*)ev.get())->editor = editor;
    editor->view = ev.get();
    tabcontent->add_child(ev);

    view_set_focused((view_item*)editor->view);
}

void app_view::destroy_editor_view(editor_ptr editor)
{
    if (!editor) return;

    app_t *app = app_t::instance();

    view_set_focused(0);
    for(auto tab : tabcontent->_views) {
        editor_view *ev = ((editor_view*)(tab.get()));
        if (ev->editor == editor) {
            tabcontent->remove_child(tab);
            break;
        }
        view_set_focused(ev);
    }

    app->closeEditor(editor);
    layout_request();

    if (!app->editors.size()) {
        ren_quit();
        return;
    }

    if (!view_get_focused()) {
        view_set_focused((view_item*)(app->editors[0]->view));
    }
}

void app_view::show_editor(editor_ptr editor, bool sole)
{
    if (sole) {
        for(auto e : app_t::instance()->editors) {
            view_item *v = (view_item*)(e->view);
            if (v) {
                v->layout()->visible = false;
            }
        }
    }

    if (editor->view) {
        view_set_focused((view_item*)(editor->view));
        ((view_item*)(editor->view))->layout()->visible = true;
    }
}