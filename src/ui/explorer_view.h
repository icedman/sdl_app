#ifndef EXPLORER_VIEW_H
#define EXPLORER_VIEW_H

#include "view.h"
#include "text.h"

struct explorer_view : view_item {
    explorer_view();
    
    view_item_ptr vscroll;
    view_item_ptr hscroll;
};

#endif // EXPLORER_VIEW_H