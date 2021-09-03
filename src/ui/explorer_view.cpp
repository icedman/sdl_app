#include "explorer_view.h"
#include "renderer.h"
#include "render_cache.h"

#include "text.h"

#include "app.h"
#include "extension.h"
#include "explorer.h"

struct icon_view : view_item {
    icon_view()
        : view_item("icon")
        , icon(0)
    {
        layout()->width = 32;
        layout()->height = 24;
    }

    void render() override {
        if (icon) {
            draw_image(icon, { 4 + layout()->render_rect.x, layout()->render_rect.y, 24, 24 });
        }
    }

    RenImage *icon;
};

struct explorer_item_view : horizontal_container {

    explorer_item_view() : horizontal_container() {
        interactive = true;
    }

    fileitem_t *file;

    bool mouse_click(int x, int y, int button) override {
        if (file && file->isDirectory) {
            if (file->canLoadMore) {
                file->load();
                file->canLoadMore = false;
            }
            file->expanded = !file->expanded;
            explorer_t::instance()->regenerateList = true;
            layout_request();
        }

        explorer_t::instance()->print();
        return true;
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
}

void explorer_view::prelayout()
{
    explorer_t *explorer = explorer_t::instance();
    explorer->update(0);

    // printf("%d %d\n", explorer->renderList.size(), _views.size());

    while(content()->_views.size() < explorer->renderList.size()) {
        view_item_ptr btn = std::make_shared<explorer_item_view>();
        btn->layout()->height = 32;
        view_item_ptr icon = std::make_shared<icon_view>();
        icon->layout()->width = 32;
        view_item_ptr text = std::make_shared<text_view>("...");
        btn->add_child(icon);
        btn->add_child(text);
        content()->add_child(btn);
    }

    for(auto _v : content()->_views) {
        _v->layout()->visible = false;
    }
    // content()->layout()->rect.h = 32 * explorer->renderList.size();
    
    view_item_list::iterator it = content()->_views.begin();
    for(auto f : explorer->renderList) {
        view_item_ptr btn = *it++;
        btn->layout()->visible = true;

        ((explorer_item_view*)btn.get())->file = f;
        std::string icon_path =icon_for_file(app_t::instance()->icons, f->name, app_t::instance()->extensions);

        view_item_ptr icon = btn->_views[0];
        ((icon_view*)icon.get())->icon = ren_create_image_from_svg((char*)icon_path.c_str(), 24,24);

        view_item_ptr text = btn->_views[1];
        ((text_view*)text.get())->text = f->name;
    }
}

void explorer_view::update()
{   
    explorer_t *explorer = explorer_t::instance();
    explorer->update(0);

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
}