#ifndef COMPLETER_VIEW_H
#define COMPLETER_VIEW_H

#include "list.h"
#include "popup.h"
#include "view.h"

struct completer_view : popup_view {
    completer_view();

    view_item_ptr list;
};

#endif // COMPLETER_VIEW_H