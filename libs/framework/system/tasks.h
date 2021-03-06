#ifndef TASKS_H
#define TASKS_H

#include <memory>
#include <vector>

struct task_t {
    task_t()
        : withdraw_on_done(false)
        , running(true)
    {
    }

    virtual bool run(int limit) { return false; }
    void start() { running = true; }
    void stop() { running = false; }

    bool withdraw_on_done;
    bool running;
};

typedef std::shared_ptr<task_t> task_ptr;
typedef std::vector<task_ptr> task_list;

struct tasks_manager_t {
    tasks_manager_t();

    static tasks_manager_t* instance();

    void enroll(task_ptr task);
    void withdraw(task_ptr task);
    bool run(int limit);

    task_list tasks;
    int robin;
};

#endif TASKS_H