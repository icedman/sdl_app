#ifndef SEARCH_VIEW_H
#define SEARCH_VIEW_H

#include "list.h"
#include "popup.h"
#include "view.h"

struct search_view : popup_view {
    search_view();

    DECLAR_VIEW_TYPE(CUSTOM, popup_view)
    std::string type_name() override { return "search"; }

    void prelayout() override;
    void show_search(int mode, std::string value = "");
    bool commit();

    void update_list();
    void update_list_indexer();
    void update_list_files();

    view_item_ptr input;
    view_item_ptr list;

    int searchDirection;
    bool _findNext;

    int mode;
};

#endif // SEARCH_VIEW_H