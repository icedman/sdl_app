#include "app_view.h"
#include "renderer.h"

#include "app.h"

#include "editor_view.h"
#include "explorer_view.h"
#include "tabbar.h"

app_view::app_view()
    : view_item("app")
{
    layout_item_ptr lo = layout();
    lo->margin = 0;
    lo->direction = LAYOUT_FLEX_DIRECTION_ROW;

    explorer = std::make_shared<explorer_view>();  

    main = std::make_shared<vertical_container>();    
    tabbar = std::make_shared<tabbar_view>();

    view_item_ptr tabcontent = std::make_shared<horizontal_container>();

    main->add_child(tabbar);
    main->add_child(tabcontent);
    
    view_item_ptr editor = std::make_shared<editor_view>();  
    ((editor_view*)editor.get())->editor = app_t::instance()->currentEditor;
    tabcontent->add_child(editor);

    add_child(explorer);
    add_child(main);
}
