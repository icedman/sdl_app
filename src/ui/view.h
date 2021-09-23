#ifndef VIEW_H
#define VIEW_H

#include "events.h"
#include "layout.h"
#include "renderer.h"
#include "style.h"

#include <map>

struct view_item;
typedef std::shared_ptr<view_item> view_item_ptr;
typedef std::vector<view_item_ptr> view_item_list;
struct view_item : layout_view, event_object_t {
    view_item(std::string type);
    view_item();
    ~view_item();

    std::string name;
    std::string type;

    layout_item_ptr layout() override;
    void update() override;
    void render() override;

    void add_child(view_item_ptr view);
    void remove_child(view_item_ptr view);
    void relayout();

    bool is_focused() override;
    bool is_pressed() override;
    bool is_dragged() override;
    bool is_hovered() override;
    bool is_clicked() override;

    RenImage* cache(int w, int h);
    void destroy_cache();

    virtual bool mouse_down(int x, int y, int button, int clicks) { return false; };
    virtual bool mouse_up(int x, int y, int button) { return false; };
    virtual bool mouse_move(int x, int y, int button) { return false; };
    virtual bool mouse_click(int x, int y, int button) { return false; };
    virtual bool mouse_drag_start(int x, int y) { return false; };
    virtual bool mouse_drag_end(int x, int y) { return false; };
    virtual bool mouse_drag(int x, int y) { return false; };
    virtual bool mouse_wheel(int x, int y) { return false; };
    virtual bool input_key(int k) { return false; };
    virtual bool input_text(std::string text) { return false; };
    virtual bool input_sequence(std::string text) { return false; };

    int on(event_type_e event_type, event_callback_t callback);
    void propagate_event(event_t& event);

    view_item_list _views;
    event_callback_list callbacks;

    RenImage* cached_image;
    bool cache_enabled;

    template <class T>
    static T* cast(view_item_ptr v) { return (T*)(v.get()); };
};

struct vertical_container : view_item {
    vertical_container()
        : view_item("container")
    {
        layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;
    }
};

struct horizontal_container : view_item {
    horizontal_container()
        : view_item("container")
    {
        layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
    }
};

view_item* view_get_root();
void view_set_root(view_item* item);
void view_set_focused(view_item* item);
view_item* view_get_focused();
view_item* view_shift_focus(int x, int y);

void view_input_list(view_item_list& list, view_item_ptr item);
void view_input_events(view_item_list& list, event_list& events);
void view_input_button(int button, int x, int y, int pressed, int clicks, event_t event);
void view_input_wheel(int x, int y, event_t event);
void view_input_key(int key, event_t event);
void view_input_text(std::string text, event_t event);
void view_input_sequence(std::string sequence, event_t event);

#endif // VIEW_H