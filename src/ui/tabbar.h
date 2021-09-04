#ifndef TABBAR_VIEW_H
#define TABBAR_VIEW_H

#include "view.h"
#include "text.h"

struct tabbar_view : horizontal_container {
    tabbar_view();

    void prelayout() override;
    
    view_item_ptr spacer;
};

#endif // TABBAR_VIEW_H