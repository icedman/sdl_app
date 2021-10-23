#ifndef LIST_H
#define LIST_H

#include "panel.h"
#include "view.h"

struct list_item_data_t {
    std::string value;
    std::string text;
    std::string icon;
    int indent;
    void* data;
    int score;

    bool equals(list_item_data_t d)
    {
        return (value == d.value && text == d.text && icon == d.icon && indent == d.indent && data == d.data);
    }
};

struct list_item_t : horizontal_container_t {
    list_item_t();

    DECLAR_VIEW_TYPE(LIST_ITEM, horizontal_container_t)

    void render(renderer_t* renderer) override;
    int content_hash(bool peek) override;

    list_item_data_t item_data;
};

struct list_t : panel_t {
    list_t();

    DECLAR_VIEW_TYPE(LIST, panel_t)

    void prerender() override;
    void update_data(std::vector<list_item_data_t> data);
    void relayout_virtual_items();

    virtual view_ptr create_item();
    virtual void update_item(view_ptr item, list_item_data_t data);

    bool handle_item_click(event_t& evt);

    void select_item(view_ptr item);
    list_item_data_t value();

    view_ptr lead_spacer;
    view_ptr tail_spacer;
    view_ptr subcontent;
    std::vector<list_item_data_t> data;

    int first_visible;
    int visible_items;
    int item_height;

    list_item_data_t selected_data;
    
    bool handle_mouse_wheel(event_t& event);
    bool handle_scrollbar_move(event_t& event);
    int content_hash(bool peek) override;

    int defer_relayout;
};

#endif LIST_H