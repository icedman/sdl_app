#include "list.h"
#include "renderer.h"
#include "render_cache.h"

#include "app.h"
#include "scrollarea.h"
#include "scrollbar.h"
#include "image.h"
#include "text.h"
#include "app_view.h"

list_item_view::list_item_view()
    : horizontal_container()
{
    type = "list";
    interactive = true;

    on(EVT_MOUSE_CLICK, [this](event_t& evt) {
        evt.cancelled = true;
        return this->mouse_click(evt.x, evt.y, evt.button);
    });
}

bool list_item_view::mouse_click(int x, int y, int button) {  
    container->select_item(this);      
    return true;
}

void list_item_view::render() {
    layout_item_ptr lo = layout();
    layout_rect r = lo->render_rect;

    if (data.selected) {
        draw_rect({ r.x, r.y, r.w, r.h }, { 255,0, 255, 150 }, false, 4);
    } else {
        draw_rect({ r.x, r.y, r.w, r.h }, { 255,0, 255, 150 }, false, 1);
    }
}

list_view::list_view(std::vector<list_item_data_t> items)
    : list_view()
{
    data = items;
}

list_view::list_view()
    : panel_view()
    , selected(0)
{
    interactive = true;

    layout()->width = 300;
    content()->layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;
    content()->layout()->wrap = false;
    content()->layout()->fit_children = true;
}

void list_view::prelayout()
{
    int fw, fh;
    ren_get_font_extents(ren_font("ui-small"), &fw, &fh, "A", 1);
    ((scrollarea_view*)(scrollarea.get()))->move_factor_x = fw;
    ((scrollarea_view*)(scrollarea.get()))->move_factor_y = fh;
}

void list_view::update()
{   
    scrollarea_view *area = view_item::cast<scrollarea_view>(scrollarea);
    layout_item_ptr alo = area->layout();
    for(auto v : content()->_views) {
        layout_item_ptr lo = v->layout();
        lo->offscreen = false;
        int y = lo->rect.y + alo->scroll_y;
        if (y + lo->rect.h < 0 || y > alo->rect.h) {
            lo->offscreen = true;
            // printf(">\n");
        }
        // printf(">%d %d\n", lo->rect.y, alo->scroll_y);
    }

    bool hasChanges = false;
    if (data.size() == content()->_views.size()) {
        view_item_list::iterator it = content()->_views.begin();
        for(auto f : data) {
            view_item_ptr iv = *it++;
            list_item_view *item = view_item::cast<list_item_view>(iv);
            if (item->data.equals(f)) {
                continue;
            }
            hasChanges = true;
            break;
        }
    } else {
        hasChanges = true;
    }

    if (!hasChanges) return;

    while(content()->_views.size() < data.size()) {
        view_item_ptr item = std::make_shared<list_item_view>();
        item->layout()->align = LAYOUT_ALIGN_CENTER;
        item->layout()->height = 32;
        view_item_ptr icon = std::make_shared<icon_view>();
        icon->layout()->width = 32;
        icon->layout()->height = 24;
        view_item_ptr text = std::make_shared<text_view>("...");

        view_item_ptr depth = std::make_shared<view_item>();
        depth->layout()->width = 1;
        item->add_child(depth);
        item->add_child(icon);
        item->add_child(text);
        content()->add_child(item);

        view_item::cast<list_item_view>(item)->container = this;
    }

    for(auto _v : content()->_views) {
        _v->layout()->visible = false;
    }
    
    bool hasIcons = false;
    view_item_list::iterator it = content()->_views.begin();
    for(auto d : data) {
        view_item_ptr item = *it++;
        item->layout()->visible = true;

        ((list_item_view*)item.get())->data = d;
        std::string icon_path =icon_for_file(app_t::instance()->icons, d.icon, app_t::instance()->extensions);

        view_item_ptr spacer = item->_views[0];
        spacer->layout()->width = 1 + (d.indent * 24);

        view_item_ptr icon = item->_views[1];
        ((icon_view*)icon.get())->icon = ren_create_image_from_svg((char*)d.icon.c_str(), 24,24);
        if (d.icon != "") {
            hasIcons = true;
        }
        icon->layout()->visible = false; // hide first
        view_item_ptr text = item->_views[2];
        ((text_view*)text.get())->text = d.text;

        text->prelayout();
        text->layout()->rect.w = text->layout()->width;
    }

    if (hasIcons) {
        it = content()->_views.begin();
        for(auto d : data) {
            view_item_ptr item = *it++;
            view_item_ptr icon = item->_views[1];
            icon->layout()->visible = true;
        }
    }

    panel_view::update();
}

void list_view::select_item(list_item_view *item)
{
    if (selected && selected != item) {
        selected->data.selected = false;
    }
    item->data.selected = !item->data.selected;
    if (item->data.selected) {
        selected = item;
    }
}