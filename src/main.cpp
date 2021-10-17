#include <stdio.h>
#include <string.h>

#include "damage.h"
#include "hash.h"
#include "layout.h"
#include "popup.h"
#include "system.h"
#include "view.h"

#include "operation.h"

#define FRAME_SKIP_INTERVAL 32

view_ptr test(int argc, char **argv);

void render_layout_item(renderer_t* renderer, layout_item_ptr item)
{
    if (!item->visible)
        return;

    renderer->draw_rect(item->render_rect, { 255, 255, 255 }, false, 1.0f);
    renderer->draw_text(NULL, (char*)item->name.c_str(), item->render_rect.x, item->render_rect.y, { 255, 255, 255 });
    for (auto child : item->children) {
        render_layout_item(renderer, child);
    }
}

extern "C" int main(int argc, char** argv)
{
    system_t* sys = system_t::instance();
    damage_t* dmg = damage_t::instance();
    // dmg = NULL;

    renderer_t* renderer = &sys->renderer;
    events_manager_t* events_manager = events_manager_t::instance();

    event_list events;
    view_list visible_views;

    int frames = FRAME_SKIP_INTERVAL;
    float fps = 0;
    const int target_fps = sys->target_fps();
    const int max_elapsed = 1000 / target_fps;

    int skipped = 0;

    image_t* svg = renderer->create_image_from_svg("./tests/3d.svg", 40, 40);
    image_t* png = renderer->create_image_from_png("./tests/game.png");
    // font_t* fnt = renderer->create_font("Source Code Pro", 16);
    font_t* fnt = renderer->create_font("Fira Code", 14);
    // font_t* fnt = renderer->create_font("asteroids", 10);
    int d = 0;

    sys->init();

    view_ptr root = test(argc, argv);
    root->add_child(popup_manager_t::instance());

    // quick first render
    renderer->begin_frame();
    renderer->clear({ 50, 50, 50 });
    renderer->end_frame();

    bool did_layout = true;
    layout_request();
    events_manager->on(EVT_WINDOW_RESIZE, [](event_t& evt) {
        layout_request();
        return false;
    });
    while (sys->is_running()) {
        sys->timer.begin();

        sys->poll_events(&events);

        if (events.size()) {
            view_dispatch_events(events, visible_views);
            events_manager->dispatch_events(events);
            events.clear();
        }

        // update
        root->update();

        // layout
        layout_run_requests();
        if (layout_should_run()) {
            layout_run(root->layout(), { 0, 0, renderer->width(), renderer->height() });
            did_layout = true;
            sys->caffeinate();
        }

        // todo control skipping with actual framerate (throttling)
        bool skip_frames = true;
        if (skip_frames) {
            if (frames++ > FRAME_SKIP_INTERVAL) {
                skip_frames = false;
                frames = 0;
            }
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
                renderer->clear({ 50, 50, 50 });
            }

            view_render(renderer, root, dmg);

            if (dmg) {
                // printf("damages:%d rendered:%d skipped:%d\n", dmg->count(), renderer->draw_count(), skipped);
                // for(auto d : dmg->damage_rects) {
                //     printf("%d %d %d %d\n", d.x,d.y,d.w,d.h);
                // }
            }

            // renderer->draw_rect({20,20,renderer->width()-40,renderer->height()-40},{255,0,255}, false, 2);
            // renderer->draw_line(20,20,420,420,{255,0,255});
            // renderer->draw_rect({20,20,400,400},{255,255,255}, true, 4, {255,0,255}, 40);

            // renderer->push_state();
            // renderer->translate(40, 40);
            // renderer->rotate(d);
            // renderer->draw_image(svg, {0,0,40,40}, {255,0,0});
            // renderer->pop_state();
            // d += 4;
            // d = d % 360;

            // renderer->draw_image(png, {400+132,40,132/2,122/2});
            // renderer->draw_image(png, 400,40);

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
            skipped ++;
        }

        do {
            if (sys->poll_events(&events)) {
                break;
            }

            if (sys->is_caffeinated()) {
                break;
            }

            // do work here

            if (!sys->is_caffeinated() && sys->is_idle()) {
                sys->delay(30);
            }

        } while (sys->timer.elapsed() < max_elapsed);

        fps = 1000.0f / sys->timer.elapsed();

        // if (fps > 0 && fps < 1000) {
        //     printf("fps:%d\n", (int)fps);
        // }
    }

    sys->shutdown();

    renderer_t::destroy_image(svg);
    renderer_t::destroy_image(png);
    renderer_t::destroy_font(fnt);
    return 0;
}