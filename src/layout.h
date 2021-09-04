#ifndef LAYOUT_H
#define LAYOUT_H

#include <string>
#include <vector>
#include <memory>

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

enum layout_align_content {
    LAYOUT_ALIGN_CONTENT_UNKNOWN = 0,
    LAYOUT_ALIGN_CONTENT_FLEX_START,
    LAYOUT_ALIGN_CONTENT_FLEX_END,
    LAYOUT_ALIGN_CONTENT_CENTER,
    LAYOUT_ALIGN_CONTENT_STRETCH,
    LAYOUT_ALIGN_CONTENT_SPACE_BETWEEN,
    LAYOUT_ALIGN_CONTENT_SPACE_AROUND
};

struct layout_color {
    int r;
    int g;
    int b;
    int a;
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

struct layout_view {

    layout_view() 
    : disabled(false)
    , focusable(false)
    , interactive(false)
    , color({ 255, 255, 255 })
    , background({ 150, 150, 150 })
    , border_color({ 150, 150, 150 })
    , border_width(0)
    , parent(0)
    {}

    bool disabled;
    bool focusable;
    bool interactive;
    layout_view *parent;
    layout_item_ptr _layout;

    layout_color color;
    layout_color background;
    layout_color border_color;
    float border_width;

    virtual bool is_focused() { return false; }
    virtual bool is_pressed() { return false; }
    virtual bool is_dragged() { return false; }
    virtual bool is_hovered() { return false; }
    virtual bool is_clicked() { return false; }
    virtual layout_item_ptr layout() { return 0; }
    virtual void set_layout(layout_item_ptr l) { _layout = l; }
    
    virtual void prelayout() {}
    virtual void postlayout() {}
    virtual void update() {}
    virtual void render() {}
};

struct layout_item {
    layout_item()
        : order(0)
        , grow(1)
        , shrink(0)
        , flex_basis(0)
        , x(0)
        , y(0)
        , scroll_x(0)
        , scroll_y(0)
        , width(0)
        , height(0)
        , fit_children(true)
        , margin(0)
        , visible(true)
        , wrap(false)
        , align_self(LAYOUT_ALIGN_UNKNOWN)
        , align_content(LAYOUT_ALIGN_CONTENT_UNKNOWN)
        , align(LAYOUT_ALIGN_FLEX_START)
        , justify(LAYOUT_JUSTIFY_FLEX_START)
        , direction(LAYOUT_FLEX_DIRECTION_COLUMN)
        , constraint({0,0,0,0})
        , render_rect({0,0,0,0})
        , view(0)
    {
        rgb = {
            r: 255,
            g: 0,
            b: 255
        };
    }

    std::string name;

    layout_color rgb;
    layout_constraint constraint; // passed down
    layout_rect rect;             // relative computed
    layout_rect render_rect;      // final computed

    int order;
    bool visible;
    bool wrap;
    int margin;
    int x, y;
    int scroll_x, scroll_y;
    int width, height;
    bool fit_children;
    int grow;                   // flex-grow
    int shrink;                 // not yet implemented
    int flex_basis;             // not yet implemented
    layout_align_items align;
    layout_align_content align_content; // not yet implemented
    layout_align_items align_self;
    layout_justify_content justify;
    layout_flex_direction direction;
    layout_item_list children;

    layout_view *view;

    bool is_column() {
        return direction == LAYOUT_FLEX_DIRECTION_COLUMN ||
                direction == LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE;
    }
    bool is_row() {
        return direction == LAYOUT_FLEX_DIRECTION_ROW ||
                direction == LAYOUT_FLEX_DIRECTION_ROW_REVERSE;
    }
};

void layout_run(layout_item_ptr item, layout_constraint constraint);
void layout_sort(layout_item_ptr item);
void layout_render_list(layout_item_list& list, layout_item_ptr item);
void layout_request();
bool layout_should_run();

#endif // LAYOUT_H