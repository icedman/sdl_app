#ifndef GUTTER_VIEW_H
#define GUTTER_VIEW_H

#include "view.h"
#include "editor.h"

struct gutter_view : view_item {
    gutter_view();

    void prelayout() override;
    void render() override;

    editor_ptr editor;
    int fg_index;
};

#endif // GUTTER_VIEW_H