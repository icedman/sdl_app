#include "app_view.h"
#include "commands_view.h"
#include "editor_view.h"
#include "explorer_view.h"
#include "splitter.h"
#include "statusbar.h"
#include "system.h"
#include "tabbar.h"
#include "text.h"

#include "app.h"
#include "explorer.h"

static app_t global_app;

static parse::grammar_ptr load_grammar(std::string path)
{
    Json::Value json = parse::loadJson(path);
    return parse::parse_grammar(json);
}

app_view_t::app_view_t()
    : view_t()
{
    view_ptr hc = std::make_shared<horizontal_container_t>();

    sidebar = std::make_shared<explorer_view_t>();
    sidebar->layout()->width = 300;
    hc->add_child(sidebar);

    view_ptr hsplit = std::make_shared<vertical_splitter_t>(sidebar.get(), hc.get());
    hc->add_child(hsplit);

    tabs = std::make_shared<tabbed_content_t>();
    hc->add_child(tabs);

    add_child(hc);

    statusbar = std::make_shared<statusbar_t>();

    line_column = statusbar->cast<statusbar_t>()->add_status("", 1, 0);
    doc_type = statusbar->cast<statusbar_t>()->add_status("", 1, 0);
    add_child(statusbar);

    tabs->cast<tabbed_content_t>()->tabbar()->on(EVT_ITEM_SELECT, [this](event_t& evt) {
        list_item_t* item = (list_item_t*)evt.source;

        if (evt.button == 1) {
            evt.cancelled = true;
            this->destroy_editor(app_t::instance()->findEditor(item->item_data.value));
            return true;
        }

        this->show_editor(app_t::instance()->openEditor(item->item_data.value));
        return true;
    });

    sidebar->cast<explorer_view_t>()->on(EVT_ITEM_SELECT, [this](event_t& evt) {
        list_item_t* item = (list_item_t*)evt.source;
        list_item_data_t d = item->item_data;
        fileitem_t* file = (fileitem_t*)d.data;
        // printf(">>%s\n", item->item_data.value.c_str());
        if (file && !file->isDirectory) {
            this->show_editor(app_t::instance()->openEditor(item->item_data.value));
        }
        return true;
    });

    cmd_actions = std::make_shared<commands_t>();
    cmd_files = std::make_shared<commands_files_t>();

    cmd_files->cast<explorer_view_t>()->on(EVT_ITEM_SELECT, [this](event_t& evt) {
        list_item_t* item = (list_item_t*)evt.source;
        list_item_data_t d = item->item_data;
        fileitem_t* file = (fileitem_t*)d.data;
        // printf(">>%s\n", item->item_data.value.c_str());
        if (file && !file->isDirectory) {
            this->show_editor(app_t::instance()->openEditor(item->item_data.value));
        }
        return true;
    });

    events_manager_t::instance()->on(EVT_KEY_SEQUENCE, [this](event_t& event) {
        this->handle_key_sequence(event);
        return true;
    });
}

void app_view_t::update()
{
    app_t* app = app_t::instance();
    for (auto e : app->editors) {
        if (!e->view) {
            create_editor(e);
        }
    }

    // update status bar
    editor_ptr editor = app->currentEditor;
    if (editor) {
        cursor_t cursor = editor->document.cursor();
        std::string lc = "Line ";
        lc += std::to_string(cursor.block()->lineNumber + 1);
        lc += ", Column ";
        lc += std::to_string(cursor.position() + 1);
        lc += "  ";
        line_column->cast<text_t>()->set_text(lc);
    }

    view_t::update();
}

void app_view_t::configure(int argc, char** argv)
{
    app_t::instance()->configure(argc, argv);
    std::string path = app_t::instance()->inputFile;
    if (path == "") {
        path = "./";
    }
    sidebar->cast<explorer_view_t>()->set_root_path(path);
    app_t::instance()->openEditor(path, true);
}

