#include "tabbar.h"
#include "button.h"
#include "hash.h"
#include "scrollbar.h"
#include "system.h"
#include "text.h"

struct tabbar_item_t : list_item_t {
    void render(renderer_t* renderer) override
    {
        layout_item_ptr lo = layout();
        if (is_hovered(this)) {
            renderer->draw_rect(lo->render_rect, { 255, 0, 255 }, false, true);
        }
    }
};

tabbar_t::tabbar_t()
    : panel_t()
{
    content()->layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;

    layout()->fit_children_x = false;
    layout()->fit_children_y = true;

    v_scroll->cast<scrollbar_t>()->disabled = true;
    remove_child(bottom);

    layout()->prelayout = [this](layout_item_t* item) {
        this->prelayout();
        return true;
    };

    layout()->name = "tabbar";
    layout()->margin_top = 8;
}

void tabbar_t::prelayout()
{
    layout()->height = 1.5f * font()->height + layout()->margin_top;
}

view_ptr tabbar_t::create_item()
{
    view_ptr item = std::make_shared<tabbar_item_t>();
    item->layout()->fit_children_x = true;
    item->layout()->fit_children_y = true;
    item->layout()->margin_left = 4;
    item->layout()->margin_right = 4;
    view_ptr btn = std::make_shared<button_t>();
    view_ptr text = std::make_shared<text_t>("ITEM TEMPLATE");
    item->add_child(text);
    item->add_child(btn);
    btn->layout()->width = 32;
    btn->layout()->fit_children_y = false;
    btn->layout()->margin = 2;
    item->layout()->visible = false;
    item->layout()->preferred_constraint.max_width = font()->width * 20;

    item->layout()->margin_left = 6;
    item->layout()->margin_right = 6;

    item->layout()->fit_children_x = true;
    item->layout()->fit_children_y = true;
    item->layout()->justify = LAYOUT_JUSTIFY_SPACE_BETWEEN;
    item->layout()->align = LAYOUT_ALIGN_CENTER;

    view_t* _item = item.get();
    item->on(EVT_MOUSE_CLICK, [this, _item](event_t& evt) {
        evt.cancelled = true;
        evt.source = _item;
        evt.button = 0;
        this->handle_item_click(evt);
        return true;
    });

    btn->on(EVT_MOUSE_CLICK, [this, _item](event_t& evt) {
        evt.cancelled = true;
        evt.source = _item;
        evt.button = 1;
        this->handle_item_click(evt);
        return true;
    });

    return item;
}

void tabbar_t::update_item(view_ptr item, list_item_data_t data)
{
    item->cast<list_item_t>()->item_data = data;
    item->cast<list_item_t>()->find_child(view_type_e::TEXT)->cast<text_t>()->set_text(data.text);
    item->layout()->visible = true;
}

void tabbar_t::update_data(std::vector<list_item_data_t> _data)
{
    data = _data;

    while (content()->children.size() < data.size()) {
        view_ptr vi = create_item();
        list_item_t* i = vi->cast<list_item_t>();
        content()->add_child(vi);
        vi->layout()->height = font()->height;
    }

    for (auto c : content()->children) {
        c->layout()->visible = false;
    }

    std::vector<list_item_data_t>::iterator it = data.begin();
    for (auto c : content()->children) {
        if (it == data.end())
            break;
        list_item_data_t d = *it++;
        update_item(c, d);
    }
}

bool tabbar_t::handle_item_click(event_t& evt)
{
    if (evt.source) {
        select_item(((view_t*)(evt.source))->ptr(), evt.button);
    }
    return true;
}

void tabbar_t::select_item(view_ptr item, int button)
{
    if (!item)
        return;

    list_item_t* n = item->cast<list_item_t>();
    event_t evt;
    evt.type = EVT_ITEM_SELECT;
    evt.source = n;
    evt.button = button;
    evt.cancelled = false;
    propagate_event(evt);
}

list_item_data_t tabbar_t::value()
{
    return selected_data;
}

void tabbar_t::render(renderer_t* renderer)
{
    layout_item_ptr lo = layout();
    color_t clr = color_darker(system_t::instance()->renderer.background, 10);
    renderer->draw_rect(lo->render_rect, clr, true, 0);
}

int tabbar_t::content_hash(bool peek)
{
    int hash = 0;
    if (data.size()) {
        hash = murmur_hash(&data[0], sizeof(list_item_data_t) * data.size(), CONTENT_HASH_SEED);
    }
    if (!peek) {
        _content_hash = hash;
    }
    return hash;
}

tabbed_content_t::tabbed_content_t()
    : vertical_container_t()
{
    _tabbar = std::make_shared<tabbar_t>();
    _content = std::make_shared<vertical_container_t>();

    add_child(_tabbar);
    add_child(_content);
}
