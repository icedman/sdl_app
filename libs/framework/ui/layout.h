#ifndef LAYOUT_H
#define LAYOUT_H

#include "color.h"
#include "rect.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#define LAYOUT_HASH_SEED 0xfc

enum layout_flex_direction_e {
    LAYOUT_FLEX_DIRECTION_UNKNOWN,
    LAYOUT_FLEX_DIRECTION_COLUMN,
    LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE,
    LAYOUT_FLEX_DIRECTION_ROW,
    LAYOUT_FLEX_DIRECTION_ROW_REVERSE
};

enum layout_justify_content_e {
    LAYOUT_JUSTIFY_UNKNOWN = 0,
    LAYOUT_JUSTIFY_FLEX_START,
    LAYOUT_JUSTIFY_FLEX_END,
    LAYOUT_JUSTIFY_CENTER,
    LAYOUT_JUSTIFY_SPACE_BETWEEN,
    LAYOUT_JUSTIFY_SPACE_AROUND,
    LAYOUT_JUSTIFY_SPACE_EVENLY
};

enum layout_align_items_e {
    LAYOUT_ALIGN_UNKNOWN = 0,
    LAYOUT_ALIGN_FLEX_START,
    LAYOUT_ALIGN_FLEX_END,
    LAYOUT_ALIGN_CENTER,
    LAYOUT_ALIGN_STRETCH
};

enum layout_align_content_e {
    LAYOUT_ALIGN_CONTENT_UNKNOWN = 0,
    LAYOUT_ALIGN_CONTENT_FLEX_START,
    LAYOUT_ALIGN_CONTENT_FLEX_END,
    LAYOUT_ALIGN_CONTENT_CENTER,
    LAYOUT_ALIGN_CONTENT_STRETCH,
    LAYOUT_ALIGN_CONTENT_SPACE_BETWEEN,
    LAYOUT_ALIGN_CONTENT_SPACE_AROUND
};

struct constraint_t {
    int min_width;
    int min_height;
    int max_width;
    int max_height;
};

struct layout_item_t;

typedef std::shared_ptr<layout_item_t> layout_item_ptr;
typedef std::vector<layout_item_ptr> layout_item_list;

typedef std::function<bool(layout_item_t*)> layout_callback_t;

struct layout_item_t {
    layout_item_t()
        : x(0)
        , y(0)
        , scroll_x(0)
        , scroll_y(0)
        , width(0)
        , height(0)
        , order(0)
        , stack(false)
        , visible(true)
        , offscreen(false)
        , wrap(false)
        , margin(0)
        , margin_left(0)
        , margin_right(0)
        , margin_top(0)
        , margin_bottom(0)
        , preferred_constraint({ 0, 0, 0, 0 })
        , fit_children_x(false)
        , fit_children_y(false)
        , grow(1)
        , shrink(0)
        , flex_basis(0)
        , align(LAYOUT_ALIGN_FLEX_START)
        , align_content(LAYOUT_ALIGN_CONTENT_UNKNOWN)
        , align_self(LAYOUT_ALIGN_UNKNOWN)
        , justify(LAYOUT_JUSTIFY_FLEX_START)
        , direction(LAYOUT_FLEX_DIRECTION_COLUMN)

        , constraint({ 0, 0, 0, 0 })
        , render_rect({ 0, 0, 0, 0 })
        , rect({ 0, 0, 0, 0 })

        , content_hash(0)
        , state_hash(0)
        , skip_layout(false)
        , rgb({ 255, 0, 255 })
    {
    }

    std::string name;
    color_t rgb;

    int x;
    int y;
    int scroll_x;
    int scroll_y;
    int width, height;

    int order;
    bool stack;
    bool visible;
    bool offscreen;
    bool wrap;
    int margin;
    int margin_left;
    int margin_right;
    int margin_top;
    int margin_bottom;
    constraint_t preferred_constraint;
    bool fit_children_x;
    bool fit_children_y;
    float grow; // flex-grow
    float shrink; // not yet implemented
    int flex_basis; // not yet implemented
    layout_align_items_e align;
    layout_align_content_e align_content; // not yet implemented
    layout_align_items_e align_self;
    layout_justify_content_e justify;
    layout_flex_direction_e direction;

    constraint_t constraint; // passed down
    rect_t render_rect; // final computed
    rect_t rect; // relative computed

    float _flex; // computed grow/shrink
    int _width, _height;
    rect_t _rect;
    int content_hash;
    int state_hash;
    bool skip_layout;

    bool is_column()
    {
        return direction == LAYOUT_FLEX_DIRECTION_COLUMN || direction == LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE;
    }
    bool is_row()
    {
        return direction == LAYOUT_FLEX_DIRECTION_ROW || direction == LAYOUT_FLEX_DIRECTION_ROW_REVERSE;
    }

    layout_callback_t prelayout;
    layout_callback_t postlayout;

    layout_item_list children;
};

void layout_request(layout_item_ptr item);
void layout_request();

bool layout_should_run();

void layout_run_requests();
void layout_run(layout_item_ptr item, constraint_t constraint, bool recompute = false);
void layout_sort(layout_item_ptr item);
void layout_compute_absolute_position(layout_item_ptr item);

#endif // LAYOUT_H
