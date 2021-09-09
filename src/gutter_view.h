#ifndef GUTTER_VIEW_H
#define GUTTER_VIEW_H

#include "editor.h"
#include "view.h"

struct gutter_view : view_item {
    gutter_view();

    void render() override;

    editor_ptr editor;
    int fg_index;
};

#endif // GUTTER_VIEW_H