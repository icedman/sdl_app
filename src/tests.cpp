#include "button.h"
#include "events.h"
#include "image.h"
#include "layout.h"
#include "list.h"
#include "panel.h"
#include "popup.h"
#include "scrollarea.h"
#include "scrollbar.h"
#include "splitter.h"
#include "system.h"
#include "tabbar.h"
#include "text.h"
#include "text_block.h"
#include "view.h"

#include "grammar.h"
#include "parse.h"
#include "reader.h"
#include "theme.h"

#include "app_view.h"
#include "editor_view.h"
#include "explorer.h"
#include "explorer_view.h"
#include "input_text.h"
#include "rich_text.h"

#define TEST test7

static bool render_layout = false;
explorer_t explorer;

view_ptr test1(int argc, char** argv); // flex layout
view_ptr test2(int argc, char** argv); // wrap
view_ptr test2b(int argc, char** argv); // wrap & frame style
view_ptr test3(int argc, char** argv); // list, splitter
view_ptr test4(int argc, char** argv); // fit to children
view_ptr test5(int argc, char** argv); // text block
view_ptr test6(int argc, char** argv); // rich text editor
view_ptr test7(int argc, char** argv); // app

std::string inputFile;
std::string themeFile;

void render_layout_item(renderer_t* renderer, layout_item_ptr item)
{
    if (!render_layout)
        return;

    if (!item->visible || item->render_rect.w < 0 || item->render_rect.h < 0)
        return;

    renderer->draw_rect(item->render_rect, { 255, 255, 255 }, false, 1.0f);

    char tmp[32];
    sprintf(tmp, "%x", item->state_hash);
    if (item->children.size())
        renderer->draw_text(NULL, (char*)tmp, item->render_rect.x, item->render_rect.y, { 255, 255, 255 });

    for (auto child : item->children) {
        render_layout_item(renderer, child);
    }
}

view_ptr test(int argc, char** argv)
{
    themeFile = "./tests/themes/dracula.json";

    const char* argTheme = 0;
    const char* argScript = 0;
    const char* defaultTheme = "Monokai";

    std::string lastArg;
    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], "-t") == 0) {
            argTheme = argv[i + 1];
            lastArg = argTheme;
            themeFile = argTheme;
        }
    }

    if (argc > 1) {
        inputFile = argv[argc - 1];
        if (inputFile == lastArg) {
            inputFile = "";
        }
    }

    return TEST(argc, argv);
}

parse::grammar_ptr load(std::string path)
{
    Json::Value json = parse::loadJson(path);
    return parse::parse_grammar(json);
}

view_ptr test7(int argc, char** argv)
{
    view_ptr root_view = std::make_shared<app_view_t>();
    root_view->cast<app_view_t>()->configure(argc, argv);
    return root_view;
}

view_ptr test6(int argc, char** argv)
{
    std::string filename = "./src/main.cpp";
    if (inputFile.length()) {
        filename = inputFile;
    }

    // std::string filename = "./tests/test.c";
    // std::string filename = "./tests/utf8_test2.txt";
    // std::string filename = "./tests/tinywl.c";
    // std::string filename = "./tests/sqlite3.c";

    view_ptr root_view = std::make_shared<horizontal_container_t>();

    view_ptr sidebar = std::make_shared<explorer_view_t>();
    sidebar->layout()->width = 300;
    sidebar->cast<explorer_view_t>()->set_root_path("./src");

    root_view->layout()->margin_left = 20;
    view_ptr view = std::make_shared<editor_view_t>();
    editor_view_t* ev = view->cast<editor_view_t>();

    ev->gutter();
    // ev->minimap()->layout()->width = 80;

    editor_ptr editor = ev->editor;
    editor->highlighter.lang = std::make_shared<language_info_t>();
    editor->highlighter.lang->grammar = load("./tests/syntaxes/c.json");
    Json::Value root = parse::loadJson(themeFile);
    editor->highlighter.theme = parse_theme(root);

    editor->pushOp("OPEN", filename);
    editor->runAllOps();

    editor->name = "editor:";
    editor->name += filename;

    view_ptr vsplitter = std::make_shared<vertical_splitter_t>(sidebar.get(), root_view.get());

    root_view->add_child(sidebar);
    root_view->add_child(vsplitter);
    root_view->add_child(view);

    view_t::set_focused(view.get());

    return root_view;
}

