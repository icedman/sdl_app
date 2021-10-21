#include "tasks.h"

#include <algorithm>
#include <cstdio>

static tasks_manager_t global_tasks;

tasks_manager_t* tasks_manager_t::instance()
{
    return &global_tasks;
}

tasks_manager_t::tasks_manager_t()
    : robin(0)
{
}

void tasks_manager_t::enroll(task_ptr task)
{
    if (std::find(tasks.begin(), tasks.end(), task) == tasks.end()) {
        tasks.push_back(task);
    }
}

void tasks_manager_t::withdraw(task_ptr task)
{
    task_list::iterator it = std::find(tasks.begin(), tasks.end(), task);
    if (it != tasks.end()) {
        tasks.erase(it);
    }
}

bool tasks_manager_t::run(int limit)
{
    if (!tasks.size())
        return false;

    task_ptr task = tasks[robin];
    robin = (robin + 1) % tasks.size();

    bool res = task->run(limit);
    if (!res && task->withdraw_on_done) {
        withdraw(task);
    }

    return res;
}