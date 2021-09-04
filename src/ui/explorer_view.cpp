#include "explorer_view.h"
#include "renderer.h"
#include "render_cache.h"

#include "scrollarea.h"
#include "scrollbar.h"
#include "image.h"
#include "text.h"
#include "app_view.h"

#include "app.h"
#include "extension.h"
#include "explorer.h"

struct explorer_item_view : horizontal_container {

    explorer_item_view() : horizontal_container() {
        interactive = true;
    }

    fileitem_t *file;

    bool mouse_click(int x, int y, int button) override {
        app_t *app = app_t::instance();
        if (file && file->isDirectory) {
            if (file->canLoadMore) {
                file->load();
                file->canLoadMore = false;
            }
            file->expanded = !file->expanded;
            explorer_t::instance()->regenerateList = true;
            layout_request();
        } else {
            bool multi = (view_input_key_mods() & K_MOD_CTRL) == K_MOD_CTRL;
            ((app_view*)(app->view))->show_editor(app->openEditor(file->fullPath), !multi);
        }
        return true;
    }

    void render() override {
        layout_item_ptr lo = layout();
        layout_rect r = lo->render_rect;
        // printf("%s %d %d %d %d\n", file->name.c_str(), r.x, r.y, r.w, r.h);
    }
};

explorer_view::explorer_view()
    : panel_view()
{
    interactive = true;

    layout()->width = 300;
    content()->layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;
    content()->layout()->wrap = false;
    content()->layout()->fit_children = true;

    ((scrollarea_view*)(scrollarea.get()))->move_factor_x = 2;
    ((scrollarea_view*)(scrollarea.get()))->move_factor_y = 2;
}

void explorer_view::prelayout()
{}

void explorer_view::postlayout()
{
    // printf("%d %d\n", content()->layout()->rect.w, content()->layout()->rect.h);
    ((scrollbar_view*)v_scroll.get())->set_size(
        content()->layout()->rect.h, 
        scrollarea->layout()->rect.h
    );

    ((scrollbar_view*)h_scroll.get())->set_size(
        content()->layout()->rect.w, 
        scrollarea->layout()->rect.w
    );
}

void explorer_view::update()
{   
    explorer_t *explorer = explorer_t::instance();
    explorer->update(0);

    // printf("%d %d\n", explorer->renderList.size(), _views.size());

    bool hasChanges = false;
    if (explorer->renderList.size() == content()->_views.size()) {
        view_item_list::iterator it = content()->_views.begin();
        for(auto f : explorer->renderList) {
            view_item_ptr btn = *it++;
            if (((explorer_item_view*)btn.get())->file == f) {
                continue;
            }
            hasChanges = true;
            break;
        }
    } else {
        hasChanges = true;
    }

    if (!hasChanges) return;

    while(content()->_views.size() < explorer->renderList.size()) {
        view_item_ptr btn = std::make_shared<explorer_item_view>();
        btn->layout()->align = LAYOUT_ALIGN_CENTER;
        btn->layout()->height = 32;
        view_item_ptr icon = std::make_shared<icon_view>();
        icon->layout()->width = 32;
        icon->layout()->height = 24;
        view_item_ptr text = std::make_shared<text_view>("...");

        view_item_ptr depth = std::make_shared<view_item>();
        depth->layout()->width = 1;
        btn->add_child(depth);
        btn->add_child(icon);
        btn->add_child(text);
        content()->add_child(btn);
    }

    for(auto _v : content()->_views) {
        _v->layout()->visible = false;
    }
    // content()->layout()->rect.h = 32 * explorer->renderList.size();
    
    std::string folder_icon_path =icon_for_file(app_t::instance()->icons, ".folder", app_t::instance()->extensions);
    std::string folder_close_icon_path =icon_for_file(app_t::instance()->icons, ".folder-open", app_t::instance()->extensions);

    view_item_list::iterator it = content()->_views.begin();
    for(auto f : explorer->renderList) {
        view_item_ptr btn = *it++;
        btn->layout()->visible = true;

        ((explorer_item_view*)btn.get())->file = f;
        std::string icon_path =icon_for_file(app_t::instance()->icons, f->name, app_t::instance()->extensions);

        view_item_ptr spacer = btn->_views[0];
        spacer->layout()->width = 1 + (f->depth * 24);

        // printf("%s %d\n", f->name.c_str(), f->depth);

        view_item_ptr icon = btn->_views[1];
        if (f->isDirectory) {
            if (f->expanded) {
                ((icon_view*)icon.get())->icon = ren_create_image_from_svg((char*)folder_close_icon_path.c_str(), 24,24);
            } else {
                ((icon_view*)icon.get())->icon = ren_create_image_from_svg((char*)folder_icon_path.c_str(), 24,24);
            }
        } else {
            ((icon_view*)icon.get())->icon = ren_create_image_from_svg((char*)icon_path.c_str(), 24,24);
        }

        view_item_ptr text = btn->_views[2];
        ((text_view*)text.get())->text = f->name;

        text->prelayout();
        text->layout()->rect.w = text->layout()->width;
    }
}

void explorer_view::_validate()
{
    scrollarea_view *scroll = (scrollarea_view*)(scrollarea.get());
    
    if (scroll->layout()->scroll_x > 0) {
        scroll->layout()->scroll_x = 0;
        layout_request();
    }
    if (scroll->layout()->scroll_y > 0) {
        scroll->layout()->scroll_y = 0;
    }
}

bool explorer_view::on_scroll()
{
    scrollarea_view *scroll = (scrollarea_view*)(scrollarea.get());
    scroll->layout()->scroll_x = -((scrollbar_view*)h_scroll.get())->index;
    scroll->layout()->scroll_y = -((scrollbar_view*)v_scroll.get())->index;
    _validate();
    return true;
}

bool explorer_view::mouse_wheel(int x, int y)
{
    scrollarea_view *scroll = (scrollarea_view*)(scrollarea.get());
    
    scroll->layout()->scroll_x += x * scroll->move_factor_x;
    scroll->layout()->scroll_y += y * scroll->move_factor_y;

    // printf("%d %d\n",
    //     scroll->layout()->scroll_x,
    //     scroll->layout()->scroll_y);

    _validate();

    ((scrollbar_view*)h_scroll.get())->set_index(-scroll->layout()->scroll_x);
    ((scrollbar_view*)v_scroll.get())->set_index(-scroll->layout()->scroll_y);

    return false;
}