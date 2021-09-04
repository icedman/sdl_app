#ifndef IMAGE_VIEW
#define IMAGE_VIEW

#include "view.h"
#include "text.h"
#include "renderer.h"

struct icon_view : view_item {
    icon_view();

    void render() override;
    
    RenImage *icon;
};

#endif // IMAGE_VIEW