view_ptr test5(int argc, char** argv)
{
    bool wrapped = true;

    parse::grammar_ptr gm;
    gm = load("./libs/tm-parser/test-cases/themes/syntaxes/c.json");

    // Json::Value root = parse::loadJson("./libs/tm-parser/test-cases/themes/dark_vs.json");
    Json::Value root = parse::loadJson("./libs/tm-parser/test-cases/themes/dracula.json");
    theme_ptr theme = parse_theme(root);

    view_ptr view = std::make_shared<panel_t>();
    view->layout()->margin = 20;

    view_ptr block = std::make_shared<text_block_t>();
    block->cast<text_block_t>()->wrapped = wrapped;

    std::string code = "extern \"C\" int main(int argc, char** argv);";
    const char* first = code.c_str();
    const char* last = first + code.length();
    std::map<size_t, scope::scope_t> scopes;

    parse::stack_ptr parser_state = gm->seed();
    parser_state = parse::parse(first, last, parser_state, scopes, true);

    std::vector<text_span_t> spans;

    std::map<size_t, scope::scope_t>::iterator it = scopes.begin();
    size_t n = 0;
    size_t l = code.length();
    while (it != scopes.end()) {
        size_t n = it->first;
        scope::scope_t scope = it->second;
        style_t s = theme->styles_for_scope(scope);

        text_span_t ts = {
            .start = (int)n,
            .length = (int)(l - n),
            .fg = { 255 * s.foreground.red, 255 * s.foreground.green, 255 * s.foreground.blue, 0 },
            .bg = { 0, 0, 0, 0 },
            .bold = false,
            .italic = s.italic,
            .underline = false,
            .caret = 0
        };

        // printf("%f %f %f %f\n", s.foreground.red, s.foreground.green, s.foreground.blue, s.foreground.alpha);
        spans.push_back(ts);
        it++;

        // std::string scopeName(scope);
        // printf("%s\n", scopeName.c_str());
    }

    // struct text_span_t {
    //     int start;
    //     int length;
    //     color_t fg;
    //     color_t bg;
    //     bool bold;
    //     bool italic;
    //     bool underline;
    //     int caret;
    // };

    spans.push_back({ 2, 15, { 0, 0, 0, 0 }, { 150, 150, 150, 60 }, false, false, false, 0 });

    // spans.clear();

    text_span_t* prev = NULL;
    for (auto& s : spans) {
        if (prev) {
            prev->length = s.start - prev->start;
        }
        prev = &s;
    }

    block->cast<text_block_t>()->_text_spans = spans;
    block->cast<text_block_t>()->set_text(code);
    view->cast<panel_t>()->content()->add_child(block);
    view->cast<panel_t>()->content()->layout()->fit_children_x = !wrapped;

    text_block_t* text_block = block->cast<text_block_t>();
    block->on(EVT_KEY_TEXT, [text_block](event_t& event) {
        if (event.text == " ") {
            text_block->set_text("");
        } else {
            text_block->set_text(text_block->text() + event.text);
        }
        return true;
    });

    view_ptr input = std::make_shared<input_text_t>();
    input->cast<input_text_t>()->set_value("hello world");
    view->cast<panel_t>()->content()->add_child(input);
    view->cast<panel_t>()->content()->add_child(std::make_shared<input_text_t>());

    return view;
}

