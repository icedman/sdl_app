#include "layout.h"
#include "hash.h"

#include <algorithm>

#define _LOG printf

static int items_visited = 0;
static int items_computed = 0;

void _layout_run(layout_item_ptr item, constraint_t constraint);

void layout_reverse_items(layout_item_ptr item, int constraint)
{
    if (item->direction == LAYOUT_FLEX_DIRECTION_ROW_REVERSE) {
        for (auto child : item->children) {
            if (!child->visible) {
                continue;
            }
            if (child->stack) {
                continue;
            }
            child->rect.x = constraint - (child->rect.x + child->rect.w);
        }
    }
    if (item->direction == LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE) {
        for (auto child : item->children) {
            if (!child->visible) {
                continue;
            }
            if (child->stack) {
                continue;
            }
            child->rect.y = constraint - (child->rect.y + child->rect.h);
        }
    }
}

void layout_position_items(layout_item_ptr item)
{
    constraint_t c = item->constraint;
    c.max_width -= (item->margin_left + item->margin_right);
    c.max_height -= (item->margin_top + item->margin_bottom);

    int constraint = c.max_width;

    int wd = 0;
    int hd = 0;
    if (item->is_row()) {
        wd = 1;
    } else {
        hd = 1;
        constraint = c.max_height;
    }

    // wrap
    std::vector<layout_item_list> groups;
    groups.push_back(layout_item_list());
    int consumed = 0;
    for (auto child : item->children) {
        if (!child->visible) {
            continue;
        }
        if (child->stack) {
            continue;
        }
        child->rect.x = 0;
        child->rect.y = 0;
        if (consumed + child->rect.w * wd + child->rect.h * hd > constraint) {
            consumed = 0;
            if (item->wrap) {
                groups.push_back(layout_item_list());
            }
        }
        consumed += child->rect.w * wd;
        consumed += child->rect.h * hd;
        layout_item_list& group = groups.back();
        group.push_back(child);
    }

    // justify content
    int xx = 0;
    int yy = 0;
    for (auto group : groups) {
        int spaceRemaining = constraint;
        for (auto child : group) {
            spaceRemaining -= child->rect.w * wd;
            spaceRemaining -= child->rect.h * hd;
        }

        if (spaceRemaining <= 0) {
            spaceRemaining = 0;
        }

        int visibleItems = group.size();
        int offsetStart = 0;
        int offset = 0;
        int offsetInc = 0;
        int justify = item->justify;
        if (visibleItems == 1 && justify == LAYOUT_JUSTIFY_SPACE_BETWEEN) {
            justify = LAYOUT_JUSTIFY_CENTER;
        }
        switch (justify) {
        case LAYOUT_JUSTIFY_FLEX_END:
            offsetStart = spaceRemaining;
            break;
        case LAYOUT_JUSTIFY_CENTER:
            offsetStart = spaceRemaining / 2;
            break;
        case LAYOUT_JUSTIFY_SPACE_BETWEEN:
            if (visibleItems > 1) {
                offsetInc = spaceRemaining / (visibleItems - 1);
            }
            break;
        case LAYOUT_JUSTIFY_SPACE_EVENLY: // not yet implemented
        case LAYOUT_JUSTIFY_SPACE_AROUND:
            if (visibleItems > 0) {
                offset = spaceRemaining / (visibleItems + 1);
                offsetInc = offset;
            }
            break;
        }

        int ww = 0;
        int hh = 0;

        for (auto child : group) {
            child->rect.x = (xx * hd) + ((offsetStart + offset) * wd);
            child->rect.y = (yy * wd) + ((offsetStart + offset) * hd);

            if (item->fit_children_x && item->rect.w < child->rect.x + child->rect.w + (item->margin_left + item->margin_right)) {
                item->rect.w = child->rect.x + child->rect.w + (item->margin_left + item->margin_right);
            }
            if (item->fit_children_y && item->rect.h < child->rect.y + child->rect.h + (item->margin_top + item->margin_bottom)) {
                item->rect.h = child->rect.y + child->rect.h + (item->margin_top + item->margin_bottom);
            }

            offset += child->rect.w * wd;
            offset += child->rect.h * hd;
            offset += offsetInc;

            if (child->rect.w > ww)
                ww = child->rect.w;
            if (child->rect.h > hh)
                hh = child->rect.h;
        }

        xx += ww;
        yy += hh;
    }

    // align items
    for (auto child : item->children) {
        if (!child->visible) {
            continue;
        }
        if (child->stack) {
            continue;
        }
        int alignOffset = (c.max_width - child->rect.w) * hd;
        alignOffset += (c.max_height - child->rect.h) * wd;

        int align = child->align_self ? child->align_self : item->align;
        switch (align) {
        case LAYOUT_ALIGN_FLEX_START:
            alignOffset = 0;
            break;
        case LAYOUT_ALIGN_CENTER:
            alignOffset *= 0.5f;
            break;
        }

        child->rect.x += alignOffset * hd;
        child->rect.y += alignOffset * wd;
    }
}

