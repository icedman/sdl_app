#ifndef TABBAR_H
#define TABBAR_H

#include "list.h"

struct tabbar_t : panel_t {
    tabbar_t();

    DECLAR_VIEW_TYPE(view_type_e::TABBAR, panel_t)

    virtual view_ptr create_item();
    virtual void update_item(view_ptr item, list_item_data_t data);

    void prelayout() override;
    void update_data(std::vector<list_item_data_t> data);
    bool handle_item_click(event_t& evt);

    void select_item(view_ptr item);
    list_item_data_t value();

    std::vector<list_item_data_t> data;

    list_item_data_t selected_data;
};

struct tabbed_content_t : vertical_container_t {
    tabbed_content_t();

    DECLAR_VIEW_TYPE(view_type_e::TABBED_CONTENT, vertical_container_t)

    view_ptr content() { return _content; }
    view_ptr tabbar() { return _tabbar; }

    view_ptr _tabbar;
    view_ptr _content;
};

#endif // TABBAR_H