#include "list.h"
#include "hash.h"
#include "image.h"
#include "scrollarea.h"
#include "system.h"
#include "text.h"

#define VISIBLE_ITEMS_PAD 4
#define TAIL_PAD 16

list_item_data_t null_selected_data = { .index = -1 };

bool list_item_data_t::compare_item(struct list_item_data_t& f1, struct list_item_data_t& f2)
{
    if (f1.score == f2.score) {
        return f1.text < f2.text;
    }
    return f1.score < f2.score;
}

list_item_t::list_item_t()
    : horizontal_container_t()
{
    can_hover = true;
    layout()->fit_children_x = false;
    layout()->fit_children_y = true;
    layout()->margin = 2;
}

list_t* list_item_t::list()
{
    view_t* p = parent;
    while (p) {
        if (p->is_type_of(view_type_e::LIST)) {
            return (list_t*)p;
        }
        p = p->parent;
    }
    return NULL;
}

bool list_item_t::is_selected()
{
    if (list()) {
        if (list()->selected_data.equals(item_data)) {
            return true;
        }
    }
    return false;
}

int list_item_t::content_hash(bool peek)
{
    struct list_item_data_hash_t {
        bool selected;
    };

    list_item_data_hash_t data = {
        is_selected()
    };

    int _hash = murmur_hash(&data, sizeof(list_item_data_hash_t), CONTENT_HASH_SEED);
    int hash = hash_combine(_hash, murmur_hash(&item_data, sizeof(list_item_data_t), CONTENT_HASH_SEED));
    if (!peek) {
        _content_hash = hash;
    }
    return hash;
}

void list_item_t::render(renderer_t* renderer)
{
    if (is_hovered(this) || is_selected()) {
        render_frame(renderer);
    }
}

list_t::list_t()
    : panel_t()
    , visible_items(0)
    , defer_relayout(DEFER_LAYOUT_FRAMES)
    , tail_pad(TAIL_PAD)
    , selected_index(-1)
{
    layout()->prelayout = [this](layout_item_t* item) {
        this->prelayout();
        return true;
    };

    layout()->name = "list";
}

void list_t::select_next()
{
    selected_data = null_selected_data;

    if (!data.size())
        return;
    if (selected_index == -1) {
        selected_index = 0;
    } else {
        selected_index++;
    }

    if (selected_index >= data.size()) {
        selected_index = data.size() - 1;
    }

    selected_data = data[selected_index];
    relayout_virtual_items();
}

void list_t::select_previous()
{
    selected_data = null_selected_data;

    if (!data.size())
        return;
    if (selected_index == -1) {
        selected_index = 0;
    } else {
        selected_index--;
    }

    if (selected_index < 0) {
        selected_index = 0;
    }

    selected_data = data[selected_index];
    relayout_virtual_items();
}

view_ptr list_t::create_item()
{
    view_ptr item = std::make_shared<list_item_t>();
    item->layout()->fit_children_x = true;
    item->layout()->fit_children_y = true;
    view_ptr indent = std::make_shared<spacer_t>();
    view_ptr icon = std::make_shared<image_view_t>();
    view_ptr text = std::make_shared<text_t>("ITEM TEMPLATE");
    item->add_child(indent);
    item->add_child(icon);
    item->add_child(text);
    item->layout()->visible = false;

    view_t* _item = item.get();
    item->on(EVT_MOUSE_CLICK, [this, _item](event_t& evt) {
        evt.cancelled = true;
        evt.source = _item;
        this->handle_item_click(evt);
        return true;
    });

    return item;
}

void list_t::update_item(view_ptr item, list_item_data_t data)
{
    item->cast<list_item_t>()->item_data = data;
    item->cast<list_item_t>()->find_child(view_type_e::SPACER)->layout()->width = data.indent ? data.indent : 1;
    if (data.icon != "") {
        item->cast<list_item_t>()->find_child(view_type_e::IMAGE)->cast<image_view_t>()->load_icon(data.icon, 24, 24);
        item->cast<list_item_t>()->find_child(view_type_e::IMAGE)->layout()->visible = true;
        item->cast<list_item_t>()->find_child(view_type_e::IMAGE)->layout()->width = 32;
    } else {
        item->cast<list_item_t>()->find_child(view_type_e::IMAGE)->layout()->visible = false;
    }
    item->cast<list_item_t>()->find_child(view_type_e::TEXT)->cast<text_t>()->set_text(data.text);
    item->layout()->visible = true;
}

int list_t::item_height()
{
    return font()->height;
}

void list_t::prerender()
{
    if (defer_relayout > 0) {
        relayout_virtual_items();
        defer_relayout--;

        // hacky .. scrollbars show/hide
        event_t evt;
        evt.sx = 0;
        evt.sy = 0;
        panel_t::handle_mouse_wheel(evt);
        relayout_virtual_items();
    }

    panel_t::prerender();
}

void list_t::relayout_virtual_items()
{
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
    }

    visible_items = lo->render_rect.h / item_height() + VISIBLE_ITEMS_PAD;

    while (subcontent->children.size() < visible_items) {
        view_ptr vi = create_item();
        list_item_t* i = vi->cast<list_item_t>();
        subcontent->add_child(vi);
        vi->layout()->height = item_height();
    }

    // layout_item_ptr lo = layout();
    // layout_item_ptr slo = scrollarea->layout();

    scrollarea->cast<scrollarea_t>()->scroll_factor_x = font()->width;
    scrollarea->cast<scrollarea_t>()->scroll_factor_y = item_height() / 2;

    first_visible = -slo->scroll_y / item_height();
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

    lead_spacer->layout()->height = (first_visible * item_height());
    lead_spacer->layout()->visible = lead_spacer->layout()->height > 1;
    tail_spacer->layout()->height = tail_pad * item_height();

    int computed = lead_spacer->layout()->height + tail_spacer->layout()->height + (vc * item_height());
    int total_height = (data.size() + tail_pad) * item_height();
    if (computed < total_height) {
        tail_spacer->layout()->height += total_height - computed;
    }

    tail_spacer->layout()->visible = tail_spacer->layout()->height > 1;

    layout_clear_hash(layout(), 6);
    relayout();
}

void list_t::update_data(std::vector<list_item_data_t> _data)
{
    data = _data;
    if (selected_index >= data.size()) {
        selected_index = -1;
    }

    int idx = 0;
    for (list_item_data_t& d : data) {
        d.index = idx++;
    }

    // hacky
    int h = layout()->render_rect.h;
    layout_run(layout(), { 0, 0, layout()->render_rect.w, 400 });
    layout_run(layout(), { 0, 0, layout()->render_rect.w, h });

    relayout_virtual_items();
    rerender();
}

bool list_t::handle_mouse_wheel(event_t& event)
{
    relayout_virtual_items();
    return panel_t::handle_mouse_wheel(event);
}

bool list_t::handle_scrollbar_move(event_t& event)
{
    relayout_virtual_items();
    return panel_t::handle_scrollbar_move(event);
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

void list_t::select_item(int index)
{
    if (index == -2) {
        index = selected_index;
    }
    selected_index = index;
    if (selected_index == -1) {
        selected_data = null_selected_data;
        return;
    }

    view_ptr item;
    for (auto c : subcontent->children) {
        list_item_t* i = c->cast<list_item_t>();
        if (i->item_data.index == index) {
            select_item(c);
            break;
        }
    }
    return;
}

list_item_data_t list_t::value()
{
    return selected_data;
}

int list_t::content_hash(bool peek)
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