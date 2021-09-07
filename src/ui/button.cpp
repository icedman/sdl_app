#include "button.h"
#include "renderer.h"

button_view::button_view(std::string t)
    : view_item("button")
{
    interactive = true;

    text = std::make_shared<text_view>(t);
    add_child(text);

    layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
    layout()->justify = LAYOUT_JUSTIFY_CENTER;
    layout()->align = LAYOUT_ALIGN_CENTER;
}
