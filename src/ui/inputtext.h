#ifndef INPUTTEXT_VIEW_H
#define INPUTTEXT_VIEW_H

#include "view.h"
#include "text.h"
#include "editor_view.h"

struct inputtext_view : horizontal_container {
    inputtext_view();
  
    view_item_ptr editor;  
    view_item_ptr placeholder;

    void render() override;
};

#endif // INPUTTEXT_VIEW_H