#ifndef APP_TABBAR_VIEW_H
#define APP_TABBAR_VIEW_H

#include "tabbar.h"

struct app_tabbar_view : tabbar_view {

    app_tabbar_view();
    void update() override;
    void select_item(list_item_view* item) override;
};

#endif // APP_TABBAR_VIEW_H