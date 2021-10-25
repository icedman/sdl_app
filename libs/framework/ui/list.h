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
    int index;

    std::string icon_font;
    color_t icon_color;

    bool equals(list_item_data_t d)
    {
        return (value == d.value && text == d.text && icon == d.icon && indent == d.indent && data == d.data);
    }

    static bool compare_item(struct list_item_data_t& f1, struct list_item_data_t& f2);
};

struct list_t;
struct list_item_t : horizontal_container_t {
    list_item_t();

    DECLAR_VIEW_TYPE(LIST_ITEM, horizontal_container_t)

    void render(renderer_t* renderer) override;
    int content_hash(bool peek) override;

    bool is_selected();

    list_t* list();
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
    virtual int item_height();

    bool handle_item_click(event_t& evt);

    void scroll_to_index(int index);
    void select_item(int i = -2);
    void select_item(view_ptr item);
    void select_next();
    void select_previous();
    list_item_data_t value();

    view_ptr lead_spacer;
    view_ptr tail_spacer;
    view_ptr subcontent;
    std::vector<list_item_data_t> data;

    int first_visible;
    int visible_items;

    list_item_data_t selected_data;
    int selected_index;

    bool handle_mouse_wheel(event_t& event) override;
    bool handle_scrollbar_move(event_t& event) override;
    int content_hash(bool peek) override;

    int defer_relayout;
    int tail_pad;
};

#endif LIST_H