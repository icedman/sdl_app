#ifndef STATUSBAR_VIEW_H
#define STATUSBAR_VIEW_H

#include "text.h"
#include "view.h"

struct statusbar_view : horizontal_container {
    statusbar_view();

    DECLAR_VIEW_TYPE(CUSTOM, horizontal_container)
    std::string type_name() override { return "statusbar"; }

    void update(int millis) override;
    void render() override;

    view_item_ptr status;
    view_item_ptr items;
};

#endif // STATUSBAR_VIEW_H