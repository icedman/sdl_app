#ifndef STATUSBAR_VIEW_H
#define STATUSBAR_VIEW_H

#include "view.h"
#include "text.h"

struct statusbar_view : horizontal_container {
    statusbar_view();
    
    void update() override;

    view_item_ptr status;
};

#endif // STATUSBAR_VIEW_H