void layout_compute_fit(layout_item_ptr item)
{
    item->_rect = { 0, 0, 0, 0 };

    // compute fit
    int x1 = item->rect.x;
    int y1 = item->rect.y;
    int x2 = 0;
    int y2 = 0;
    for (auto child : item->children) {
        if (!child->visible) {
            continue;
        }
        if (x1 > child->rect.x) {
            x1 = child->rect.x;
        }
        if (y1 > child->rect.y) {
            y1 = child->rect.y;
        }
        if (x2 < child->rect.x + child->rect.w) {
            x2 = child->rect.x + child->rect.w;
        }
        if (y2 < child->rect.y + child->rect.h) {
            y2 = child->rect.y + child->rect.h;
        }
    }

    item->_rect = { x1, y1, x2 - x1, y2 - y1 };
    item->_rect.w += item->margin_left + item->margin_right;
    item->_rect.h += item->margin_top + item->margin_bottom;
}

void layout_stack_run(layout_item_ptr item, constraint_t constraint)
{
    rect_t rect = {
        x : item->x,
        y : item->y,
        w : item->width ? item->width : constraint.max_width,
        h : item->height ? item->height : constraint.max_height
    };
    item->rect = rect;
    item->render_rect = rect;
    item->constraint = constraint;

    if (!item->visible) {
        item->rect = { 0, 0, 0, 0 };
        return;
    }

    if (!item->children.size()) {
        return;
    }

    constraint.max_width = rect.w;
    constraint.max_height = rect.h;
    for (auto child : item->children) {
        _layout_run(child, constraint);
    }

    // if (item->fit_children) {
    // layout_compute_fit(item);
    // item->rect.w = item->_rect.w;
    // item->rect.h = item->_rect.h;
    // printf("%d %d\n", item->_rect.w, item->_rect.h);
    // }
}

