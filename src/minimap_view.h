#ifndef MINIMAP_VIEW_H
#define MINIMAP_VIEW_H

#include "view.h"
#include "scrollbar.h"

struct minimap_view : view_item {
    minimap_view();

    void postlayout() override;
    void render() override;
};

#endif // MINIMAP_VIEW_H