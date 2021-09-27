#ifndef SEARCH_VIEW_H
#define SEARCH_VIEW_H

#include "list.h"
#include "popup.h"
#include "view.h"

struct search_view : popup_view {
    search_view();

    void prelayout() override;
    void show_search(std::string value = "");
    bool commit();
    
    void update_list();

    view_item_ptr input;
    view_item_ptr list;

    int searchDirection;
    bool _findNext;
};

#endif // SEARCH_VIEW_H