void layout_horizontal_run(layout_item_ptr item, constraint_t constraint)
{
    rect_t rect = {
        x : item->x,
        y : item->y,
        w : constraint.max_width,
        h : constraint.max_height
    };

    if (item->width) {
        rect.w = item->width;
    }

    item->rect = rect;
    item->render_rect = rect;
    item->constraint = constraint;

    constraint.max_width -= (item->margin_left + item->margin_right);
    constraint.max_height -= (item->margin_top + item->margin_bottom);

    if (!item->visible) {
        item->rect = { 0, 0, 0, 0 };
        return;
    }

    if (!item->children.size()) {
        return;
    }

    layout_item_list fixedItems;
    layout_item_list flexItems;
    layout_item_list stackedItems;

    int visibleItems = 0;
    int totalFlex = 0;
    int totalFlexBasis = 0;
    int totalFixed = 0;
    for (auto child : item->children) {
        if (!child->visible) {
            continue;
        }
        if (child->stack) {
            stackedItems.push_back(child);
            continue;
        }
        visibleItems++;
        int cw = child->width;
        // if (child->fit_children_x) {
        //     _layout_run(child, { 0, 0, 0, 0 });
        //     layout_compute_fit(child);
        //     cw = child->_rect.w;
        // }
        if (cw != 0) {
            fixedItems.push_back(child);
            totalFixed += cw;
            int cc = child->height;
            if (cc > 0 && cc < constraint.min_height) {
                cc = constraint.min_height;
            }
            if (cc == 0 || cc > constraint.max_height) {
                cc = constraint.max_height;
            }
            _layout_run(child, { 0, 0, cw, cc });
        } else {
            flexItems.push_back(child);
            totalFlex += child->grow;
            totalFlexBasis += child->flex_basis;
        }
    }

    int spaceRemaining = constraint.max_width - totalFixed;
    if (spaceRemaining < 0)
        spaceRemaining = 0;

    // first pass : constraints
    for (auto child : flexItems) {
        child->_flex = child->grow;
        int ww = spaceRemaining * child->grow / totalFlex;
        if (child->preferred_constraint.max_width && ww > child->preferred_constraint.max_width) {
            child->_width = child->preferred_constraint.max_width;
            child->_flex = 0;
        }
    }

    // second pass : distribute
    for (auto child : flexItems) {
        int ww = spaceRemaining * child->_flex / totalFlex;
        if (child->_flex == 0 && child->_width) {
            ww = child->_width;
        }
        _layout_run(child, { 0, 0, ww, constraint.max_height });
    }

    for (auto child : stackedItems) {
        _layout_run(child, constraint);
    }

    layout_position_items(item);
    layout_reverse_items(item, constraint.max_width);
}

void layout_vertical_run(layout_item_ptr item, constraint_t constraint)
{
    rect_t rect = {
        x : item->x,
        y : item->y,
        w : constraint.max_width,
        h : constraint.max_height
    };

    if (item->height) {
        rect.h = item->height;
    }

    item->rect = rect;
    item->render_rect = rect;
    item->constraint = constraint;

    constraint.max_width -= (item->margin_left + item->margin_right);
    constraint.max_height -= (item->margin_top + item->margin_bottom);

    if (!item->visible) {
        item->rect = { 0, 0, 0, 0 };
        return;
    }

    if (!item->children.size()) {
        return;
    }

    layout_item_list fixedItems;
    layout_item_list flexItems;
    layout_item_list stackedItems;

    int visibleItems = 0;
    int totalFlex = 0;
    int totalFlexBasis = 0;
    int totalFixed = 0;
    for (auto child : item->children) {
        if (!child->visible) {
            continue;
        }
        if (child->stack) {
            stackedItems.push_back(child);
            continue;
        }
        visibleItems++;
        int ch = child->height;
        // if (child->fit_children_y) {
        //     _layout_run(child, { 0, 0, 0, 0 });
        //     layout_compute_fit(child);
        //     ch = child->_rect.h;
        // }
        if (ch != 0) {
            fixedItems.push_back(child);
            totalFixed += ch;
            int cc = child->width;
            if (cc > 0 && cc < constraint.min_width) {
                cc = constraint.min_width;
            }
            if (cc == 0 || cc > constraint.max_width) {
                cc = constraint.max_width;
            }
            _layout_run(child, { 0, 0, cc, ch });
        } else {
            flexItems.push_back(child);
            totalFlex += child->grow;
            totalFlexBasis += child->flex_basis;
        }
    }

    int spaceRemaining = constraint.max_height - totalFixed;
    if (spaceRemaining < 0)
        spaceRemaining = 0;

    // first pass : constraints
    for (auto child : flexItems) {
        child->_flex = child->grow;
        int hh = spaceRemaining * child->grow / totalFlex;
        if (child->preferred_constraint.max_height && hh > child->preferred_constraint.max_height) {
            child->_height = child->preferred_constraint.max_height;
            child->_flex = 0;
        }
    }

    // second pass : distribute
    for (auto child : flexItems) {
        int hh = spaceRemaining * child->_flex / totalFlex;
        if (child->_flex == 0 && child->_height) {
            hh = child->_height;
        }
        _layout_run(child, { 0, 0, constraint.max_width, hh });
    }

    for (auto child : stackedItems) {
        _layout_run(child, constraint);
        layout_compute_fit(child);
    }

    layout_position_items(item);
    layout_reverse_items(item, constraint.max_height);
}

