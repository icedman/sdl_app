#include "tabbar.h"
#include "renderer.h"
#include "button.h"

tabbar_view::tabbar_view()
    : horizontal_container()
{
    interactive = true;

    layout()->margin = 8;
    layout()->height = 40;
    layout()->direction = LAYOUT_FLEX_DIRECTION_ROW;
    layout()->justify = LAYOUT_JUSTIFY_FLEX_START;
    layout()->align = LAYOUT_ALIGN_FLEX_END;

    for(int i=0;i<6;i++) {
        std::string name = "tab ";
        name += 'a' + i;

        view_item_ptr tab = std::make_shared<button_view>(name);
        tab->layout()->height = 32;
        add_child(tab);
    }

    spacer = std::make_shared<view_item>("spacer");
    add_child(spacer);
}

void tabbar_view::prelayout()
{
    spacer->layout()->grow = 2;
}