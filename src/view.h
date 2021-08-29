#ifndef VIEW_H
#define VIEW_H

#include "layout.h"
#include "events.h"
#include "renderer.h"

#include <map>

struct view_item;
typedef std::shared_ptr<view_item> view_item_ptr;
typedef std::vector<view_item_ptr> view_item_list;
struct view_item : layout_view {
    view_item(std::string type);
    view_item();
    ~view_item();

    std::string name;
    std::string type;

    layout_item_ptr layout() override;

    void add_child(view_item_ptr view);
    void remove_child(view_item_ptr view);
    // void delete_later();

    bool is_focused() override;
    bool is_pressed() override;
    bool is_dragged() override;
    bool is_hovered() override;
    bool is_clicked() override;

    RenImage* cache(int w, int h);

    virtual bool mouse_down(int x, int y, int button);
    virtual bool mouse_up(int x, int y, int button);
    virtual bool mouse_move(int x, int y, int button);

    view_item_list _views;

    RenImage *_cache;
};

void view_input_list(view_item_list &list, view_item_ptr item);
void view_input_events(view_item_list &list, event_list &events);
void view_input_button(int button, int x, int y, int pressed);

#endif // VIEW_H