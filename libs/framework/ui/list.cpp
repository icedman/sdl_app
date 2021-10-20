#include "list.h"
#include "hash.h"
#include "scrollarea.h"
#include "system.h"
#include "text.h"

#define VISIBLE_ITEMS_PAD 4
#define TAIL_PAD 16

list_item_t::list_item_t()
    : horizontal_container_t()
{
    can_hover = true;
    layout()->fit_children_x = false;
    layout()->fit_children_y = true;
    layout()->margin = 2;
}

void list_item_t::render(renderer_t* renderer)
{
    render_frame(renderer);
}

list_t::list_t()
    : panel_t()
    , visible_items(0)
{
    layout()->prelayout = [this](layout_item_t* item) {
        this->prelayout();
        return true;
    };

    layout()->name = "list";
}

view_ptr list_t::create_item()
{
    view_ptr item = std::make_shared<list_item_t>();
    item->layout()->fit_children_x = true;
    item->layout()->fit_children_y = true;
    view_ptr indent = std::make_shared<spacer_t>();
    view_ptr text = std::make_shared<text_t>("ITEM TEMPLATE");
    item->add_child(indent);
    item->add_child(text);
    item->layout()->visible = false;

    item->on(EVT_MOUSE_CLICK, [this, item](event_t& evt) {
        evt.cancelled = true;
        evt.source = item.get();
        this->handle_item_click(evt);
        return true;
    });

    return item;
}

void list_t::update_item(view_ptr item, list_item_data_t data)
{
    item->cast<list_item_t>()->item_data = data;
    item->cast<list_item_t>()->find_child(view_type_e::SPACER)->layout()->width = data.indent ? data.indent : 1;
    item->cast<list_item_t>()->find_child(view_type_e::TEXT)->cast<text_t>()->set_text(data.text);
    item->layout()->visible = true;
}

void list_t::prelayout()
{
    if (!data.size())
        return;

    layout_item_ptr lo = layout();
    layout_item_ptr slo = scrollarea->layout();

    if (!subcontent) {
        lead_spacer = std::make_shared<spacer_t>();
        tail_spacer = std::make_shared<spacer_t>();
        subcontent = std::make_shared<vertical_container_t>();
        content()->add_child(lead_spacer);
        content()->add_child(subcontent);
        content()->add_child(tail_spacer);

        subcontent->layout()->fit_children_x = true;
        subcontent->layout()->fit_children_y = true;

        item_height = font()->height;
    }

    visible_items = lo->render_rect.h / item_height + VISIBLE_ITEMS_PAD;

    while (subcontent->children.size() < visible_items) {
        view_ptr vi = create_item();
        list_item_t* i = vi->cast<list_item_t>();
        subcontent->add_child(vi);
        vi->layout()->height = item_height;
    }

    // layout_item_ptr lo = layout();
    // layout_item_ptr slo = scrollarea->layout();

    scrollarea->cast<scrollarea_t>()->scroll_factor_x = font()->width;
    scrollarea->cast<scrollarea_t>()->scroll_factor_y = item_height / 2;

    first_visible = -slo->scroll_y / item_height;
    std::vector<list_item_data_t>::iterator it = data.begin();
    if (first_visible >= data.size())
        first_visible = data.size() - 1;
    if (first_visible < 0)
        first_visible = 0;
    it += first_visible;

    int vc = 0;
    for (auto c : subcontent->children) {
        if (it != data.end()) {
            list_item_data_t d = *it++;
            update_item(c, d);
            vc++;
        } else {
            c->layout()->visible = false;
        }
    }

    lead_spacer->layout()->height = (first_visible * item_height);
    lead_spacer->layout()->visible = lead_spacer->layout()->height > 1;
    tail_spacer->layout()->height = TAIL_PAD * item_height;

    int computed = lead_spacer->layout()->height + tail_spacer->layout()->height + (vc * item_height);
    int total_height = (data.size() + TAIL_PAD) * item_height;
    if (computed < total_height) {
        tail_spacer->layout()->height += total_height - computed;
    }

    tail_spacer->layout()->visible = tail_spacer->layout()->height > 1;
}

void list_t::relayout_virtual_items()
{
    layout_clear_hash(layout(), 6);
    relayout();
    relayout();
}

void list_t::render(renderer_t* renderer)
{
    panel_t::render(renderer);
    relayout_virtual_items();
}

void list_t::update_data(std::vector<list_item_data_t> _data)
{
    data = _data;
    _content_hash = 0;
}

bool list_t::handle_item_click(event_t& evt)
{
    if (evt.source) {
        select_item(((view_t*)(evt.source))->ptr());
    }
    return true;
}

void list_t::select_item(view_ptr item)
{
    if (!item)
        return;

    list_item_t* n = item->cast<list_item_t>();
    event_t evt;
    evt.type = EVT_ITEM_SELECT;
    evt.source = n;
    evt.cancelled = false;
    propagate_event(evt);
}

list_item_data_t list_t::value()
{
    return selected_data;
}

int list_t::content_hash(bool peek)
{
    if (!peek) {
        _content_hash = 1;
    }
    return _content_hash;
}