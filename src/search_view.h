#ifndef SEARCH_VIEW_H
#define SEARCH_VIEW_H

#include "list.h"
#include "popup.h"
#include "view.h"

struct search_view : popup_view {
    search_view();

    bool input_text(std::string text) override;
    void show_search();

    view_item_ptr input;
    view_item_ptr list;
};

#endif // SEARCH_VIEW_H