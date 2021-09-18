#include "tabbar_view.h"
#include "app_view.h"
#include "app.h"

app_tabbar_view::app_tabbar_view()
    : tabbar_view()
{
    autoscroll = true;
}

void app_tabbar_view::update() {

    app_t* app = app_t::instance();
    ((list_view*)this)->value = app_t::instance()->currentEditor->document.fullPath;
    
    bool hasChanges = false;

    hasChanges = hasChanges || app->editors.size() != data.size();

    if (!hasChanges) {
        list_view::update();
        return;
    }

    if (!layout()->visible || !app_t::instance()->currentEditor) return;

    // printf("repopulate explorer\n");

    std::string folder_icon_path = icon_for_file(app_t::instance()->icons, ".folder-open", app_t::instance()->extensions);
    std::string folder_close_icon_path = icon_for_file(app_t::instance()->icons, ".folder", app_t::instance()->extensions);

    data.clear();
    for (auto f : app->editors) {
        list_item_data_t item = {
            icon : "",
            text : f->document.fileName,
            indent : 0,
            data : f.get(),
            value : f->document.fullPath
        };
        data.push_back(item);
    }

    list_view::update();

    for (auto btn : content()->_views) {
        btn->layout()->preferred_constraint = {
            0,0,
            200,0
        };
        btn->layout()->justify = LAYOUT_JUSTIFY_CENTER;
        btn->layout()->align = LAYOUT_ALIGN_CENTER;
    }
}

void app_tabbar_view::select_item(list_item_view* item)
{
    list_view::select_item(item);
    app_t* app = app_t::instance();
    bool multi = (Renderer::instance()->key_mods() & K_MOD_CTRL) == K_MOD_CTRL;
    ((app_view*)(app->view))->show_editor(app->openEditor(item->data.value), !multi);
}
