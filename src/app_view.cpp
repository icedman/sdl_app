#include "app_view.h"
#include "renderer.h"

#include "app.h"
#include "operation.h"
#include "scripting.h"
#include "statusbar.h"

#include "editor_view.h"
#include "explorer_view.h"
#include "search_view.h"
#include "statusbar_view.h"
#include "tabbar_view.h"

#include "style.h"

extern color_info_t darker(color_info_t c, int x);
extern color_info_t lighter(color_info_t c, int x);

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

    popups = std::make_shared<popup_manager>();
    search = std::make_shared<search_view>();
    search->on(EVT_ITEM_SELECT, [this](event_t& e) {
        e.cancelled = true;
        list_item_view* item = (list_item_view*)e.target;
        return true;
    });
    add_child(popups);

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

    popup_manager* pm = view_item::cast<popup_manager>(popups);
    search_view* sv = view_item::cast<search_view>(search);
    list_view* list = view_item::cast<list_view>(sv->list);
    if (pm->_views.size()) {
        switch (op) {
        case CANCEL:
            close_popups();
            return true;
        }
        return pm->input_sequence(keySequence);
    }

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

    case POPUP_COMMANDS: {
        Scripting::instance()->execute("app.log('popup command palette')");
        return true;
    }

    case POPUP_SEARCH_FILES:
    case POPUP_SEARCH:
    case POPUP_SEARCH_LINE: {
        int fw, fh;
        Renderer::instance()->get_font_extents(Renderer::instance()->font((char*)style.font.c_str()), &fw, &fh, NULL, 1);
        int x = (layout()->render_rect.w / 2) - (30 * fw / 2);

        view_item::cast<search_view>(search)->show_search(op, op == POPUP_SEARCH_LINE ? ":" : "");

        popup_manager* pm = view_item::cast<popup_manager>(popups);
        pm->clear();
        pm->push_at(search, { x, 0, 0, 0 }, POPUP_DIRECTION_DOWN);
        return true;
    }

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

    case TOGGLE_SIDEBAR:
        app_t::instance()->showSidebar = !app_t::instance()->showSidebar;
        layout_request();
        return true;

    case CYCLE_TABS:
        // cycle tabs
        return true;

    default:
        break;
    }

    // printf("%s\n", keySequence.c_str());
    return false;
}

void app_view::update(int millis)
{
    app_t* app = app_t::instance();
    for (auto e : app->editors) {
        if (!e->view) {
            create_editor_view(e);
        }
    }

    view_item::update(millis);
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

void app_view::close_popups()
{
    app_t::log("close popups");
    popup_manager* pm = view_item::cast<popup_manager>(popups);
    if (pm->_views.size()) {
        pm->clear();
        show_editor(app_t::instance()->currentEditor);
    }
}

void app_view::setup_style()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;
    style_t comment = theme->styles_for_scope("comment");

    view_style_t vs_default;
    view_style_clear(vs_default);

    vs_default.font = "editor";
    vs_default.filled = false;
    vs_default.fg = Renderer::instance()->color_for_index(comment.foreground.index);
    vs_default.bg = Renderer::instance()->color_for_index(app->bgApp);
    vs_default.border_color = Renderer::instance()->color_for_index(app->bgApp);

    view_style_t vs = vs_default;
    view_style_t vs_item = vs_default;
    view_style_register(vs_default, "default");

    vs = vs_default;
    view_style_register(vs_default, "editor");

    vs = vs_default;
    view_style_register(vs_default, "scrollbar");

    vs = vs_default;
    view_style_register(vs, "gutter");

    vs = vs_default;
    vs.bg = darker(Renderer::instance()->color_for_index(app->bgApp), 5);
    vs.filled = Renderer::instance()->is_terminal() ? false : true;
    view_style_register(vs, "list");
    vs_item = vs;
    view_style_register(vs_item, "list.item");
    vs_item.bg = vs.fg;
    vs_item.fg = vs.bg;
    vs_item.filled = Renderer::instance()->is_terminal() ? false : true;
    view_style_register(vs_item, "list.item:selected");
    view_style_register(vs_item, "list.item:hovered");

    vs_default;
    vs.bg = darker(Renderer::instance()->color_for_index(app->bgApp), 5);
    vs.filled = Renderer::instance()->is_terminal() ? false : true;
    view_style_register(vs, "explorer");
    vs_item = vs;
    view_style_register(vs_item, "explorer.item");
    vs_item.bg = vs.fg;
    vs_item.fg = vs.bg;
    vs_item.filled = Renderer::instance()->is_terminal() ? false : true;
    view_style_register(vs_item, "explorer.item:selected");
    view_style_register(vs_item, "explorer.item:hovered");

    vs = vs_default;
    vs.bg = darker(Renderer::instance()->color_for_index(app->bgApp), 5);
    vs.filled = Renderer::instance()->is_terminal() ? false : true;
    view_style_register(vs, "tabbar");
    vs_item = vs;
    view_style_register(vs_item, "tabbar.item");
    vs_item.bg = vs.fg;
    vs_item.fg = vs.bg;
    vs_item.filled = Renderer::instance()->is_terminal() ? false : true;
    view_style_register(vs_item, "tabbar.item:selected");
    view_style_register(vs_item, "tabbar.item:hovered");

    vs = vs_default;
    vs.bg = darker(Renderer::instance()->color_for_index(app->bgApp), 5);
    vs.filled = true;
    view_style_register(vs, "completer");
    view_style_register(vs, "search");
    vs_item = vs;
    view_style_register(vs_item, "completer.item");
    view_style_register(vs_item, "search.item");
    vs_item.bg = vs.fg;
    vs_item.fg = vs.bg;
    vs_item.filled = Renderer::instance()->is_terminal() ? false : true;
    view_style_register(vs_item, "completer.item:selected");
    view_style_register(vs_item, "completer.item:hovered");
    view_style_register(vs_item, "search.item:selected");
    view_style_register(vs_item, "search.item:hovered");

    vs = vs_default;
    vs.bg = darker(Renderer::instance()->color_for_index(app->bgApp), 5);
    vs_item.filled = Renderer::instance()->is_terminal() ? false : true;
    view_style_register(vs, "statusbar");
}

void app_view::prelayout()
{
    tabbar->layout()->visible = app_t::instance()->showTabbar;
    statusbar->layout()->visible = app_t::instance()->showStatusBar;
    explorer->layout()->visible = app_t::instance()->showSidebar;
}