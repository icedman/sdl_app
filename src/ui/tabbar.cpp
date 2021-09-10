#include "tabbar.h"
#include "renderer.h"
#include "render_cache.h"

#include "app.h"
#include "app_view.h"
#include "button.h"
#include "image.h"

#include "app.h"
#include "app_view.h"
#include "style.h"

struct tab_item_view : horizontal_container {

    tab_item_view()
        : horizontal_container()
    {
        interactive = true;

        on(EVT_MOUSE_CLICK, [this](event_t& evt) {
            evt.cancelled = true;
            return this->mouse_click(evt.x, evt.y, evt.button);
        });
    }

    editor_ptr editor;

    bool mouse_click(int x, int y, int button) override
    {
        // view_set_focused((view_item*)editor->view);
        app_t* app = app_t::instance();
        ((app_view*)(app->view))->show_editor(editor, true);
        return true;
    }

    void render() override
    {
        app_t* app = app_t::instance();
        view_style_t vs = view_style_get("explorer");

        layout_item_ptr lo = layout();
        layout_rect r = lo->render_rect;
        // printf("%s %d %d %d %d\n", file->name.c_str(), r.x, r.y, r.w, r.h);

        bool focused = view_get_focused() == editor->view;

        // RenColor clr = {
        //     255,
        //     0,
        //     255,
        //     focused ? 250 : 50
        // };

        RenColor clr = { (uint8_t)vs.fg.red, (uint8_t)vs.fg.green, (uint8_t)vs.fg.blue, focused ? 20 : 5 };

        draw_rect({ lo->render_rect.x,
                      lo->render_rect.y,
                      lo->render_rect.w,
                      lo->render_rect.h },
            clr, true);
    }
};

tabbar_view::tabbar_view()
    : horizontal_container()
{
    type = "tabbar";
    interactive = true;

    layout()->margin_top = 8;
    layout()->height = 40;
    layout()->justify = LAYOUT_JUSTIFY_FLEX_START;
    layout()->align = LAYOUT_ALIGN_FLEX_END;

    // for(int i=0;i<6;i++) {
    //     std::string name = "tab ";
    //     name += 'a' + i;

    //     view_item_ptr tab = std::make_shared<button_view>(name);
    //     tab->layout()->height = 32;
    //     add_child(tab);
    // }

    scrollarea = std::make_shared<scrollarea_view>();
    content()->layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
    content()->layout()->fit_children = true;
    content()->layout()->wrap = false;

    ((scrollarea_view*)(scrollarea.get()))->move_factor_y = 0;
    add_child(scrollarea);

    // spacer = std::make_shared<view_item>("spacer");
    // add_child(spacer);
}

void tabbar_view::prelayout()
{
}

void tabbar_view::update()
{
    app_t* app = app_t::instance();

    bool hasChanges = false;
    if (app->editors.size() == content()->_views.size()) {
        view_item_list::iterator it = content()->_views.begin();
        for (auto e : app->editors) {
            view_item_ptr tab = *it++;
            if (((tab_item_view*)tab.get())->editor == e) {
                continue;
            }
            hasChanges = true;
            break;
        }
    } else {
        hasChanges = true;
    }

    if (!hasChanges)
        return;

    while (content()->_views.size() < app->editors.size()) {
        view_item_ptr btn = std::make_shared<tab_item_view>();
        btn->layout()->preferred_constraint = {
            0,0,
            200,0
        };

        btn->layout()->justify = LAYOUT_JUSTIFY_CENTER;
        btn->layout()->align = LAYOUT_ALIGN_CENTER;
        btn->layout()->height = 32;

        // view_item_ptr icon = std::make_shared<icon_view>();
        // icon->layout()->width = 32;
        // icon->layout()->height = 24;
        // btn->add_child(icon);

        view_item_ptr text = std::make_shared<text_view>("...");
        btn->add_child(text);

        content()->add_child(btn);
    }

    for (auto _v : content()->_views) {
        _v->layout()->visible = false;
    }

    view_item_list::iterator it = content()->_views.begin();
    for (auto e : app->editors) {
        view_item_ptr btn = *it++;
        btn->layout()->visible = true;

        ((tab_item_view*)btn.get())->editor = e;
        // std::string icon_path =icon_for_file(app_t::instance()->icons, f->name, app_t::instance()->extensions);

        // view_item_ptr spacer = btn->_views[0];
        // spacer->layout()->width = 1 + (f->depth * 24);

        // printf("%s %d\n", f->name.c_str(), f->depth);

        // view_item_ptr icon = btn->_views[0];
        // if (f->isDirectory) {
        //     if (f->expanded) {
        //         ((icon_view*)icon.get())->icon = ren_create_image_from_svg((char*)folder_close_icon_path.c_str(), 24,24);
        //     } else {
        //         ((icon_view*)icon.get())->icon = ren_create_image_from_svg((char*)folder_icon_path.c_str(), 24,24);
        //     }
        // } else {
        //     ((icon_view*)icon.get())->icon = ren_create_image_from_svg((char*)icon_path.c_str(), 24,24);
        // }

        view_item_ptr text = btn->_views[0];
        ((text_view*)text.get())->text = e->document.fileName.length() ? e->document.fileName : "untitled";

        text->prelayout();
        text->layout()->rect.w = text->layout()->width;
    }

    view_item::update();
}

view_item_ptr tabbar_view::content()
{
    return ((scrollarea_view*)scrollarea.get())->content;
}

void tabbar_view::render() {
    app_t* app = app_t::instance();
    view_style_t vs = view_style_get("explorer");

    layout_item_ptr lo = layout();

    draw_rect({
        lo->render_rect.x,
        lo->render_rect.y,
        lo->render_rect.w,
        lo->render_rect.h
    } , { (uint8_t)vs.bg.red, (uint8_t)vs.bg.green, (uint8_t)vs.bg.blue }, true);
}