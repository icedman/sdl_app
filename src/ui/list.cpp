#include "list.h"
#include "renderer.h"

#include "app.h"
#include "app_view.h"
#include "image.h"
#include "scrollarea.h"
#include "scrollbar.h"
#include "style.h"
#include "text.h"

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

bool list_item_view::mouse_click(int x, int y, int button)
{
    container->select_item(this);
    return true;
}

void list_item_view::render()
{
    app_t* app = app_t::instance();
    view_style_t vs = view_style_get("explorer");

    layout_item_ptr lo = layout();
    layout_rect r = lo->render_rect;

    RenColor clr = { (uint8_t)vs.fg.red, (uint8_t)vs.fg.green, (uint8_t)vs.fg.blue,
        (uint8_t)(Renderer::instance()->is_terminal() ? vs.fg.index : 20) };

    if (container->is_selected(this) || is_hovered()) {
        Renderer::instance()->draw_rect({ r.x, r.y, r.w, r.h }, clr, true);
    } else {
        if (container->focused_value == data.value) {
            Renderer::instance()->draw_rect({ r.x, r.y, r.w, r.h }, clr, true);
        }
    }
}

list_view::list_view(std::vector<list_item_data_t> items)
    : list_view()
{
    data = items;
    layout()->margin = 8;
    if (Renderer::instance()->is_terminal()) {
        layout()->margin = 0;
    }
}

list_view::list_view()
    : panel_view()
    , autoscroll(false)
{
    interactive = true;

    layout()->width = 300;
    if (Renderer::instance()->is_terminal()) {
        layout()->width = 20;
    }
    content()->layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;
}

void list_view::prelayout()
{
    int fw, fh;
    Renderer::instance()->get_font_extents(Renderer::instance()->font("ui-small"), &fw, &fh, "A", 1);
    ((scrollarea_view*)(scrollarea.get()))->move_factor_x = fw;
    ((scrollarea_view*)(scrollarea.get()))->move_factor_y = fh;
}

