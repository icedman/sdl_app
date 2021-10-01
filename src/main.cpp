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

#define FRAME_RENDER_INTERVAL 16

extern int ren_rendered;

struct sdl_backend_t : backend_t {
    void setClipboardText(std::string text) override
    {
        Renderer::instance()->set_clipboard(text);
    };

    std::string getClipboardText() override
    {
        return Renderer::instance()->get_clipboard();
    };
};

int main(int argc, char** argv)
{
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

    int frames = FRAME_RENDER_INTERVAL;
    while (renderer->is_running()) {
        int elapsed = renderer->ticks();
        if (app_t::instance()->currentEditor) {
            app_t::instance()->currentEditor->runAllOps();
        }

        scripting->update(elapsed);
        root_view->update(elapsed);
        if (animation::has_animations()) {
            renderer->throttle_up_events();
        }

        int pw = w;
        int ph = h;
        renderer->get_window_size(&w, &h);
        if (layout_should_run() || pw != w || ph != h) {
            layout_run(root, { 0, 0, w, h });
            render_list.clear();
            layout_render_list(render_list, root); // << this positions items on the screen
            frames = FRAME_RENDER_INTERVAL;
        }

        // todo implement frame rate throttling
        bool skip_render = renderer->is_throttle_up_events();
        if (skip_render) {
            if (frames++ > FRAME_RENDER_INTERVAL) {
                skip_render = false;
                frames = 0;
            }
        }

        if (!skip_render) {
            renderer->begin_frame(NULL, w, h);
            renderer->state_save();

            renderer->render_view_tree((view_item*)root->view);

            renderer->state_restore();
            renderer->end_frame();

            view_list.clear();
            view_input_list(view_list, root_view);
        }

        renderer->listen_events(&events);
        view_input_events(view_list, events);
    }

    renderer->shutdown();
    scripting->shutdown();

    app.shutdown();
}
