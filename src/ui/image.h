#ifndef IMAGE_VIEW
#define IMAGE_VIEW

#include "renderer.h"
#include "text.h"
#include "view.h"

struct icon_view : view_item {
    icon_view();

    void render() override;

    RenImage* icon;
};

#endif // IMAGE_VIEW
