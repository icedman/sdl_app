#ifndef LAYOUT_H
#define LAYOUT_H

#include <string>
#include <vector>
#include <memory>

struct layout_view {

    layout_view() 
    : disabled(false)
    , can_focus(false)
    , can_press(false)
    , can_drag(false)
    , can_hover(false)
    , can_input(false)
    {}

    bool disabled;
    bool can_focus;
    bool can_press;
    bool can_drag;
    bool can_hover;
    bool can_input;

    virtual bool is_focused() { return false; }
    virtual bool is_pressed() { return false; }
    virtual bool is_dragged() { return false; }
    virtual bool is_hovered() { return false; }
    virtual bool is_clicked() { return false; }
    virtual void precalculate() {}
};

enum layout_flex_direction {
    LAYOUT_FLEX_DIRECTION_UNKNOWN,
    LAYOUT_FLEX_DIRECTION_COLUMN,
    LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE,
    LAYOUT_FLEX_DIRECTION_ROW,
    LAYOUT_FLEX_DIRECTION_ROW_REVERSE
};

enum layout_justify_content {
    LAYOUT_JUSTIFY_UNKNOWN = 0,
    LAYOUT_JUSTIFY_FLEX_START,
    LAYOUT_JUSTIFY_FLEX_END,
    LAYOUT_JUSTIFY_CENTER,
    LAYOUT_JUSTIFY_SPACE_BETWEEN,
    LAYOUT_JUSTIFY_SPACE_AROUND,
    LAYOUT_JUSTIFY_SPACE_EVENLY
};

enum layout_align_items {
    LAYOUT_ALIGN_UNKNOWN = 0,
    LAYOUT_ALIGN_FLEX_START,
    LAYOUT_ALIGN_FLEX_END,
    LAYOUT_ALIGN_CENTER,
    LAYOUT_ALIGN_STRETCH
};

struct layout_rgb {
    int r;
    int g;
    int b;
};

struct layout_rect {
    int x;
    int y;
    int w;
    int h;
};

struct layout_constraint {
    int min_width;
    int min_height;
    int max_width;
    int max_height;
};

struct layout_item;

typedef std::shared_ptr<layout_item> layout_item_ptr;
typedef std::vector<layout_item_ptr> layout_item_list;

struct layout_item {
    layout_item();

    std::string name;

    layout_rgb rgb;
    layout_constraint constraint; // passed down
    layout_rect rect;             // relative computed
    layout_rect render_rect;      // final computed

    bool visible;
    bool wrap;
    int margin;
    int x, y;
    int width, height;
    int flex;                   // flex-grow
    int flex_shrink;            // not yet implemented
    int flex_basis;             // not yet implemented
    layout_align_items align;
    layout_align_items align_self;  // not yet implemented
    layout_justify_content justify;
    layout_flex_direction direction;
    layout_item_list children;

    layout_view *view;
};

void layout_run(layout_item_ptr item, layout_constraint constraint);
void layout_render_list(layout_item_list& list, layout_item_ptr item);

#endif // LAYOUT_H