void list_view::update()
{
    if (autoscroll && prev_value != value) {
        if (!ensure_visible_cursor()) {
            prev_value = value;
        } else {
            Renderer::instance()->listen_quick(0);
        }
    }

    scrollarea_view* area = view_item::cast<scrollarea_view>(scrollarea);
    layout_item_ptr alo = area->layout();
    for (auto v : content()->_views) {
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
        for (auto f : data) {
            view_item_ptr iv = *it++;
            list_item_view* item = view_item::cast<list_item_view>(iv);
            if (item->data.equals(f)) {
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

    while (content()->_views.size() < data.size()) {
        view_item_ptr item = create_item();
        content()->add_child(item);

        view_item::cast<list_item_view>(item)->container = this;
    }

    for (auto _v : content()->_views) {
        _v->layout()->visible = false;
    }

    bool hasIcons = false;
    view_item_list::iterator it = content()->_views.begin();
    for (auto d : data) {
        view_item_ptr item = *it++;
        list_item_view* iv = view_item::cast<list_item_view>(item);
        item->layout()->visible = true;

        ((list_item_view*)item.get())->data = d;

        if (iv->depth) {
            iv->depth->layout()->width = 1 + (d.indent * 24);
            if (Renderer::instance()->is_terminal()) {
                iv->depth->layout()->width = 1 + d.indent;
            }
        }

        if (iv->icon) {
            std::string icon_path = icon_for_file(app_t::instance()->icons, d.icon, app_t::instance()->extensions);
            ((icon_view*)(iv->icon.get()))->icon = Renderer::instance()->create_image_from_svg((char*)d.icon.c_str(), 24, 24);
            if (d.icon != "") {
                hasIcons = true;
            }
            iv->icon->layout()->visible = false; // hide first
        }

        if (iv->text) {
            view_item::cast<text_view>(iv->text)->text = d.text;
            iv->text->prelayout();
            iv->text->layout()->rect.w = iv->text->layout()->width;
        }
    }

    if (hasIcons) {
        it = content()->_views.begin();
        for (auto d : data) {
            view_item_ptr item = *it++;
            view_item_ptr icon = item->_views[1];
            icon->layout()->visible = true;
        }
    }

    panel_view::update();
}

void list_view::select_item(list_item_view* item)
{
    if (item->data.value != value) {
        value = item->data.value;

        // propagate this event
        event_t event;
        event.type = EVT_ITEM_SELECT;
        event.source = this;
        event.target = item;
        event.cancelled = false;
        propagate_event(event);
    }
}

bool list_view::is_selected(list_item_view* item)
{
    // printf(">%s -- %s\n", value.c_str(), item->data.value.c_str());
    return value == item->data.value && value.length();
}

void list_view::render()
{
    app_t* app = app_t::instance();
    view_style_t vs = view_style_get("explorer");

    layout_item_ptr lo = layout();

    if (Renderer::instance()->is_terminal()) {
    } else {
        Renderer::instance()->draw_rect({ lo->render_rect.x,
                                            lo->render_rect.y,
                                            lo->render_rect.w,
                                            lo->render_rect.h },
            { (uint8_t)vs.bg.red, (uint8_t)vs.bg.green, (uint8_t)vs.bg.blue }, true);
    }

    // layout_rect r = lo->render_rect;
    // Renderer::instance()->draw_rect({ r.x, r.y, r.w - 20, r.h - 4 }, { 255,0,255,150 }, false, 1);
}

int focused_index(std::vector<list_item_data_t>& data, std::string value)
{
    if (value == "")
        return -1;

    int idx = 0;
    for (auto d : data) {
        if (d.value == value) {
            return idx;
        }
        idx++;
    }

    return -1;
}

void list_view::focus_previous()
{
    if (!data.size())
        return;

    if (focused_value == "") {
        focused_value = data[0].value;
        return;
    }

    int idx = focused_index(data, focused_value);
    if (idx > 0) {
        focused_value = data[idx - 1].value;
    }
}

void list_view::focus_next()
{
    if (!data.size())
        return;

    if (focused_value == "") {
        focused_value = data[0].value;
        return;
    }

    int idx = focused_index(data, focused_value);
    if (idx >= 0 && ++idx < data.size()) {
        focused_value = data[idx].value;
    }
}

void list_view::select_focused()
{
    if (!data.size())
        return;
    int idx = focused_index(data, focused_value);
    if (idx == -1)
        return;

    for (auto v : content()->_views) {
        list_item_view* item = view_item::cast<list_item_view>(v);
        if (item->data.value == focused_value) {
            select_item(item);
            return;
        }
    }
}

view_item_ptr list_view::create_item()
{
    view_item_ptr item = std::make_shared<list_item_view>();
    list_item_view* iv = view_item::cast<list_item_view>(item);
    item->layout()->align = LAYOUT_ALIGN_CENTER;
    item->layout()->height = 24;

    iv->icon = std::make_shared<icon_view>();
    iv->icon->layout()->width = 32;
    iv->icon->layout()->height = 24;

    iv->text = std::make_shared<text_view>("...");

    iv->depth = std::make_shared<view_item>();
    iv->depth->layout()->width = 1;

    item->add_child(iv->depth);
    item->add_child(iv->icon);
    item->add_child(iv->text);

    if (Renderer::instance()->is_terminal()) {
        item->layout()->height = 1;
        iv->icon->layout()->width = 1;
        iv->icon->layout()->height = 1;
    }

    return item;
}

bool list_view::ensure_visible_cursor()
{
    int idx = focused_index(data, focused_value);
    if (idx == -1) {
        idx = focused_index(data, value);
    }
    if (idx == -1) {
        return true;
    }
    if (idx >= content()->_views.size()) {
        return true;
    }

    view_item_ptr item = content()->_views[idx];

    // list_item_view *iv = view_item::cast<list_item_view>(item);
    // list_view *lv = view_item::cast<list_view>(this);
    // scrollbar_view* hs = view_item::cast<scrollbar_view>(h_scroll);
    bool scrolled = false;

    scrollarea_view* area = view_item::cast<scrollarea_view>(scrollarea);
    layout_item_ptr alo = area->layout();
    layout_item_ptr lo = item->layout();

    if (lo->render_rect.x + lo->render_rect.w > alo->render_rect.x + alo->render_rect.w) {
        mouse_wheel(-1, 0);
        scrolled = true;
    }
    if (lo->render_rect.x < alo->render_rect.x) {
        mouse_wheel(1, 0);
        scrolled = true;
    }

    if (lo->render_rect.y + lo->render_rect.h > alo->render_rect.y + alo->render_rect.h) {
        mouse_wheel(0, -1);
        scrolled = true;
    }
    if (lo->render_rect.y < alo->render_rect.y) {
        mouse_wheel(0, 1);
        scrolled = true;
    }

    return scrolled;
}