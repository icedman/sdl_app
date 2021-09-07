#ifndef LIST_VIEW_H
#define LIST_VIEW_H

#include "view.h"
#include "panel.h"

struct list_item_data_t {
    std::string icon;
    std::string text;
    int indent;
    void *data;
    bool selected;

    bool equals(list_item_data_t d) {
        return (icon == d.icon && text == d.text && indent == d.indent && data == d.data);
    }
};

struct list_view;
struct list_item_view : horizontal_container {

    list_item_view();
    bool mouse_click(int x, int y, int button) override;
    void render() override;

    list_item_data_t data;
    list_view* container;
};

struct list_view : panel_view {
    list_view(std::vector<list_item_data_t> items);
    list_view();
    
    virtual void prelayout() override;
    virtual void update() override;

    virtual void select_item(list_item_view *item);

    std::vector<list_item_data_t> data;
    list_item_view *selected;
};

#endif // LIST_VIEW_H