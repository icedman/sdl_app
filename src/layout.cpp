#include "layout.h"
#include <stdio.h>

void layout_render_list(layout_item_list& list, layout_item_ptr item) {
    if (!item->visible) {
        return;
    }
    list.push_back(item);
    for(auto child : item->children) {
        child->render_rect = child->rect;
        child->render_rect.x += item->render_rect.x;
        child->render_rect.y += item->render_rect.y;
        layout_render_list(list, child);
    }
}

void layout_reverse_items(layout_item_ptr item, int constraint)
{
    if (item->direction == LAYOUT_FLEX_DIRECTION_ROW_REVERSE) {
        for(auto child : item->children) {
            if (!child->visible) {
                continue;
            }
            child->rect.x = constraint - (child->rect.x + child->rect.w);
        }
    }
    if (item->direction == LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE) {
        for(auto child : item->children) {
            if (!child->visible) {
                continue;
            }
            child->rect.y = constraint - (child->rect.y + child->rect.h);
        }
    }
}

int layout_position_items(layout_item_ptr item, int constraint)
{
    int xx = 0;
    int yy = 0;
    int wd = 1;
    int hd = 1;
    if (item->direction == LAYOUT_FLEX_DIRECTION_ROW ||
            item->direction == LAYOUT_FLEX_DIRECTION_ROW_REVERSE) {
        hd = 0;
    } else {
        wd = 0;
    }
    int spaceRemaining = constraint;
    for(auto child : item->children) {
        if (!child->visible) {
            continue;
        }
        child->rect.x = xx;
        child->rect.y = yy;
        xx += child->rect.w * wd;
        yy += child->rect.h * hd;
        spaceRemaining -= (child->rect.w * wd);
        spaceRemaining -= (child->rect.h * hd);
    }
    return spaceRemaining;
}

void layout_align_items(layout_item_ptr item, int constraint)
{
    int hd = 1;
    int wd = 1;
    if (item->direction == LAYOUT_FLEX_DIRECTION_ROW ||
            item->direction == LAYOUT_FLEX_DIRECTION_ROW_REVERSE) {
        wd = 0;
    } else {
        hd = 0;
    }

    for(auto child : item->children) {
        if (!child->visible) {
            continue;
        }
        float offset = 0;
        switch(child->align_self ? child->align_self : item->align) {
        case LAYOUT_ALIGN_FLEX_END:
            offset = 1;
            break;
        case LAYOUT_ALIGN_CENTER:
            offset = 0.5f;
            break;
        }
        if (hd && child->rect.h < constraint) {
            if (item->align == LAYOUT_ALIGN_STRETCH) {
                child->rect.h = constraint;
                continue;
            }
            child->rect.y += (constraint - child->rect.h) * offset;
        }
        if (wd && child->rect.w < constraint) {
            if (item->align == LAYOUT_ALIGN_STRETCH) {
                child->rect.w = constraint;
                continue;
            }
            child->rect.x += (constraint - child->rect.w) * offset;
        }
    }
}

