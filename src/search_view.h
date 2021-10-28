#ifndef SEARCH_VIEW_H
#define SEARCH_VIEW_H

#include "editor.h"
#include "popup.h"
#include "prompt_view.h"

struct editor_view_t;
struct search_view_t : prompt_view_t {
    search_view_t(editor_view_t* e);

    DECLAR_VIEW_TYPE(CUSTOM, popup_t)
    std::string type_name() override { return "search_view"; }

    bool update_data(std::string text);
    bool commit();

    bool _findNext;
    int _searchDirection;
};

#endif // SEARCH_VIEW_H