#ifndef EXPLORER_VIEW_H
#define EXPLORER_VIEW_H

#include "list.h"
#include "panel.h"
#include "view.h"

struct fileitem_t;
struct explorer_view : list_view {
    explorer_view();

    DECLAR_VIEW_TYPE(CUSTOM, list_view)

    bool worker(int millis) override;
    void update(int millis) override;
    void select_item(list_item_view* item) override;
};

#endif // EXPLORER_VIEW_H