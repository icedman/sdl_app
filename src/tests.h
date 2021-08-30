#ifndef TESTS_H
#define TESTS_H

#include "view.h"
#include "layout.h"
#include "scrollarea.h"

struct editor_view : view_item {
    editor_view() 
        : view_item("editor")
        , start(0)
        , wait(0)
    {}

    ~editor_view() {}

    bool mouse_wheel(int x, int y) override;

    int start;
    int wait;
};

view_item_ptr test1();
view_item_ptr test2();
view_item_ptr test3();
view_item_ptr test4();
view_item_ptr test5();

#endif // TESTS_H