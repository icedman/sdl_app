#ifndef LIST_VIEW_H
#define LIST_VIEW_H

#include "animation.h"
#include "panel.h"
#include "view.h"

struct list_item_data_t {
    std::string icon;
    std::string text;
    int indent;
    void* data;
    std::string value;
    int score;

    bool equals(list_item_data_t d)
    {
        return (icon == d.icon && text == d.text && indent == d.indent && data == d.data);
    }
};

struct list_view;
struct list_item_view : horizontal_container {

    list_item_view();
    bool mouse_click(int x, int y, int button) override;
    void render() override;
    void prerender() override;

    list_item_data_t data;
    list_view* container;

    view_item_ptr icon;
    view_item_ptr text;
    view_item_ptr depth;
};

struct list_view : panel_view {
    list_view(std::vector<list_item_data_t> items);
    list_view();

    virtual void prelayout() override;
    virtual void update(int millis = 0) override;
    virtual void render() override;
    virtual bool input_sequence(std::string text) override;

    virtual void select_item(list_item_view* item);
    bool is_item_selected(list_item_view* item);
    bool is_item_focused(list_item_view* item);

    void select_focused();
    void focus_previous();
    void focus_next();
    void clear();

    bool ensure_visible_cursor();

    virtual view_item_ptr create_item();

    list_item_view* item_from_value(std::string value);

    std::vector<list_item_data_t> data;

    list_item_view* _value;
    list_item_view* _prev_value;
    list_item_view* _focused_value;

    bool autoscroll;

    static bool compare_item(struct list_item_data_t& f1, struct list_item_data_t& f2);

    animate_ease_values scroll_animation;
};

#endif // LIST_VIEW_H