#ifndef EXPLORER_VIEW_H
#define EXPLORER_VIEW_H

#include "list.h"
#include "panel.h"
#include "view.h"

struct fileitem_t;
struct explorer_view : list_view {
    explorer_view();

    void update() override;
    void select_item(list_item_view* item) override;
};

#endif // EXPLORER_VIEW_H