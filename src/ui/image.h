#ifndef IMAGE_VIEW_H
#define IMAGE_VIEW_H

#include "renderer.h"
#include "text.h"
#include "view.h"

struct icon_view : view_item {
    icon_view();

    DECLAR_VIEW_TYPE(IMAGE, view_item)

    void render() override;

    RenImage* icon;
};

#endif // IMAGE_VIEW_H
