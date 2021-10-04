#ifndef GUTTER_VIEW_H
#define GUTTER_VIEW_H

#include "editor.h"
#include "view.h"

struct gutter_view : view_item {
    gutter_view();

    DECLAR_VIEW_TYPE(CUSTOM, view_item)

    void prerender() override;
    void render() override;

    int first;
    int last;
    int prev_first;
    int prev_last;
};

#endif // GUTTER_VIEW_H