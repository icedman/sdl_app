#include "view.h"

view_item::view_item()
    : type("view")
{}

layout_item_ptr view_item::layout()
{
    if (!_layout) {
        _layout = std::make_shared<layout_item>();
        _layout->view = this;
    }
    return _layout;
}

void view_item::add_child(view_item_ptr view)
{
    _views.push_back(view);
    add_child(view->layout());
}

void view_item::add_child(view_item *view)
{
    add_child(view->layout());
}

void view_item::add_child(layout_item_ptr item)
{
    for(auto i : layout()->children) {
        if (i == item) {
            return;
        }
    }
    layout()->children.push_back(item);
}

void view_item::remove_child(view_item_ptr view)
{
    view_item_list::iterator it = _views.begin();
    while(it++ != _views.end()) {
        if (*it == view) {
            _views.erase(it);
            return;
        }
    }
    remove_child(view->layout());
}

void view_item::remove_child(view_item *view)
{
    remove_child(view->layout());
}

void view_item::remove_child(layout_item_ptr item)
{
    layout_item_list::iterator it = layout()->children.begin();
    while(it++ != layout()->children.end()) {
        if (*it == item) {
            layout()->children.erase(it);
            return;
        }
    }
}
