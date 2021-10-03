#ifndef INPUTTEXT_VIEW_H
#define INPUTTEXT_VIEW_H

#include "editor_view.h"
#include "text.h"
#include "view.h"

struct inputtext_view : horizontal_container {
    inputtext_view(std::string text = "");

    DECLAR_VIEW_TYPE(INPUTTEXT, horizontal_container)

    void render() override;

    std::string value();
    void set_value(std::string text);

    void set_editor(view_item_ptr editor);
    view_item_ptr editor;
};

#endif // INPUTTEXT_VIEW_H