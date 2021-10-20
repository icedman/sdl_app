#include "explorer_view.h"
#include "explorer.h"
#include "renderer.h"

explorer_view_t::explorer_view_t()
    : list_t()
{
    on(EVT_ITEM_SELECT, [this](event_t& evt) {
        evt.cancelled = true;
        view_t* item = (view_t*)evt.source;
        if (!item)
            return false;
        list_item_data_t d = item->cast<list_item_t>()->item_data;
        fileitem_t* file = (fileitem_t*)d.data;
        if (file->isDirectory) {
            if (file->canLoadMore) {
                explorer_t::instance()->loadFolder(file);
                file->canLoadMore = false;
            }
            file->expanded = !file->expanded;
            explorer_t::instance()->regenerateList = true;
            explorer_t::instance()->update(0);
            explorer_t::instance()->print();
            update_explorer_data();
        }
        return true;
    });

    layout()->margin = 8;
}

void explorer_view_t::update_explorer_data()
{
    std::vector<list_item_data_t> data;
    for (auto f : explorer_t::instance()->renderList) {
        list_item_data_t d = {
            value : f->fullPath,
            text : f->name,
            indent : f->depth * (font()->width * 2),
            data : f
        };
        data.push_back(d);
    }
    update_data(data);
}

void explorer_view_t::set_root_path(std::string path)
{
    explorer_t::instance()->setRootFromFile(path);
    explorer_t::instance()->update(0);
    update_explorer_data();
}