#include "explorer_view.h"
#include "renderer.h"
#include "button.h"

explorer_view::explorer_view()
    : view_item("explorer")
{
    layout()->width = 300;
    layout()->direction = LAYOUT_FLEX_DIRECTION_COLUMN;

    for(int i=0;i<20; i++) {
        std::string text = "button ";
        text += 'a' + i;
        view_item_ptr btn = std::make_shared<button_view>(text);
        btn->layout()->height = 22;
        add_child(btn);
    }
}
