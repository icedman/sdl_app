#include "layout.h"
#include "renderer.h"
#include "render_cache.h"
#include "events.h"
#include "tests.h"

#include "app.h"
#include "backend.h"
#include "search.h"
#include "statusbar.h"
#include "explorer.h"
#include "editor.h"
#include "operation.h"
#include "keyinput.h"
#include "util.h"

#include "text.h"
#include "button.h"

#include "theme.h"

#define FRAME_RENDER_INTERVAL 24

struct sdl_backend_t : backend_t {
    void setClipboardText(std::string text) override {
        ren_set_clipboard(text);
    };

    std::string getClipboardText() override {
        return ren_get_clipboard();
    };
};

std::map<int, color_info_t> colorMap;

void updateColors()
{
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;

    auto it = theme->colorIndices.begin();
    while (it != theme->colorIndices.end()) {
        // printf("%d %f %f %f\n", it->first, it->second.red, it->second.green, it->second.blue);
        color_info_t fg = it->second;
        fg.red = fg.red <= 1 ? fg.red * 255 : fg.red;
        fg.green = fg.green <= 1 ? fg.green * 255 : fg.green;
        fg.blue = fg.blue <= 1 ? fg.blue * 255 : fg.blue;
        colorMap[it->second.index] = fg;
        it++;
    }
}

void render_item(layout_item_ptr item)
{
    if (!item->visible) {
        return;
    }

    view_item *view = (view_item*)item->view;

    state_save();
    set_clip_rect({
        item->render_rect.x - 1,
        item->render_rect.y - 1,
        item->render_rect.w + 2,
        item->render_rect.h + 2
    });

    bool fill = false;
    float stroke = 1.0f;
    RenColor clr = { (uint8_t)item->rgb.r, (uint8_t)item->rgb.g, (uint8_t)item->rgb.b, 50 };
    if (view && view->is_hovered()) {
        clr.a = 200;
    }
    if (view && view->is_pressed()) {
        // clr = { 255, 0, 0 };
        // fill = true;
        stroke = 1.5f;
    }

    if (view && view->is_clicked()) {
        printf(">>click\n");
    }

    if (view) {
        view->render();
    }

    // printf("%l %d %d %d %d\n", ct, item->render_rect.x, item->render_rect.y, item->render_rect.w, item->render_rect.h);
    draw_rect({
        item->render_rect.x,
        item->render_rect.y,
        item->render_rect.w,
        item->render_rect.h
    },
    clr, fill, stroke);

    std::string text = item->view ? ((view_item*)item->view)->name : item->name;
    if (item->view && ((view_item*)item->view)->type == "text") {
        text = ((text_view*)item->view)->text;
    }

    // draw_text(fontUI, (char*)text.c_str(), item->render_rect.x + 4, item->render_rect.y + 2, { 255, 255, 0},
    //     false, false, true);

    for(auto child : item->children) {
        child->render_rect = child->rect;
        child->render_rect.x += item->render_rect.x + item->scroll_x;
        child->render_rect.y += item->render_rect.y + item->scroll_y;
        render_item(child);
    }

    state_restore();
}

int main(int argc, char **argv)
{
    sdl_backend_t backend;
    app_t app;
    keybinding_t keybinding;
    explorer_t explorer;
    statusbar_t statusbar;
    search_t search;

    app.configure(argc, argv);
    app.setupColors();
    std::string file = "./src/main.cpp";
    if (argc > 1) {
        file = argv[argc - 1];
    }
    updateColors();

    theme_ptr theme = app.theme;

    app.openEditor(file);
    explorer.setRootFromFile(file);

    // app.currentEditor->singleLineEdit = true;

    ren_init();
    rencache_init();

    color_info_t bg = colorMap[app.bgApp];

    // quick draw
    int w, h;
    ren_get_window_size(&w, &h);
    ren_begin_frame();
    ren_draw_rect({x:0,y:0,width:w,height:h}, { (uint8_t)bg.red,(uint8_t)bg.green,(uint8_t)bg.blue });
    ren_end_frame();
    w = 0;
    h = 0;

    // RenImage *tmp = ren_create_image_from_svg((char*)icon.c_str(), 24,24);
    // RenImage *tmp = ren_create_image(80,80);
    // ren_begin_frame(tmp);
    // ren_draw_rect({0,0,80,80}, {150,0,150});
    // ren_draw_rect({20,20,20,20}, {255,0,0});
    // ren_end_frame();

    view_item_ptr root_view = test_root();
    layout_item_ptr root = root_view->layout();

    view_item_list view_list;
    layout_item_list render_list;
    event_list events;

    // RenFont *font = ren_create_font("Monaco 12");
    RenFont *font = ren_create_font("Fira Code 16");
    // RenFont *font = ren_create_font("Source Code Pro 16");
    ren_create_font("Source Code Pro 12", "ui");
    ren_set_default_font(font);

    rencache_show_debug(true);

    int frames = FRAME_RENDER_INTERVAL;
    while(ren_is_running()) {
        ren_listen_events(&events);

        view_list.clear();
        view_input_list(view_list, root_view);
        view_input_events(view_list, events);

        app_t::instance()->currentEditor->runAllOps();
        root_view->update();

        int pw = w;
        int ph = h;
        ren_get_window_size(&w, &h);
        if (layout_should_run() || pw != w || ph != h) {
            layout_run(root, { 0, 0, w, h });
            render_list.clear();
            layout_render_list(render_list, root); // << this positions items on the screen
            frames = FRAME_RENDER_INTERVAL - 4;
        }

        // todo implement frame rate limit
        if (frames++ < FRAME_RENDER_INTERVAL) {
            continue;
        }
        frames = 0;

        begin_frame(w, h);
        state_save();

        draw_rect({x:0,y:0,width:w,height:h}, { (uint8_t)bg.red,(uint8_t)bg.green,(uint8_t)bg.blue });

        render_item(root);

        // draw_image(tmp,{0,0,80,80});

        state_restore();
        end_frame();
    }

    rencache_shutdown();
    ren_shutdown();

    app.shutdown();
}