view_ptr test4(int argc, char** argv)
{
    view_ptr view = std::make_shared<view_t>();
    view->layout()->margin = 20;

    view_ptr content = view;

    view_ptr vc = std::make_shared<vertical_container_t>();
    {
        view_ptr button = std::make_shared<button_t>();
        std::string text = "Button";
        text_t* button_text = button->cast<button_t>()->text->cast<text_t>();
        button_text->set_text(text);
        vc->add_child(button);
    }

    view_ptr hc = std::make_shared<horizontal_container_t>();
    {
        view_ptr button = std::make_shared<button_t>();
        std::string text = "Button";
        text_t* button_text = button->cast<button_t>()->text->cast<text_t>();
        button_text->set_text(text);
        hc->add_child(button);
    }

    view_ptr sp = std::make_shared<horizontal_splitter_t>(vc.get(), content.get());
    content->add_child(vc);
    content->add_child(sp);
    content->add_child(hc);

    return view;
}

view_ptr menu;
view_ptr submenu;
view_ptr create_submenu();

view_ptr create_popup()
{
    view_ptr popup = std::make_shared<popup_t>();

    view_ptr btn = std::make_shared<button_t>("hello 1");
    popup->cast<popup_t>()->content()->add_child(btn);
    btn->cast<button_t>()->on(EVT_MOUSE_CLICK, [btn](event_t& evt) {
        popup_manager_t::instance()->cast<popup_manager_t>()->push_at(create_submenu(), btn->layout()->render_rect, POPUP_DIRECTION_RIGHT);
        return true;
    });

    btn = std::make_shared<button_t>("hello 2");
    popup->cast<popup_t>()->content()->add_child(btn);
    btn->cast<button_t>()->on(EVT_MOUSE_CLICK, [btn](event_t& evt) {
        popup_manager_t::instance()->cast<popup_manager_t>()->push_at(create_submenu(), btn->layout()->render_rect, POPUP_DIRECTION_RIGHT);
        return true;
    });

    popup->layout()->width = 300;
    popup->layout()->height = 300;
    return popup;
}

view_ptr create_menu()
{
    if (menu)
        return menu;
    menu = create_popup();
    return menu;
}

view_ptr create_submenu()
{
    if (submenu)
        return submenu;
    submenu = create_popup();
    return submenu;
}

