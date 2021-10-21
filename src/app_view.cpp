#include "app_view.h"
#include "editor_view.h"
#include "explorer_view.h"
#include "splitter.h"
#include "tabbar.h"
#include "statusbar.h"
#include "system.h"
#include "text.h"

#include "app.h"
#include "grammar.h"
#include "parse.h"

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

    view_ptr hsplit = std::make_shared<vertical_splitter_t>(sidebar, hc);
    hc->add_child(hsplit);

    tabs = std::make_shared<tabbed_content_t>();
    hc->add_child(tabs);

    add_child(hc);

    statusbar = std::make_shared<statusbar_t>();

    fps = statusbar->cast<statusbar_t>()->add_status("", 0, 0);
    add_child(statusbar);

    tabs->cast<tabbed_content_t>()->tabbar()->on(EVT_ITEM_SELECT, [this](event_t& evt) {
        list_item_t *item = (list_item_t*)evt.source;

        if (evt.button == 1) {
            evt.cancelled = true;
            this->destroy_editor(app_t::instance()->findEditor(item->item_data.value));
            return true;
        }

        this->show_editor(app_t::instance()->openEditor(item->item_data.value));
        return true;
    });

    sidebar->cast<explorer_view_t>()->on(EVT_ITEM_SELECT, [this](event_t& evt) {
        list_item_t *item = (list_item_t*)evt.source;
        printf(">>%s\n", item->item_data.value.c_str());
        this->show_editor(app_t::instance()->openEditor(item->item_data.value));
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
}

void app_view_t::configure(int argc, char** argv)
{
    app_t::instance()->configure(argc, argv);
    sidebar->cast<explorer_view_t>()->set_root_path(app_t::instance()->inputFile);
    app_t::instance()->openEditor(app_t::instance()->inputFile, true);
}

void app_view_t::show_editor(editor_ptr editor)
{
    view_t *view = (view_t*)(editor->view);
    if (!view) return;

    int mods = system_t::instance()->key_mods();
    if ((mods & K_MOD_CTRL) != K_MOD_CTRL) {
        for(auto c : tabs->cast<tabbed_content_t>()->content()->children) {
            c->layout()->visible = false;
        }
    }

    view->layout()->visible = true;
    set_focused(view);

    layout_clear_hash(layout(), 8);
    relayout();
}

void app_view_t::create_editor(editor_ptr editor)
{
    view_ptr view = std::make_shared<editor_view_t>();
    tabs->cast<tabbed_content_t>()->content()->add_child(view);

    editor_view_t* ev = view->cast<editor_view_t>();
    ev->editor = editor;
    editor->view = ev;

    ev->gutter();
    ev->minimap();

    // editor->highlighter.lang = std::make_shared<language_info_t>();
    // editor->highlighter.lang->grammar = load_grammar("./tests/syntaxes/c.json");
    // Json::Value root = parse::loadJson(themeFile);
    // editor->highlighter.theme = parse_theme(root);
    // editor->pushOp("OPEN", filename);
    // editor->runAllOps();
    // editor->name = "editor:";
    // editor->name += filename;

    update_tabs();
    show_editor(editor);
}

void app_view_t::destroy_editor(editor_ptr editor)
{
    if (!editor) return;

    set_focused(nullptr);
    set_hovered(nullptr);

    app_t* app = app_t::instance();

    if (app->editors.size() == 1) {
        system_t::instance()->quit();
        return;
    }

    if (editor->view) {
        view_t* view = (view_t*)editor->view;
        editor_view_t *ev = (editor_view_t*)view;
        ev->cleanup();

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