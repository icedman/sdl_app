#ifndef TABBAR_VIEW_H
#define TABBAR_VIEW_H

#include "scrollarea.h"
#include "text.h"
#include "view.h"

struct tabbar_view : horizontal_container {
    tabbar_view();

    void prelayout() override;
    void update() override;
    void render() override;

    view_item_ptr scrollarea;
    view_item_ptr content();
};

#endif // TABBAR_VIEW_H