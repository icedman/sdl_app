#include "app_view.h"
#include "editor_view.h"
#include "explorer_view.h"
#include "splitter.h"
#include "tabbar.h"

#include "app.h"
#include "editor.h"
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

    std::vector<list_item_data_t> tabbar_data;
    for (int i = 0; i < 4; i++) {
        list_item_data_t d = {
            value : "Item " + std::to_string(i),
            text : "Item " + std::to_string(i)
        };
        tabbar_data.push_back(d);
    }
    tabs->cast<tabbed_content_t>()->tabbar()->cast<tabbar_t>()->update_data(tabbar_data);

    add_child(hc);
}

void app_view_t::configure(int argc, char** argv)
{
    app_t::instance()->configure(argc, argv);

    std::string filename = app_t::instance()->inputFile;
    std::string themeFile = app_t::instance()->themeName; //"./tests/themes/dracula.json";

    if (filename == "") {
        filename = "./";
    }

    sidebar->cast<explorer_view_t>()->set_root_path(filename);

    view_ptr view = std::make_shared<editor_view_t>();
    tabs->cast<tabbed_content_t>()->content()->add_child(view);

    editor_view_t* ev = view->cast<editor_view_t>();

    ev->gutter();
    ev->minimap()->layout()->width = 80;

    editor_ptr editor = ev->editor;
    editor->highlighter.lang = std::make_shared<language_info_t>();
    editor->highlighter.lang->grammar = load_grammar("./tests/syntaxes/c.json");
    Json::Value root = parse::loadJson(themeFile);
    editor->highlighter.theme = parse_theme(root);

    editor->pushOp("OPEN", filename);
    editor->runAllOps();

    editor->name = "editor:";
    editor->name += filename;
}