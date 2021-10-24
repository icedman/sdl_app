#ifndef EXPLORER_VIEW_H
#define EXPLORER_VIEW_H

#include "list.h"
#include "tasks.h"

struct explorer_view_t;
struct explorer_task_t : task_t {
    explorer_task_t(explorer_view_t* ev);

    bool run(int limit) override;

    explorer_view_t* ev;
};

struct explorer_view_t : list_t {
    explorer_view_t();

    DECLAR_VIEW_TYPE(CUSTOM, list_t)
    virtual std::string type_name() { return "explorer"; }

    void render(renderer_t* renderer) override;
    void set_root_path(std::string path);
    void update_explorer_data();

    void start_tasks();
    void stop_tasks();

    task_ptr task;
};

#endif // EXPLORER_VIEW_H