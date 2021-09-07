#ifndef STATUSBAR_VIEW_H
#define STATUSBAR_VIEW_H

#include "text.h"
#include "view.h"

struct statusbar_view : horizontal_container {
    statusbar_view();

    void update() override;

    view_item_ptr status;
    view_item_ptr items;
};

#endif // STATUSBAR_VIEW_H