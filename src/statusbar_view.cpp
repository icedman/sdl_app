#include "statusbar_view.h"
#include "renderer.h"
#include "render_cache.h"

#include "app.h"
#include "statusbar.h"
#include "style.h"

statusbar_view::statusbar_view()
    : horizontal_container()
{
    type = "statusbar";

    layout()->height = 24;
    layout()->fit_children = false;
    layout()->justify = LAYOUT_JUSTIFY_SPACE_AROUND;

    view_item_ptr container = std::make_shared<view_item>();
    container->layout()->height = 24;
    container->layout()->grow = 3;

    status = std::make_shared<text_view>("status");
    status->layout()->height = 24;
    items = std::make_shared<horizontal_container>();
    items->layout()->justify = LAYOUT_JUSTIFY_FLEX_END;
    items->layout()->grow = 2;

    container->add_child(status);
    add_child(container);
    add_child(items);

    for (int i = 0; i < 5; i++) {
        std::string t = "#";
        t += '1' + i;
        view_item_ptr item = std::make_shared<text_view>(t);
        items->add_child(item);
    }
}

void statusbar_view::update()
{
    statusbar_t* statusbar = statusbar_t::instance();
    if (statusbar) {
        text_view* text = view_item::cast<text_view>(status);
        text->text = statusbar->status;
        text->prelayout();
        text->layout()->rect.w = text->layout()->width;
        int i = 0;
        for (auto v : items->_views) {
            text_view* text = view_item::cast<text_view>(v);
            // text->text = statusbar->text[i++];
            text->prelayout();
            text->layout()->rect.w = text->layout()->width;
        }
    }

    view_item::update();
}

void statusbar_view::render() {
    app_t* app = app_t::instance();
    view_style_t vs = view_style_get("editor");

    layout_item_ptr lo = layout();

    draw_rect({
        lo->render_rect.x,
        lo->render_rect.y,
        lo->render_rect.w,
        lo->render_rect.h
    } , { (uint8_t)vs.bg.red, (uint8_t)vs.bg.green, (uint8_t)vs.bg.blue }, true);

}