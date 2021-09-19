// printf("%d %f %f %f\n", it->first, it->second.red, it->second.green, it->second.blue);// printf("%d %f %f %f\n", it->first, it->second.red, it->second.green, it->second.blue);

#include "events.h"
#include "layout.h"
#include "renderer.h"
#include "tests.h"
 
#include "app.h"
#include "backend.h"
#include "editor.h"
#include "explorer.h"
#include "keyinput.h"
#include "operation.h"
#include "search.h"
#include "statusbar.h"

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

static inline void render_item(layout_item_ptr item)
{
    if (!item->visible || item->offscreen) {
        return;
    }

    // ren_timer_begin();

    view_item* view = (view_item*)item->view;

    RenRect clip = {
        item->render_rect.x - 1,
        item->render_rect.y - 1,
        item->render_rect.w + 2,
        item->render_rect.h + 2
    };

    Renderer::instance()->state_save();

    if (view->_views.size()) {
        Renderer::instance()->set_clip_rect(clip);
    }

    bool fill = false;
    int stroke = 1;
    RenColor clr = { (uint8_t)item->rgb.r, (uint8_t)item->rgb.g, (uint8_t)item->rgb.b, 50 };
    if (view && view->is_hovered()) {
        clr.a = 200;
    }
    if (view && view->is_pressed()) {
        // clr = { 255, 0, 0 };
        // fill = true;
        stroke = 1.5f;
        printf(">>press %s\n", view->type.c_str());
    }
    if (view && view->is_clicked()) {
        printf(">>click %s\n", view->type.c_str());
    }

    if (view) {
        view->render();
    }

    // printf("%l %d %d %d %d\n", ct, item->render_rect.x, item->render_rect.y, item->render_rect.w, item->render_rect.h);
    // draw_rect({
    //     item->render_rect.x,
    //     item->render_rect.y,
    //     item->render_rect.w,
    //     item->render_rect.h
    // },
    // clr, fill, stroke, 0);

    // std::string text = item->view ? ((view_item*)item->view)->name : item->name;
    // if (item->view && ((view_item*)item->view)->type == "text") {
    //     text = ((text_view*)item->view)->text;
    // }

    // draw_text(fontUI, (char*)text.c_str(), item->render_rect.x + 4, item->render_rect.y + 2, { 255, 255, 0},
    //     false, false, true);

    for (auto child : item->children) {
        child->render_rect = child->rect;
        child->render_rect.x += item->render_rect.x + item->scroll_x;
        child->render_rect.y += item->render_rect.y + item->scroll_y;

        // RenRect cc = {
        //     child->render_rect.x,
        //     child->render_rect.y,
        //     child->render_rect.w,
        //     child->render_rect.h
        // };

        render_item(child);
    }

    Renderer::instance()->state_restore();

    // printf("%s : %d\n", ((view_item*)(item->view))->type.c_str(), ren_timer_end());
}

int main(int argc, char** argv)
{
    sdl_backend_t backend;
    app_t app;
    keybinding_t keybinding;
    explorer_t explorer;
    statusbar_t statusbar;
    search_t search;
   
    app.configure(argc, argv);
    app.setupColors(!Renderer::instance()->is_terminal());
    std::string file = "./src/main.cpp";
    if (argc > 1) {
        file = argv[argc - 1];
    }

    theme_ptr theme = app.theme;

    app.openEditor(file);
    explorer.setRootFromFile(file);

    // app.currentEditor->singleLineEdit = true;
    Renderer::instance()->init();

    color_info_t bg = Renderer::instance()->color_for_index(app.bgApp);

    // quick draw
    int w, h;
    Renderer::instance()->get_window_size(&w, &h);
    Renderer::instance()->begin_frame();
    Renderer::instance()->draw_rect({ x : 0, y : 0, width : w, height : h }, { (uint8_t)bg.red, (uint8_t)bg.green, (uint8_t)bg.blue });
    Renderer::instance()->end_frame();
    w = 0;
    h = 0;

    view_item_ptr root_view = test_root();
    layout_item_ptr root = root_view->layout();

    view_item_list view_list;
    layout_item_list render_list;
    event_list events;

    // RenFont *font = Renderer::instance()->create_font("Monaco 12");
    RenFont* font = Renderer::instance()->create_font("Fira Code 14", "editor");
    // RenFont* font = Renderer::instance()->create_font("Source Code Pro 12", "editor");
    // Renderer::instance()->register_font("/home/iceman/.ashlar/fonts/monospace.ttf");
    Renderer::instance()->create_font("Source Code Pro 12", "ui");
    Renderer::instance()->create_font("Source Code Pro 10", "ui-small");
    Renderer::instance()->set_default_font(font);

    // Renderer::instance()->show_debug(true);

    int frames = FRAME_RENDER_INTERVAL;
    while (Renderer::instance()->is_running()) {
        if (app_t::instance()->currentEditor) {
            app_t::instance()->currentEditor->runAllOps();
        }
        if (statusbar_t::instance()) {
            statusbar_t::instance()->update(10);
        }

        root_view->update();

        int pw = w;
        int ph = h;
        Renderer::instance()->get_window_size(&w, &h);
        if (layout_should_run() || pw != w || ph != h) {
            layout_run(root, { 0, 0, w, h });
            render_list.clear();
            layout_render_list(render_list, root); // << this positions items on the screen
            frames = FRAME_RENDER_INTERVAL - 4;
        }

        // todo implement frame rate throttling
        bool skip_render = false;
        if (Renderer::instance()->listen_is_quick()) {
            if (Renderer::instance()->is_terminal()) {
                frames = FRAME_RENDER_INTERVAL;
            }
            if (frames++ < FRAME_RENDER_INTERVAL) {
                skip_render = true;
            }
            frames = 0;
        }

        if (!skip_render) {
            Renderer::instance()->begin_frame(NULL, w, h);
            Renderer::instance()->state_save();

            render_item(root);

            Renderer::instance()->state_restore();
            Renderer::instance()->end_frame();
        }

        Renderer::instance()->listen_events(&events);

        view_list.clear();
        view_input_list(view_list, root_view);
        view_input_events(view_list, events);

    }

    Renderer::instance()->shutdown();

    app.shutdown();
}
