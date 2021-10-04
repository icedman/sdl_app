#ifndef STATUSBAR_VIEW_H
#define STATUSBAR_VIEW_H

#include "text.h"
#include "view.h"

struct statusbar_view : horizontal_container {
    statusbar_view();

    DECLAR_VIEW_TYPE(CUSTOM, horizontal_container)

    void update(int millis) override;
    void prerender() override;
    void render() override;

    view_item_ptr status;
    view_item_ptr items;

    std::string current_status;
    std::string previous_status;
};

#endif // STATUSBAR_VIEW_H