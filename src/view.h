#ifndef VIEW_H
#define VIEW_H

#include "layout.h"

#include <map>

struct view_item;
typedef std::shared_ptr<view_item> view_item_ptr;
typedef std::vector<view_item_ptr> view_item_list;
struct view_item : layout_view {
    view_item();

    std::string name;
    std::string type;

    layout_item_ptr layout();

    void add_child(view_item_ptr view);
    void add_child(view_item *view);
    void add_child(layout_item_ptr item);
    void remove_child(view_item_ptr view);
    void remove_child(view_item *view);
    void remove_child(layout_item_ptr item);

    bool can_focus;
    bool can_press;
    bool can_drag;
    bool can_input;

    layout_item_ptr _layout;
    view_item_list _views;
};

#endif // VIEW_H