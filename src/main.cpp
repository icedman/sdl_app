// printf("%d %f %f %f\n", it->first, it->second.red, it->second.green, it->second.blue);// printf("%d %f %f %f\n", it->first, it->second.red, it->second.green, it->second.blue);

#include "animation.h"
#include "events.h"
#include "layout.h"
#include "renderer.h"
#include "scripting.h"
#include "tests.h"

#include "app.h"
#include "backend.h"
#include "editor.h"
#include "explorer.h"
#include "operation.h"
#include "search.h"
#include "statusbar.h"
#include "utf8.h"

#include "button.h"
#include "text.h"

#include "theme.h"

#include "block.h"
#include <set>
#include <vector>

#define FRAME_SKIP_INTERVAL 32
#define FRAME_RATE 120

struct sdl_backend_t : backend_t {
    void setClipboardText(std::string text) override
    {
        Renderer::instance()->set_clipboard(text);
    }

    std::string getClipboardText() override
    {
        return Renderer::instance()->get_clipboard();
    }
};

int main(int argc, char** argv)
{
    style_init();

    sdl_backend_t backend;
    app_t app;
    keybinding_t keybinding;
    explorer_t explorer;
    statusbar_t statusbar;
    search_t search;

    Renderer* renderer = Renderer::instance();
    Scripting* scripting = Scripting::instance();

    app.configure(argc, argv);
    app.setupColors(!renderer->is_terminal());
    std::string file = app.inputFile;
    if (file == "") {
        file = "~/";
    }

    theme_ptr theme = app.theme;

    app.openEditor(file);
    explorer.setRootFromFile(file);

    renderer->init();
    scripting->init();

    color_info_t bg = renderer->color_for_index(app.bgApp);

    // quick draw
    int w, h;
    renderer->get_window_size(&w, &h);
    renderer->begin_frame();
    renderer->draw_rect({ x : 0, y : 0, width : w, height : h }, { (uint8_t)bg.red, (uint8_t)bg.green, (uint8_t)bg.blue });
    renderer->end_frame();
    w = 0;
    h = 0;

    view_item_ptr root_view = test_root();
    layout_item_ptr root = root_view->layout();

    view_item_list view_list;
    layout_item_list render_list;
    event_list events;

    // RenFont *font = renderer->create_font("Monaco 12");
    RenFont* font = renderer->create_font("Fira Code 12", "editor");
    // RenFont* font = renderer->create_font("Source Code Pro 12", "editor");
    // renderer->register_font("/home/iceman/.ashlar/fonts/monospace.ttf");
    renderer->create_font("Source Code Pro 12", "ui");
    renderer->create_font("Source Code Pro 10", "ui-small");
    renderer->set_default_font(font);

    int frames = FRAME_SKIP_INTERVAL;

    float fps = 0;
    const int target_fps = FRAME_RATE;
    const int max_elapsed = 1000 / target_fps;
    while (renderer->is_running()) {
        backend.begin();
        renderer->listen_events(&events);

        bool skip_frames = renderer->is_idle() || renderer->is_throttle_up_events();

        int pw = w;
        int ph = h;
        renderer->get_window_size(&w, &h);
        if (layout_should_run() || pw != w || ph != h) {
            layout_run(root, { 0, 0, w, h });
            render_list.clear();
            layout_render_list(render_list, root);
            frames = FRAME_SKIP_INTERVAL;
        }

        // input based updates
        view_input_events(view_list, events);
        events.clear();

        scripting->update(0);
        root_view->update(0);

        if (pw != w || ph != h) {
            renderer->damage_rects.clear();
            renderer->damage({ 0, 0, w, h });
        }

        if (skip_frames) {
            if (frames++ > FRAME_SKIP_INTERVAL) {
                skip_frames = false;
                frames = 0;
            }
        }

        // make draw efficient
        if (!skip_frames) {

            renderer->prerender_view_tree((view_item*)root->view);

            renderer->begin_frame(NULL, w, h);

            renderer->state_save();
            renderer->render_view_tree((view_item*)root->view);
            renderer->state_restore();

            {
                static int count = 0;
                static int damages = 0;
                if (renderer->draw_count() > 0) {
                    count = renderer->draw_count();
                }
                if (renderer->damage_rects.size() > 0) {
                    damages = renderer->damage_rects.size();
                }
                char tmp[64];
                sprintf(tmp, "fps: %04d damages: %04d drawn: %04d", (int)fps, damages, count);
                int fw, fh;
                renderer->get_font_extents(NULL, &fw, &fh, tmp, strlen(tmp));
                int fx = renderer->is_terminal() ? 2 : 20;
                int fy = renderer->is_terminal() ? 1 : 20;
                renderer->damage({fx,fy,fw,fh});
                renderer->draw_rect({fx,fy,fw,fh}, {50,50,50}, true);
                renderer->draw_text(NULL, tmp, fx, fy, { 255, 255, 255 });
            }

            renderer->end_frame();

            view_list.clear();
            view_input_list(view_list, root_view);
        }

        do {
            if (renderer->is_terminal())
                break;

            renderer->listen_events(&events);
            if (renderer->is_throttle_up_events()) {
                break;
            }

            if (renderer->is_idle()) {
                backend.delay(30);
            }
        } while (backend.elapsed() < max_elapsed);

        if (!skip_frames)
            fps = 1000.0f / backend.elapsed();
    }

    renderer->shutdown();
    scripting->shutdown();

    app.shutdown();
    return 0;
}
