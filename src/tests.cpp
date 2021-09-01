#include "tests.h"

#include "text.h"
#include "button.h"
#include "explorer.h"
#include "scrollbar.h"

#include "render_cache.h"

view_item_ptr test5() {
    view_item_ptr root = std::make_shared<view_item>();
    root->layout()->margin = 0;
    root->layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;

    // view_item_ptr scrollarea = std::make_shared<editor_view>();
    // view_item_ptr content = ((editor_view*)scrollarea.get())->content;
    // root->add_child(scrollarea);

    view_item_ptr explorer = std::make_shared<explorer_view>();  
    root->add_child(explorer);

    view_item_ptr editor = std::make_shared<editor_view>();  
    editor->can_press = true;  
    editor->can_hover = true;
    root->add_child(editor);
    return root;
}

struct my_root : view_item {
    bool on_scroll() override {
        int v = ((scrollbar_view*)v_scroll.get())->index;
        int h = ((scrollbar_view*)h_scroll.get())->index;
        // printf(">%d %d\n", v,h);

        layout_item_ptr lo = scrollarea->layout();
        lo->scroll_x = -h * 20;
        lo->scroll_y = -v * 20;

        rencache_invalidate();
        return true;
    }

    void update() override {
        ((scrollbar_view*)v_scroll.get())->set_size(100, 10);
        ((scrollbar_view*)h_scroll.get())->set_size(100, 10);
    }

    view_item_ptr v_scroll;
    view_item_ptr h_scroll;
    view_item_ptr scrollarea;
};

view_item_ptr test4() {
    view_item_ptr root = std::make_shared<my_root>();
    layout_item_ptr layout = root->layout();
    layout->margin = 40;
    layout->direction = LAYOUT_FLEX_DIRECTION_COLUMN;

    view_item_ptr h_layout = std::make_shared<view_item>();
    h_layout->layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;

    view_item_ptr v_scroll = std::make_shared<vscrollbar_view>();
    v_scroll->layout()->width = 18;
    view_item_ptr h_scroll = std::make_shared<hscrollbar_view>();
    h_scroll->layout()->height = 18;

    view_item_ptr scrollarea = std::make_shared<scrollarea_view>();
    view_item_ptr content = ((scrollarea_view*)scrollarea.get())->content;
    h_layout->add_child(scrollarea);
    h_layout->add_child(v_scroll);

    my_root *_root = ((my_root*)root.get());
    _root->scrollarea = scrollarea;
    _root->v_scroll = v_scroll;
    _root->h_scroll = h_scroll;

    for(int i=0; i<10; i++) {
        std::string t = "button ";
        t += ('a' + i);
        view_item_ptr button = std::make_shared<button_view>(t);
        button->can_scroll = true;
        content->add_child(button);
    }

    root->add_child(h_layout);

    view_item_ptr hv = std::make_shared<view_item>();
    hv->layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
    hv->layout()->height = h_scroll->layout()->height;
    hv->add_child(h_scroll);
    view_item_ptr hvs = std::make_shared<view_item>();
    hvs->layout()->width = 18;
    hvs->layout()->height = 18;
    hv->add_child(hvs);
    
    root->add_child(hv);
    return root;
}

view_item_ptr test3() {
    view_item_ptr root = std::make_shared<view_item>();

    view_item_ptr item_a = std::make_shared<view_item>();
    view_item_ptr item_b = std::make_shared<view_item>();
    view_item_ptr item_c = std::make_shared<view_item>();
    view_item_ptr item_d = std::make_shared<view_item>();
    view_item_ptr item_e = std::make_shared<view_item>();
    // view_item_ptr item_g = std::make_shared<view_item>();
    // view_item_ptr item_h = std::make_shared<view_item>();

    item_a->add_child(item_b);
    item_a->add_child(item_c);

    item_c->layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
    item_c->add_child(item_d);
    item_c->add_child(item_e);
    item_e->layout()->grow = 4;

    view_item_ptr button = std::make_shared<button_view>("hey button");
    button->add_child(std::make_shared<text_view>("hello world"));
    
    // view_item_ptr button = std::make_shared<view_item>();

    button->layout()->width = 200;
    button->layout()->height = 40;
    button->layout()->margin = 4;
    button->layout()->wrap = true;
    button->layout()->fit_children = false;
    button->can_press = true;
    button->can_hover = true;
    item_b->add_child(button);
    item_b->layout()->margin = 20;
    // item_b->layout()->scroll_x = -80;
    // item_b->layout()->scroll_y = 80;


    view_item_ptr item_f = std::make_shared<text_view>("hello world");
    item_b->add_child(item_f);

    root->add_child(item_a);
    return root;
}

