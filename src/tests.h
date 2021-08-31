#ifndef TESTS_H
#define TESTS_H

#include "view.h"
#include "layout.h"
#include "scrollarea.h"
#include "scrollbar.h"

struct editor_view : view_item {
    editor_view();

    bool mouse_wheel(int x, int y) override;
    bool on_scroll() override; // scrollbar events
    
    int start;

    view_item_ptr vscroll;
    view_item_ptr hscroll;
};

view_item_ptr test1();
view_item_ptr test2();
view_item_ptr test3();
view_item_ptr test4();
view_item_ptr test5();

#endif // TESTS_H