view_ptr test3(int argc, char** argv)
{
    view_ptr view = std::make_shared<view_t>();
    view->layout()->margin = 20;

    view_ptr tabbar = std::make_shared<tabbar_t>();
    view->add_child(tabbar);

    view_ptr top = std::make_shared<vertical_container_t>();
    view->add_child(top);

    view_ptr text = std::make_shared<text_t>();
    text->cast<text_t>()->set_text("Hello World");

    std::vector<text_span_t> spans;
    spans.push_back({
        1,
        4,
        { 100, 0, 255 },
        bold : true,
        italic : true,
        underline : true
    });
    text->cast<text_t>()->_text_spans = spans;
    top->add_child(text);
    top->layout()->grow = 1;

    text->can_focus = true;
    text_t* _text = text->cast<text_t>();
    text->on(EVT_KEY_TEXT, [_text](event_t& event) {
        std::string t = event.text + event.text + event.text;
        if (event.text == " ") {
            _text->set_text("");
        } else {
            _text->set_text(t);
        }
        return true;
    });

    view_ptr toolbar = std::make_shared<horizontal_container_t>();
    top->add_child(toolbar);
    toolbar->layout()->height = 40;
    toolbar->layout()->justify = LAYOUT_JUSTIFY_SPACE_BETWEEN;

    // view_ptr icon = std::make_shared<image_view_t>();
    // icon->cast<image_view_t>()->load_icon("./tests/icon.svg", 140, 140);
    // toolbar->add_child(icon);

    std::vector<list_item_data_t> tabbar_data;
    for (int i = 0; i < 8; i++) {
        list_item_data_t d = {
            value : "List Item " + std::to_string(i),
            text : "List Item " + std::to_string(i)
        };
        tabbar_data.push_back(d);
    }
    tabbar->cast<tabbar_t>()->update_data(tabbar_data);

    view_ptr vsplitter = std::make_shared<horizontal_splitter_t>();
    view->add_child(vsplitter);

    view_ptr panel = std::make_shared<panel_t>();

    view_ptr hcontainer = std::make_shared<horizontal_container_t>();

    view->add_child(hcontainer);
    hcontainer->layout()->grow = 7;

    view_ptr hsplitter = std::make_shared<vertical_splitter_t>();

    view_ptr list = std::make_shared<list_t>();
    list->layout()->grow = 1;
    hcontainer->add_child(list);
    hcontainer->add_child(hsplitter);
    hcontainer->add_child(panel);
    panel->layout()->grow = 4;

    hsplitter->cast<splitter_t>()->container = hcontainer.get();
    hsplitter->cast<splitter_t>()->target = panel.get();

    vsplitter->cast<splitter_t>()->container = view.get();
    vsplitter->cast<splitter_t>()->target = hcontainer.get();

    int item_count = 100;
    for (int i = 0; i < item_count; i++) {
        view_ptr text = std::make_shared<text_t>();
        text->cast<text_t>()->set_text("Hello World " + std::to_string(i));
        panel->cast<panel_t>()->content()->add_child(text);
    }

    std::vector<list_item_data_t> data;
    for (int i = 0; i < item_count * 100; i++) {
        list_item_data_t d = {
            value : "List Item " + std::to_string(i),
            text : "List Item " + std::to_string(i)
        };
        data.push_back(d);
    }
    list->cast<list_t>()->update_data(data);

    for (int i = 0; i < 8; i++) {
        view_ptr button = std::make_shared<button_t>();
        std::string text = "Button " + std::to_string(i);
        text_t* button_text = button->cast<button_t>()->text->cast<text_t>();
        button_text->set_text(text);
        toolbar->add_child(button);

        if (i == 0) {
            button_text->on(EVT_KEY_TEXT, [button_text](event_t& event) {
                if (event.text == " ") {
                    button_text->set_text("");
                } else {
                    button_text->set_text(button_text->text() + event.text);
                }
                return true;
            });

            button->on(EVT_MOUSE_DOWN, [button_text](event_t& event) {
                popup_manager_t::instance()->cast<popup_manager_t>()->push_at(create_menu(), button_text->layout()->render_rect);
                return true;
            });
        } else {
            button->on(EVT_MOUSE_DOWN, [text](event_t& event) {
                printf("%s\n", text.c_str());
                return true;
            });
        }
    }

    return view;
}

view_ptr test2b(int argc, char** argv)
{
    view_ptr inner = std::make_shared<view_t>();
    layout_item_ptr root = inner->layout();

    view_ptr view = std::make_shared<view_t>();
    view->layout()->margin = 20;
    view->add_child(inner);

    // root->direction = LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE;
    root->direction = LAYOUT_FLEX_DIRECTION_ROW;

    // root->justify = LAYOUT_JUSTIFY_FLEX_END;
    root->justify = LAYOUT_JUSTIFY_CENTER;
    // root->justify = LAYOUT_JUSTIFY_SPACE_BETWEEN;
    // root->justify = LAYOUT_JUSTIFY_SPACE_AROUND;

    root->align = LAYOUT_ALIGN_FLEX_START;
    // root->align = LAYOUT_ALIGN_FLEX_END;
    // root->align = LAYOUT_ALIGN_CENTER;
    // root->align = LAYOUT_ALIGN_STRETCH;

    root->wrap = true;

    view_ptr item_a = std::make_shared<view_t>();
    view_ptr item_b = std::make_shared<view_t>();
    view_ptr item_c = std::make_shared<view_t>();
    view_ptr item_d = std::make_shared<view_t>();
    view_ptr item_e = std::make_shared<view_t>();
    view_ptr item_f = std::make_shared<view_t>();
    view_ptr item_g = std::make_shared<view_t>();
    view_ptr item_h = std::make_shared<view_t>();

    styled_frame_t style = {
        .bg = { 150, 150, 150 },
        .filled = true,
        .border = { 2 },
        .border_color = { 255, 0, 255, 255 },
        .margin = { 8 }
    };
    item_a->set_style(style);

    inner->add_child(item_a);
    style.border_radius = { 16 };
    item_b->set_style(style);

    inner->add_child(item_b);
    inner->add_child(item_c);
    inner->add_child(item_d);
    inner->add_child(item_e);
    inner->add_child(item_f);
    inner->add_child(item_g);
    inner->add_child(item_h);

    int i = 0;
    for (auto child : root->children) {
        child->name = 'a' + i++;
        child->width = 200;
        child->height = 200;
    }

    return view;
}