view_item_ptr test2() {
    layout_item_ptr root = std::make_shared<layout_item>();
    // root->direction = LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE;
    root->direction = LAYOUT_FLEX_DIRECTION_ROW;
    
    // root->justify = LAYOUT_JUSTIFY_FLEX_END;
    root->justify = LAYOUT_JUSTIFY_CENTER;
    // root->justify = LAYOUT_JUSTIFY_SPACE_BETWEEN;
    // root->justify = LAYOUT_JUSTIFY_SPACE_AROUND;
    
    root->align = LAYOUT_ALIGN_FLEX_START;
    // root->align = LAYOUT_ALIGN_FLEX_END;
    // root->align = LAYOUT_ALIGN_CENTER;
    // root->align = LAYOUT_ALIGN_STRETCH;

    root->wrap = true;

    layout_item_ptr item_a = std::make_shared<layout_item>();
    layout_item_ptr item_b = std::make_shared<layout_item>();
    layout_item_ptr item_c = std::make_shared<layout_item>();
    layout_item_ptr item_d = std::make_shared<layout_item>();
    layout_item_ptr item_e = std::make_shared<layout_item>();
    layout_item_ptr item_f = std::make_shared<layout_item>();
    layout_item_ptr item_g = std::make_shared<layout_item>();
    layout_item_ptr item_h = std::make_shared<layout_item>();

    root->children.push_back(item_a);
    root->children.push_back(item_b);
    root->children.push_back(item_c);
    root->children.push_back(item_d);
    root->children.push_back(item_e);
    root->children.push_back(item_f);
    root->children.push_back(item_g);
    root->children.push_back(item_h);

    int i = 0;
    for(auto child : root->children) {
        child->name = 'a' + i++;
        child->width = 200;
        child->height = 200;
    }

    view_item_ptr view = std::make_shared<view_item>();
    view->set_layout(root);
    return view;
}

view_item_ptr test1() {
    layout_item_ptr root = std::make_shared<layout_item>();
    root->direction = LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE;
    root->margin = 20;

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

    item_a->name = "a";
    item_b->name = "b";
    item_c->name = "c";
    item_d->name = "d";
    item_e->name = "e";
    item_f->name = "f";
    item_g->name = "g";
    item_h->name = "h";

    item_a->margin = 20;
    item_b->margin = 20;
    item_c->margin = 20;
    item_d->margin = 20;
    item_e->margin = 20;
    item_f->margin = 20;
    item_g->margin = 20;
    item_h->margin = 20;

    item_a->children.push_back(std::make_shared<layout_item>());
    item_b->children.push_back(std::make_shared<layout_item>());
    item_c->children.push_back(std::make_shared<layout_item>());
    item_d->children.push_back(std::make_shared<layout_item>());
    item_e->children.push_back(std::make_shared<layout_item>());
    item_f->children.push_back(std::make_shared<layout_item>());
    item_g->children.push_back(std::make_shared<layout_item>());
    item_h->children.push_back(std::make_shared<layout_item>());

    item_a->grow = 1;
    item_b->grow = 3;
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
    item_e->height = 80;
    item_e->width = 200;
    item_f->height = 80;
    item_f->width = 200;

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

    item_g->width = 80;
    item_h->width = 80;
    item_h->height = 200;

    view_item_ptr view = std::make_shared<view_item>();
    view->set_layout(root);
    return view;
}
