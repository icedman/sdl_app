#include "tests.h"

layout_item_ptr sample_layout() {
    layout_item_ptr root = std::make_shared<layout_item>();
    layout_item_ptr item_a = std::make_shared<layout_item>();
    layout_item_ptr item_b = std::make_shared<layout_item>();
    layout_item_ptr item_c = std::make_shared<layout_item>();
    layout_item_ptr item_d = std::make_shared<layout_item>();
    layout_item_ptr item_e = std::make_shared<layout_item>();
    layout_item_ptr item_f = std::make_shared<layout_item>();
    layout_item_ptr item_g = std::make_shared<layout_item>();
    layout_item_ptr item_h = std::make_shared<layout_item>();

    item_a->rgb = { 255, 0, 255 };
    item_b->rgb = { 255, 255, 0 };
    item_c->rgb = { 0, 255, 0 };
    item_d->rgb = { 0, 255, 255 };
    item_e->rgb = { 0, 0, 255 };
    item_f->rgb = { 155, 0, 155 };
    item_g->rgb = { 155, 155, 0 };
    item_h->rgb = { 0, 155, 0 };

    layout_item_list renderList;

    root->direction = LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE;
    root->x = 10;
    root->y = 10;

    item_a->name = "a";
    item_b->name = "b";
    item_c->name = "c";
    item_d->name = "d";
    item_e->name = "e";
    item_f->name = "f";
    item_g->name = "g";
    item_h->name = "h";

    item_a->flex = 1;
    item_b->flex = 3;
    // item_a->visible = false;

    root->children.push_back(item_a);
    root->children.push_back(item_b);
    item_b->direction = LAYOUT_FLEX_DIRECTION_ROW;
    item_b->children.push_back(item_c);
    item_b->children.push_back(item_d);
    item_c->width = 400;
    // item_c->flex_basis = 400;

    item_d->direction = LAYOUT_FLEX_DIRECTION_COLUMN;
    // item_d->justify = LAYOUT_JUSTIFY_FLEX_END;
    // item_d->justify = LAYOUT_JUSTIFY_CENTER;
    // item_d->justify = LAYOUT_JUSTIFY_SPACE_BETWEEN;
    item_d->justify = LAYOUT_JUSTIFY_SPACE_AROUND;
    // item_d->align = LAYOUT_ALIGN_FLEX_END;
    item_d->align = LAYOUT_ALIGN_CENTER;
    // item_d->align = LAYOUT_ALIGN_STRETCH;

    item_d->children.push_back(item_e);
    item_d->children.push_back(item_f);

    item_e->align_self = LAYOUT_ALIGN_FLEX_END;
    // item_e->height = 40;
    // item_e->width = 200;
    // item_f->height = 40;
    // item_f->width = 200;

    // item_c->direction = LAYOUT_FLEX_DIRECTION_ROW;
    item_c->direction = LAYOUT_FLEX_DIRECTION_ROW_REVERSE;
    // item_c->justify = LAYOUT_JUSTIFY_FLEX_END;
    // item_c->justify = LAYOUT_JUSTIFY_CENTER;
    // item_c->justify = LAYOUT_JUSTIFY_SPACE_BETWEEN;
    item_c->justify = LAYOUT_JUSTIFY_SPACE_AROUND;
    item_c->align = LAYOUT_ALIGN_FLEX_END;
    // item_c->align = LAYOUT_ALIGN_CENTER;
    // item_c->align = LAYOUT_ALIGN_STRETCH;
    item_c->children.push_back(item_g);
    item_c->children.push_back(item_h);

    // item_g->width = 40;
    // item_h->width = 40;
    // item_h->height = 200;
    return root;
}
