#ifndef APP_VIEW_H
#define APP_VIEW_H

#include "view.h"

struct app_view : view_item {
    app_view();

    view_item_ptr tabbar;
    view_item_ptr explorer;
    view_item_ptr main;
};

#endif // APP_VIEW_H