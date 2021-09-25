#ifndef INPUTTEXT_VIEW_H
#define INPUTTEXT_VIEW_H

#include "editor_view.h"
#include "text.h"
#include "view.h"

struct inputtext_view : horizontal_container {
    inputtext_view();

    view_item_ptr editor;
    void render() override;
};

#endif // INPUTTEXT_VIEW_H