#include "explorer_view.h"
#include "render_cache.h"
#include "renderer.h"

#include "app_view.h"
#include "image.h"
#include "scrollarea.h"
#include "scrollbar.h"
#include "text.h"

#include "app.h"
#include "explorer.h"
#include "extension.h"

explorer_view::explorer_view()
    : list_view()
{
    type = "explorer";
}

void explorer_view::update()
{
    explorer_t* explorer = explorer_t::instance();
    bool hasChanges = explorer->regenerateList;
    explorer->update(0); // did change?

    hasChanges = hasChanges || explorer->renderList.size() != data.size();

    if (!hasChanges) {
        list_view::update();
        return;
    }

    printf("repopulate explorer\n");

    std::string folder_icon_path = icon_for_file(app_t::instance()->icons, ".folder-open", app_t::instance()->extensions);
    std::string folder_close_icon_path = icon_for_file(app_t::instance()->icons, ".folder", app_t::instance()->extensions);

    data.clear();
    for (auto f : explorer->renderList) {
        std::string icon;
        if (f->isDirectory) {
            if (f->expanded) {
                icon = folder_icon_path;
            } else {
                icon = folder_close_icon_path;
            }
        } else {
            icon = icon_for_file(app_t::instance()->icons, f->name, app_t::instance()->extensions);
        }
        list_item_data_t item = {
            icon : icon,
            text : f->name,
            indent : f->depth,
            data : f
        };
        data.push_back(item);
    }

    list_view::update();
}

void explorer_view::select_item(list_item_view* item)
{
    fileitem_t* file = (fileitem_t*)item->data.data;

    app_t* app = app_t::instance();
    if (file && file->isDirectory) {
        if (file->canLoadMore) {
            file->load();
            file->canLoadMore = false;
        }
        file->expanded = !file->expanded;
        explorer_t::instance()->regenerateList = true;
        layout_request();
    } else {
        bool multi = (view_input_key_mods() & K_MOD_CTRL) == K_MOD_CTRL;
        ((app_view*)(app->view))->show_editor(app->openEditor(file->fullPath), !multi);
    }
}