view_ptr test2(int argc, char** argv)
{
    render_layout = true;
    view_ptr inner = std::make_shared<view_t>();
    layout_item_ptr root = inner->layout();
    view_ptr view = std::make_shared<view_t>();
    view->layout()->margin = 20;
    view->add_child(inner);

    // root->direction = LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE;
    root->direction = LAYOUT_FLEX_DIRECTION_ROW;

    // root->justify = LAYOUT_JUSTIFY_FLEX_END;
    root->justify = LAYOUT_JUSTIFY_CENTER;
    // root->justify = LAYOUT_JUSTIFY_SPACE_BETWEEN;
    // root->justify = LAYOUT_JUSTIFY_SPACE_AROUND;

    root->align = LAYOUT_ALIGN_FLEX_START;
    // root->align = LAYOUT_ALIGN_FLEX_END;
    // root->align = LAYOUT_ALIGN_CENTER;
    // root->align = LAYOUT_ALIGN_STRETCH;

    root->wrap = true;

    layout_item_ptr item_a = std::make_shared<layout_item_t>();
    layout_item_ptr item_b = std::make_shared<layout_item_t>();
    layout_item_ptr item_c = std::make_shared<layout_item_t>();
    layout_item_ptr item_d = std::make_shared<layout_item_t>();
    layout_item_ptr item_e = std::make_shared<layout_item_t>();
    layout_item_ptr item_f = std::make_shared<layout_item_t>();
    layout_item_ptr item_g = std::make_shared<layout_item_t>();
    layout_item_ptr item_h = std::make_shared<layout_item_t>();

    root->children.push_back(item_a);
    root->children.push_back(item_b);
    root->children.push_back(item_c);
    root->children.push_back(item_d);
    root->children.push_back(item_e);
    root->children.push_back(item_f);
    root->children.push_back(item_g);
    root->children.push_back(item_h);

    int i = 0;
    for (auto child : root->children) {
        child->name = 'a' + i++;
        child->width = 200;
        child->height = 200;
    }

    return view;
}

