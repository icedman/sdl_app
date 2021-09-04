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
    void update() override;

    void add_child(view_item_ptr view);
    void remove_child(view_item_ptr view);
    // void delete_later();

    bool is_focused() override;
    bool is_pressed() override;
    bool is_dragged() override;
    bool is_hovered() override;
    bool is_clicked() override;

    RenImage* cache(int w, int h);

    virtual bool mouse_down(int x, int y, int button, int clicks);
    virtual bool mouse_up(int x, int y, int button);
    virtual bool mouse_click(int x, int y, int button);
    virtual bool mouse_move(int x, int y, int button);
    virtual bool mouse_drag_start(int x, int y);
    virtual bool mouse_drag_end(int x, int y);
    virtual bool mouse_drag(int x, int y);
    virtual bool mouse_wheel(int x, int y);

    virtual bool input_key(int k);
    virtual bool input_text(std::string text);
    virtual bool input_sequence(std::string text);

    virtual bool on_scroll();

    view_item_list _views;

    RenImage *_cache;
};

struct vertical_container : view_item {
    vertical_container() : view_item("container")
    {
        layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;
    }
};

struct horizontal_container : view_item {
    horizontal_container() : view_item("container")
    {
        layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
    }
};

view_item* view_get_root();
void view_set_root(view_item *item);
void view_set_focused(view_item *item);
view_item* view_get_focused();
void view_input_list(view_item_list &list, view_item_ptr item);
void view_input_events(view_item_list &list, event_list &events);
void view_input_button(int button, int x, int y, int pressed, int clicks = 0);
void view_input_wheel(int x, int y);
void view_input_key(int key);
void view_input_text(std::string text);
void view_input_sequence(std::string sequence);
 int view_input_key_mods();

#endif // VIEW_H