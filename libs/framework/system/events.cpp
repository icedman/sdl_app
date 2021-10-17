#include "events.h"

static events_manager_t global_events;

events_manager_t* events_manager_t::instance()
{
    return &global_events;
}

int events_manager_t::on(event_type_e event_type, event_callback_t callback)
{
    callbacks[event_type].insert(callbacks[event_type].begin(), callback);
    return callbacks[event_type].size();
}

void events_manager_t::dispatch_event(event_t& event)
{
    if (event.cancelled)
        return;

    for (auto cb : callbacks[EVT_ALL]) {
        cb(event);
        if (event.cancelled) {
            break;
        }
    }

    if (event.cancelled)
        return;

    for (auto cb : callbacks[event.type]) {
        cb(event);
        if (event.cancelled) {
            break;
        }
    }
}

void events_manager_t::dispatch_events(event_list& events)
{
    for (auto& event : events) {
        dispatch_event(event);
    }
}