view_ptr test1(int argc, char** argv)
{
    render_layout = true;
    view_ptr inner = std::make_shared<view_t>();
    layout_item_ptr root = inner->layout();
    view_ptr view = std::make_shared<view_t>();
    view->layout()->margin = 20;
    view->add_child(inner);

    root->direction = LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE;
    root->margin = 20;

    layout_item_ptr item_a = std::make_shared<layout_item_t>();
    layout_item_ptr item_b = std::make_shared<layout_item_t>();
    layout_item_ptr item_c = std::make_shared<layout_item_t>();
    layout_item_ptr item_d = std::make_shared<layout_item_t>();
    layout_item_ptr item_e = std::make_shared<layout_item_t>();
    layout_item_ptr item_f = std::make_shared<layout_item_t>();
    layout_item_ptr item_g = std::make_shared<layout_item_t>();
    layout_item_ptr item_h = std::make_shared<layout_item_t>();

    item_a->rgb = { 255, 0, 255 };
    item_b->rgb = { 255, 255, 0 };
    item_c->rgb = { 0, 255, 0 };
    item_d->rgb = { 0, 255, 255 };
    item_e->rgb = { 0, 0, 255 };
    item_f->rgb = { 155, 0, 155 };
    item_g->rgb = { 155, 155, 0 };
    item_h->rgb = { 0, 155, 0 };

    item_a->name = "a";
    item_b->name = "b";
    item_c->name = "c";
    item_d->name = "d";
    item_e->name = "e";
    item_f->name = "f";
    item_g->name = "g";
    item_h->name = "h";

    item_a->margin = 20;
    item_b->margin = 20;
    item_c->margin = 20;
    item_d->margin = 20;
    item_e->margin = 20;
    item_f->margin = 20;
    item_g->margin = 20;
    item_h->margin = 20;

    item_a->grow = 1;
    item_b->grow = 3;
    // item_a->visible = false;

    root->children.push_back(item_a);
    root->children.push_back(item_b);
    item_b->direction = LAYOUT_FLEX_DIRECTION_ROW;
    item_b->children.push_back(item_c);
    item_b->children.push_back(item_d);
    item_c->width = 400;
    // item_c->flex_basis = 400;

    item_d->direction = LAYOUT_FLEX_DIRECTION_COLUMN;
    // item_d->justify = LAYOUT_JUSTIFY_FLEX_END;
    // item_d->justify = LAYOUT_JUSTIFY_CENTER;
    // item_d->justify = LAYOUT_JUSTIFY_SPACE_BETWEEN;
    item_d->justify = LAYOUT_JUSTIFY_SPACE_AROUND;
    // item_d->align = LAYOUT_ALIGN_FLEX_END;
    item_d->align = LAYOUT_ALIGN_CENTER;
    // item_d->align = LAYOUT_ALIGN_STRETCH;

    item_d->children.push_back(item_e);
    item_d->children.push_back(item_f);

    item_e->align_self = LAYOUT_ALIGN_FLEX_END;
    item_e->height = 80;
    item_e->width = 200;
    item_f->height = 80;
    item_f->width = 200;

    item_c->direction = LAYOUT_FLEX_DIRECTION_ROW;
    // item_c->direction = LAYOUT_FLEX_DIRECTION_ROW_REVERSE;
    // item_c->justify = LAYOUT_JUSTIFY_FLEX_END;
    // item_c->justify = LAYOUT_JUSTIFY_CENTER;
    item_c->justify = LAYOUT_JUSTIFY_SPACE_BETWEEN;
    // item_c->justify = LAYOUT_JUSTIFY_SPACE_AROUND;
    item_c->align = LAYOUT_ALIGN_FLEX_END;
    // item_c->align = LAYOUT_ALIGN_CENTER;
    // item_c->align = LAYOUT_ALIGN_STRETCH;
    item_c->children.push_back(item_g);
    item_c->children.push_back(item_h);

    item_g->width = 100;
    item_g->height = 400;
    item_h->width = 80;
    item_h->height = 200;

    if (item_a->children.size() == 0)
        item_a->children.push_back(std::make_shared<layout_item_t>());
    if (item_b->children.size() == 0)
        item_b->children.push_back(std::make_shared<layout_item_t>());
    if (item_c->children.size() == 0)
        item_c->children.push_back(std::make_shared<layout_item_t>());
    if (item_d->children.size() == 0)
        item_d->children.push_back(std::make_shared<layout_item_t>());
    if (item_e->children.size() == 0)
        item_e->children.push_back(std::make_shared<layout_item_t>());
    if (item_f->children.size() == 0)
        item_f->children.push_back(std::make_shared<layout_item_t>());
    if (item_g->children.size() == 0)
        item_g->children.push_back(std::make_shared<layout_item_t>());
    if (item_h->children.size() == 0)
        item_h->children.push_back(std::make_shared<layout_item_t>());

    return view;
}
