#include "explorer_view.h"
#include "app.h"
#include "explorer.h"
#include "renderer.h"
#include "system.h"

explorer_task_t::explorer_task_t(explorer_view_t* ev)
    : task_t()
    , ev(ev)
{
}

bool explorer_task_t::run(int limit)
{
    if (!running)
        return false;

    explorer_t::instance()->update(0);
    if (explorer_t::instance()->regenerateList) {
        ev->update_explorer_data();
        explorer_t::instance()->regenerateList = false; // done
        return true;
    }
    return false;
}

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

    layout()->margin_left = 8;
    layout()->margin_top = 8;
    layout()->margin_bottom = 0;
    layout()->margin_right = 0;
}

void explorer_view_t::update_explorer_data()
{
    icon_t ico;
    ico = icon_for_file(app_t::instance()->icons, ".folder-open", app_t::instance()->extensions);
    std::string folder_icon_path = ico.path;
    ico = icon_for_file(app_t::instance()->icons, ".folder", app_t::instance()->extensions);
    std::string folder_close_icon_path = ico.path;

    std::vector<list_item_data_t> data;
    for (auto f : explorer_t::instance()->renderList) {
        list_item_data_t d = {
            value : f->fullPath,
            text : f->name,
            indent : f->depth * (font()->width * 2),
            data : f
        };

        if (f->isDirectory) {
            d.icon = f->expanded ? folder_icon_path : folder_close_icon_path;
        } else {
            if (app_t::instance()->icons) {
                ico = icon_for_file(app_t::instance()->icons, f->name, app_t::instance()->extensions);
                d.icon = ico.path;
                if (!ico.svg) {
                    d.icon_font = ico.path;
                    d.icon = ico.character;
                }
            }
        }

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

void explorer_view_t::start_tasks()
{
    if (!task) {
        task = std::make_shared<explorer_task_t>(this);
        tasks_manager_t::instance()->enroll(task);
    }
}

void explorer_view_t::stop_tasks()
{
    if (task) {
        task->stop();
        tasks_manager_t::instance()->withdraw(task);
        task = nullptr;
    }
}

void explorer_view_t::render(renderer_t* renderer)
{
    layout_item_ptr lo = layout();
    color_t clr = color_darker(system_t::instance()->renderer.background, 10);
    renderer->draw_rect(lo->render_rect, clr, true, 0);
    list_t::render(renderer);
}