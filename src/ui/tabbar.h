#ifndef TABBAR_VIEW_H
#define TABBAR_VIEW_H

#include "list.h"

struct tabbar_view : list_view {
    tabbar_view();

    DECLAR_VIEW_TYPE(SCROLLBAR, list_view)
};

#endif // TABBAR_VIEW_H