bool app_view_t::handle_key_sequence(event_t& event)
{
    app_t* app = app_t::instance();
    
    operation_e op = operationFromKeys(event.text);
    switch (op) {
    case POPUP_COMMANDS:
        show_files();
        break;
    
    case TOGGLE_SIDEBAR:
        sidebar->layout()->visible = !sidebar->layout()->visible;
        layout_clear_hash(layout(), 2);
        layout_request();
        break;
    
    case CLOSE:
        if (app->currentEditor) {
            destroy_editor(app->currentEditor);
        }
        break;
    }

    return false;
}

void app_view_t::show_actions()
{
    // if (cmd_actions->cast<commands_t>()->update_data()) {
    //     view_ptr _pm = popup_manager_t::instance();
    //     popup_manager_t* pm = _pm->cast<popup_manager_t>();
    //     pm->clear();
    //     pm->push_at(cmd_actions, {});
    // }
}

void app_view_t::show_files()
{
    if (cmd_files->cast<commands_t>()->update_data()) {
        layout_item_ptr lo = layout();
        view_ptr _pm = popup_manager_t::instance();
        popup_manager_t* pm = _pm->cast<popup_manager_t>();
        pm->clear();
        pm->push_at(cmd_files, { lo->render_rect.w / 2 - cmd_files->layout()->width / 2, 0, 0 });
    }
}

void app_view_t::show_editor(editor_ptr editor)
{
    view_t* view = (view_t*)(editor->view);
    if (!view)
        return;

    int mods = system_t::instance()->key_mods();
    if ((mods & K_MOD_CTRL) != K_MOD_CTRL) {
        for (auto c : tabs->cast<tabbed_content_t>()->content()->children) {
            c->set_visible(false);
        }
    }

    view->set_visible(true);
    set_focused(view);

    layout_clear_hash(layout(), 8);
    relayout();

    system_t::instance()->caffeinate();
}

void app_view_t::create_editor(editor_ptr editor)
{
    view_ptr view = std::make_shared<editor_view_t>();
    tabs->cast<tabbed_content_t>()->content()->add_child(view);

    editor_view_t* ev = view->cast<editor_view_t>();
    ev->editor = editor;
    editor->view = ev;

    editor->enableIndexer();

    ev->gutter();
    ev->minimap();
    ev->start_tasks();

    update_tabs();
    show_editor(editor);

    if (ev->editor && ev->editor->highlighter.lang) {
        doc_type->cast<text_t>()->set_text(" " + ev->editor->highlighter.lang->id + " ");
    }

    ev->on(EVT_FOCUS_IN, [this, ev](event_t& event) {
        app_t::instance()->currentEditor = ev->editor;
        if (ev->editor && ev->editor->highlighter.lang) {
            doc_type->cast<text_t>()->set_text(" " + ev->editor->highlighter.lang->id + " ");
        }
        return true;
    });
}

void app_view_t::destroy_editor(editor_ptr editor)
{
    if (!editor)
        return;

    set_focused(nullptr);
    set_hovered(nullptr);

    app_t* app = app_t::instance();

    if (app->editors.size() == 1) {
        system_t::instance()->quit();
        return;
    }

    if (editor->view) {
        view_t* view = (view_t*)editor->view;
        editor_view_t* ev = (editor_view_t*)view;
        ev->stop_tasks();

        tabs->cast<tabbed_content_t>()->content()->remove_child(view->ptr());
        app->closeEditor(editor);
    }

    update_tabs();
    show_editor(app->currentEditor);
}

void app_view_t::update_tabs()
{
    std::vector<list_item_data_t> tabbar_data;
    app_t* app = app_t::instance();
    for (auto e : app->editors) {
        std::string fname = e->document.fileName;
        if (fname == "") {
            fname = "untitled";
        }
        list_item_data_t d = {
            value : e->document.fullPath,
            text : fname
        };
        tabbar_data.push_back(d);
    }
    tabs->cast<tabbed_content_t>()->tabbar()->cast<tabbar_t>()->update_data(tabbar_data);

    layout_clear_hash(tabs->layout(), 4);
    tabs->relayout();
}