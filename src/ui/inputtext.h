#ifndef INPUTTEXT_VIEW_H
#define INPUTTEXT_VIEW_H

#include "editor_view.h"
#include "text.h"
#include "view.h"

struct inputtext_view : horizontal_container {
    inputtext_view();

    void render() override;

    void set_editor(view_item_ptr editor);
    view_item_ptr editor;
};

#endif // INPUTTEXT_VIEW_H