#include <stdio.h>
#include <string.h>

#include "damage.h"
#include "hash.h"
#include "layout.h"
#include "popup.h"
#include "system.h"
#include "tasks.h"
#include "view.h"

#include "operation.h"

#define FRAME_SKIP_INTERVAL 32

view_ptr test(int argc, char** argv);
void render_layout_item(renderer_t* renderer, layout_item_ptr item);

extern "C" int main(int argc, char** argv)
{
    system_t* sys = system_t::instance();
    damage_t* dmg = damage_t::instance();
    // dmg = NULL;

    renderer_t* renderer = &sys->renderer;
    events_manager_t* events_manager = events_manager_t::instance();
    tasks_manager_t* tasks_manager = tasks_manager_t::instance();

    renderer->foreground = { 255, 255, 255 };
    renderer->background = { 50, 50, 50 };

    event_list events;
    view_list visible_views;

    int frames = FRAME_SKIP_INTERVAL;
    float fps = 0;
    const int target_fps = sys->target_fps();
    const int max_elapsed = 1000 / target_fps;

    int skipped = 0;

    renderer->create_font("Source Code Pro", 12, "ui");
    renderer->create_font("Fira Code", 14, "editor");

    int d = 0;

    sys->init();

    view_ptr root = test(argc, argv);
    root->add_child(popup_manager_t::instance());

    // quick first render
    renderer->begin_frame();
    renderer->clear(renderer->background);
    renderer->end_frame();

    int suspend_frame_skipping = 0;
    bool did_layout = true;
    layout_request();
    events_manager->on(EVT_WINDOW_RESIZE, [](event_t& evt) {
        layout_request();
        return false;
    });
    events_manager->on(EVT_KEY_SEQUENCE, [](event_t& evt) {
        if (evt.text == "ctrl+r") {
            layout_request();
        }
        return false;
    });

    while (sys->is_running()) {
        sys->timer.begin();

        sys->poll_events(&events);
        if (events.size()) {
            events_manager->dispatch_events(events);
            view_dispatch_events(events, visible_views);
            events.clear();
        }

        // update
        root->update();

        // layout
        if (layout_should_run()) {
            layout_run(root->layout(), { 0, 0, renderer->width(), renderer->height() });
            did_layout = true;
        }

        // todo control skipping with actual framerate (throttling)
        bool skip_frames = true;
        if (skip_frames) {
            if (frames++ > FRAME_SKIP_INTERVAL) {
                skip_frames = false;
                frames = 0;
            }
        }

        if (suspend_frame_skipping > 0) {
            suspend_frame_skipping--;
            skip_frames = false;
            sys->caffeinate();
        }

        // render
        if (!skip_frames) {
            if (dmg && did_layout) {
                dmg->damage_whole();
                did_layout = false;
            }

            renderer->begin_frame();

            visible_views.clear();
            view_prerender(root, visible_views, dmg);

            if (!dmg || dmg->count()) {
                renderer->clear(renderer->background);
            }

            view_render(renderer, root, dmg);

#if 1
            if (dmg) {
                printf("damages:%d rendered:%d skipped:%d\n", dmg->count(), renderer->draw_count(), skipped);
                // for(auto d : dmg->damage_rects) {
                //     printf("%d %d %d %d\n", d.x,d.y,d.w,d.h);
                // }
            } else {
                printf("rendered:%d skipped:%d\n", renderer->draw_count(), skipped);
            }
#endif

            if (dmg) {
                renderer->set_update_rects(dmg->rects(), dmg->count());
            }

            // render_layout_item(renderer, root->layout());

            renderer->end_frame();

            if (dmg) {
                dmg->clear();
            }

            skipped = 0;
        } else {
            skipped++;
        }

        do {
            if (sys->poll_events(&events)) {
                if (events.size()) {
                    if (events[0].type == EVT_MOUSE_MOTION && events[0].button == 0) {
                    // ignore
                    } else {
                        break;
                    }
                }
            }

            if (sys->is_caffeinated()) {
                break;
            }

            if (sys->is_idle()) {
                if (!tasks_manager->run(max_elapsed)) {
                    sys->delay(50);
                }
            }

        } while (sys->timer.elapsed() < max_elapsed);

        fps = 1000.0f / sys->timer.elapsed();
        if (fps > 0 && fps < 1000) {
            // printf("fps:%d\n", (int)fps);
            sys->stats.fps = fps;
        }
    }

    printf("visible views %d\n", visible_views.size());

    sys->shutdown();

    printf("bye\n");
    return 0;
}