int layout_compute_hash(layout_item_ptr item)
{
    struct layout_hash_data_t {
        int x;
        int y;
        int scroll_x;
        int scroll_y;
        int width;
        int height;
        int content_hash;
        constraint_t constraint;
    };

    layout_hash_data_t hash_data = {
        item->x,
        item->y,
        item->scroll_x,
        item->scroll_y,
        item->width,
        item->height,
        item->content_hash,
        item->constraint
    };

    int hash = murmur_hash(&hash_data, sizeof(layout_hash_data_t), LAYOUT_HASH_SEED);
    for (auto c : item->children) {
        hash = hash_combine(hash, c->state_hash);
    }

    return hash;
}

void prelayout_run(layout_item_ptr item)
{
    for (auto child : item->children) {
        prelayout_run(child);
    }

    if (item->prelayout) {
        item->prelayout(item.get());
    }
}

void postlayout_run(layout_item_ptr item)
{
    for (auto child : item->children) {
        postlayout_run(child);
    }

    if (item->postlayout) {
        item->postlayout(item.get());
    }

    item->state_hash = layout_compute_hash(item);
}

void _layout_run(layout_item_ptr item, constraint_t constraint)
{
    item->constraint = constraint;

    if (layout_compute_hash(item) == item->state_hash) {
        return;
    }

    items_visited++;

    // margin
    if (item->margin) {
        item->margin_left = item->margin_left ? item->margin_left : item->margin;
        item->margin_right = item->margin_right ? item->margin_right : item->margin;
        item->margin_top = item->margin_top ? item->margin_top : item->margin;
        item->margin_bottom = item->margin_bottom ? item->margin_bottom : item->margin;
    }

    if (item->stack) {
        layout_stack_run(item, constraint);
    } else if (item->is_row()) {
        layout_horizontal_run(item, constraint);
    } else {
        layout_vertical_run(item, constraint);
    }

    // apply margin
    for (auto child : item->children) {
        if (!child->visible) {
            continue;
        }
        if (child->stack) {
            continue;
        }
        child->rect.x += item->margin_left;
        child->rect.y += item->margin_top;
    }
}

void _layout_compute_absolute_position(layout_item_ptr item)
{
    if (!item->visible)
        return;

    items_computed++;
    
    for (auto child : item->children) {
        child->render_rect = child->rect;
        child->render_rect.x += item->render_rect.x + item->scroll_x;
        child->render_rect.y += item->render_rect.y + item->scroll_y;
        _layout_compute_absolute_position(child);
    }
}

void layout_compute_absolute_position(layout_item_ptr item)
{
    items_computed = 0;
    _layout_compute_absolute_position(item);
    _LOG("%s %d\n", item->name.c_str(), items_computed);
}

void layout_clear_hash(layout_item_ptr item, int depth)
{
    if (depth <= 0)
        return;
    item->state_hash = 0;
    for (auto c : item->children) {
        layout_clear_hash(c, depth - 1);
    }
}

void layout_run(layout_item_ptr item, constraint_t constraint, bool recompute)
{
    prelayout_run(item);

    items_visited = 0;

    rect_t r = item->rect;
    rect_t rr = item->render_rect;
    if (recompute) {
        constraint = { 0, 0, r.w, r.h };
    }

    _layout_run(item, constraint);

    item->render_rect = item->rect; // root

    if (recompute) {
        item->rect = r;
        item->render_rect = rr;
    }

    postlayout_run(item);
    layout_compute_absolute_position(item);

    _LOG("%s %d\n", item->name.c_str(), items_visited);
}

static bool compare_item_order(layout_item_ptr f1, layout_item_ptr f2)
{
    return f1->order < f2->order;
}

void layout_sort(layout_item_ptr item)
{
    sort(item->children.begin(), item->children.end(), compare_item_order);
}

bool should_run = false;
void layout_request()
{
    should_run = true;
}

bool layout_should_run()
{
    bool res = should_run;
    should_run = false;
    return res;
}

#undef _LOG