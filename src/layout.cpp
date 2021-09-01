#include "layout.h"
#include <stdio.h>

static bool run_requested = false;

void layout_render_list(layout_item_list& list, layout_item_ptr item) {
    if (!item->visible) {
        return;
    }
    list.push_back(item);
    for(auto child : item->children) {
        child->render_rect = child->rect;
        child->render_rect.x += item->render_rect.x + item->scroll_x;
        child->render_rect.y += item->render_rect.y + item->scroll_y;
        layout_render_list(list, child);
        // if offscreen or off area .. discard
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

void layout_position_items(layout_item_ptr item) {
    int constraint = item->constraint.max_width;

    int wd = 0;
    int hd = 0;
    if (item->is_row()) {
        wd = 1;
    } else {
        hd = 1;
        constraint = item->constraint.max_height;
    }

    // wrap
    std::vector<layout_item_list> groups;
    groups.push_back(layout_item_list());
    int consumed = 0;
    for(auto child : item->children) {
        if (!child->visible) {
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
    for(auto group : groups) {
        int spaceRemaining = constraint;
        for(auto child : group) {
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
        switch (justify)  {
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

        int ww = 0;
        int hh = 0;
        for(auto child : group) {
            if (hh == 0 || hh < child->rect.h) {
                hh = child->rect.h * wd;
            }
            if (ww == 0 || ww < child->rect.w) {
                ww = child->rect.w * hd;
            }

            child->rect.x = (xx * hd) + ((offsetStart + offset) * wd);
            child->rect.y = (yy * wd) + ((offsetStart + offset) * hd);

            if (item->rect.w < child->rect.x + child->rect.w + item->margin*2) {
                if (item->fit_children) {
                    item->rect.w = child->rect.x + child->rect.w + item->margin*2;
                }
            }
            if (item->rect.h < child->rect.y + child->rect.h + item->margin*2) {
                if (item->fit_children) {
                    item->rect.h = child->rect.y + child->rect.h + item->margin*2;
                }
            }

            offset += child->rect.w * wd;
            offset += child->rect.h * hd;
            offset += offsetInc;
        }

        xx += ww;
        yy += hh;
    }

    // align items
    int alignOffset = (item->constraint.max_width - xx) * hd;
    alignOffset += (item->constraint.max_height - yy) * wd;

    switch(item->align) {
    case LAYOUT_ALIGN_FLEX_START:
        alignOffset = 0;
        break;
    case LAYOUT_ALIGN_CENTER:
        alignOffset *= 0.5f;
        break;
    }

    if (alignOffset > 0) {
        for(auto child : item->children) {
            if (!child->visible) {
                continue;
            }
            child->rect.x += alignOffset * hd;
            child->rect.y += alignOffset * wd;
        }
    }
}

void layout_horizontal_run(layout_item_ptr item, layout_constraint constraint)
{
    layout_rect rect = {
        x: item->x, 
        y: item->y,
        w: constraint.max_width,
        h: constraint.max_height
    };
    item->rect = rect;
    item->render_rect = rect;

    constraint.max_width -= item->margin * 2;
    constraint.max_height -= item->margin * 2;
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
            totalFlex += child->grow;
            totalFlexBasis += child->flex_basis;
        }
    }

    int spaceRemaining = constraint.max_width - totalFixed;
    if (spaceRemaining < 0)
        spaceRemaining = 0;

    for(auto child : flexItems) {
        int ww = spaceRemaining * child->grow / totalFlex;
        layout_run(child, { 0, 0, ww, constraint.max_height });
    }

    layout_position_items(item);
    layout_reverse_items(item, constraint.max_width);
}

void layout_vertical_run(layout_item_ptr item, layout_constraint constraint)
{
    if (constraint.max_width == item->constraint.max_width &&
            constraint.max_height == item->constraint.max_height) {
        return;
    }

    layout_rect rect = {
        x: item->x, 
        y: item->y,
        w: constraint.max_width,
        h: constraint.max_height
    };
    item->rect = rect;
    item->render_rect = rect;

    constraint.max_width -= item->margin * 2;
    constraint.max_height -= item->margin * 2;
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
            totalFlex += child->grow;
            totalFlexBasis += child->flex_basis;
        }
    }

    int spaceRemaining = constraint.max_height - totalFixed;
    if (spaceRemaining < 0)
        spaceRemaining = 0;

    for(auto child : flexItems) {
        int hh = spaceRemaining * child->grow / totalFlex;
        layout_run(child, { 0, 0, constraint.max_width, hh});
    }

    layout_position_items(item);
    layout_reverse_items(item, constraint.max_height);
}

void _prelayout(layout_item_ptr item)
{
    if (item->view) {
        item->view->prelayout();
    }
    for(auto child : item->children) {
        _prelayout(child);
    }
}

void _layout_run(layout_item_ptr item, layout_constraint constraint)
{
    _prelayout(item);

    if (item->direction == LAYOUT_FLEX_DIRECTION_ROW ||
            item->direction == LAYOUT_FLEX_DIRECTION_ROW_REVERSE) {
        layout_horizontal_run(item, constraint);
    } else {
        layout_vertical_run(item, constraint);
    }

    // apply margin
    for(auto child : item->children) {
        if (!child->visible) {
            continue;
        }
        child->rect.x += item->margin;
        child->rect.y += item->margin;
    }
}

void layout_run(layout_item_ptr item, layout_constraint constraint)
{
    _layout_run(item, constraint);
    item->render_rect = item->rect; // root
}

void layout_request()
{
    run_requested = true;
}

bool layout_should_run()
{
    bool res = run_requested;
    run_requested = false;
    return res;
}