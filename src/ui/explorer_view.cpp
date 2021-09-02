#include "explorer_view.h"
#include "renderer.h"
#include "render_cache.h"

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

explorer_view::explorer_view()
    : view_item("explorer")
{
    explorer_t *explorer = explorer_t::instance();
    explorer->update(0);

    layout()->width = 300;
    layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;

    // for(int i=0;i<20; i++) {
    for(auto f : explorer->renderList) {
        view_item_ptr btn = std::make_shared<horizontal_container>();
        btn->layout()->height = 32;

        std::string icon_path =icon_for_file(app_t::instance()->icons, f->name, app_t::instance()->extensions);

        view_item_ptr icon = std::make_shared<icon_view>();
        ((icon_view*)icon.get())->icon = ren_create_image_from_svg((char*)icon_path.c_str(), 24,24);

        icon->layout()->width = 32;
        view_item_ptr text = std::make_shared<text_view>(f->name);
        btn->add_child(icon);
        btn->add_child(text);

        add_child(btn);
    }
}