void layout_justify_items(layout_item_ptr item, int spaceRemaining, int visibleItems)
{
    if (spaceRemaining > 0) {
        int offsetStart = 0;
        int offset = 0;
        int offsetInc = 0;
        switch (item->justify)  {
        case LAYOUT_JUSTIFY_FLEX_END:
            offsetStart = spaceRemaining;
            break;
        case LAYOUT_JUSTIFY_CENTER:
            offsetStart = spaceRemaining/2;
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
        for(auto child : item->children) {
            if (!child->visible) {
                continue;
            }
            if (item->direction == LAYOUT_FLEX_DIRECTION_ROW ||
                    item->direction == LAYOUT_FLEX_DIRECTION_ROW_REVERSE) {
                child->rect.x += offsetStart + offset;
            } else {
                child->rect.y += offsetStart + offset;
            }
            offset += offsetInc;
        }
    }
}

void layout_horizontal_run(layout_item_ptr item, layout_constraint constraint)
{
    layout_rect rect = {
x:
        item->x, item->y,
w:
constraint.max_width, h:
        constraint.max_height
    };
    item->rect = rect;
    item->render_rect = rect;
    item->constraint = constraint;

    if (!item->visible) {
        item->rect = { 0,0,0,0 };
        return;
    }

    if (!item->children.size()) {
        return;
    }

    layout_item_list fixedItems;
    layout_item_list flexItems;

    int visibleItems = 0;
    int totalFlex = 0;
    int totalFlexBasis = 0;
    int totalFixed = 0;
    for(auto child : item->children) {
        if (!child->visible) {
            continue;
        }
        visibleItems++;
        if (child->width != 0) {
            fixedItems.push_back(child);
            totalFixed += child->width;
            int cc = child->height;
            if (cc > 0 && cc < constraint.min_height) {
                cc = constraint.min_height;
            }
            if (cc == 0 || cc > constraint.max_height) {
                cc = constraint.max_height;
            }
            layout_run(child, { 0, 0, child->width, cc });
        } else {
            flexItems.push_back(child);
            totalFlex += child->flex;
            totalFlexBasis += child->flex_basis;
        }
    }

    int spaceRemaining = constraint.max_width - totalFixed;
    if (spaceRemaining < 0)
        spaceRemaining = 0;

    // compute flex from basis
    // if (totalFlexBasis > 0 && spaceRemaining > 0) {
    //     for(auto child : flexItems) {
    //         if (child->flex_basis == 0) continue;
    //         float fb = (float)child->flex_basis / spaceRemaining * totalFlex;
    //         printf("%f\n", fb);
    //     }
    // }

    for(auto child : flexItems) {
        int ww = spaceRemaining * child->flex / totalFlex;
        layout_run(child, { 0, 0, ww, constraint.max_height });
    }

    spaceRemaining = layout_position_items(item, constraint.max_width);
    layout_justify_items(item, spaceRemaining, visibleItems);
    layout_align_items(item, constraint.max_height);
    layout_reverse_items(item, constraint.max_width);
}

void layout_vertical_run(layout_item_ptr item, layout_constraint constraint)
{
    if (constraint.max_width == item->constraint.max_width &&
            constraint.max_height == item->constraint.max_height) {
        return;
    }

    layout_rect rect = {
x:
        item->x, item->y,
w:
constraint.max_width, h:
        constraint.max_height
    };
    item->rect = rect;
    item->render_rect = rect;
    item->constraint = constraint;

    if (!item->visible) {
        item->rect = { 0,0,0,0 };
        return;
    }

    if (!item->children.size()) {
        return;
    }

    layout_item_list fixedItems;
    layout_item_list flexItems;

    int visibleItems = 0;
    int totalFlex = 0;
    int totalFlexBasis = 0;
    int totalFixed = 0;
    for(auto child : item->children) {
        if (!child->visible) {
            continue;
        }
        visibleItems++;
        if (child->height != 0) {
            fixedItems.push_back(child);
            totalFixed += child->height;
            int cc = child->width;
            if (cc > 0 && cc < constraint.min_width) {
                cc = constraint.min_width;
            }
            if (cc == 0 || cc > constraint.max_width) {
                cc = constraint.max_width;
            }
            layout_run(child, { 0, 0, cc, child->height });
        } else {
            flexItems.push_back(child);
            totalFlex += child->flex;
            totalFlexBasis += child->flex_basis;
        }
    }

    int spaceRemaining = constraint.max_height - totalFixed;
    if (spaceRemaining < 0)
        spaceRemaining = 0;

    for(auto child : flexItems) {
        int hh = spaceRemaining * child->flex / totalFlex;
        layout_run(child, { 0, 0, constraint.max_width, hh});
    }

    spaceRemaining = layout_position_items(item, constraint.max_height);
    layout_justify_items(item, spaceRemaining, visibleItems);
    layout_align_items(item, constraint.max_width);
    layout_reverse_items(item, constraint.max_height);
}

void _layout_run(layout_item_ptr item, layout_constraint constraint)
{
    if (item->direction == LAYOUT_FLEX_DIRECTION_ROW ||
            item->direction == LAYOUT_FLEX_DIRECTION_ROW_REVERSE) {
        layout_horizontal_run(item, constraint);
    } else {
        layout_vertical_run(item, constraint);
    }
}

void layout_run(layout_item_ptr item, layout_constraint constraint)
{
    _layout_run(item, constraint);
    item->render_rect = item->rect; // root
}

layout_item::layout_item()
    : flex(1)
    , flex_shrink(0)
    , flex_basis(0)
    , x(0)
    , y(0)
    , width(0)
    , height(0)
    , visible(true)
    , align_self(LAYOUT_ALIGN_UNKNOWN)
    , align(LAYOUT_ALIGN_FLEX_START)
    , justify(LAYOUT_JUSTIFY_FLEX_START)
    , direction(LAYOUT_FLEX_DIRECTION_COLUMN)
{
    rgb = {
        r: 255,
        g: 0,
        b: 0
    };
}