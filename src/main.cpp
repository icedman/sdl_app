#include "layout.h"
#include "renderer.h"
#include "render_cache.h"
#include "events.h"
#include "tests.h"

#include "text.h"

void render_item(layout_item_ptr i)
{
    bool fill = false;
    float stroke = 1.0f;
    RenColor clr = { i->rgb.r, i->rgb.g, i->rgb.b };
    if (i->view && i->view->is_hovered()) {
        clr = { 150, 0, 150 };
    }
    if (i->view && i->view->is_pressed()) {
        clr = { 255, 0, 0 };
        // fill = true;
        stroke = 1.5f;
    }
    if (i->view && i->view->is_clicked()) {
        printf(">>click\n");
    }

    // printf("%l %d %d %d %d\n", ct, i->render_rect.x, i->render_rect.y, i->render_rect.w, i->render_rect.h);
    rencache_draw_rect({
        i->render_rect.x,
        i->render_rect.y,
        i->render_rect.w,
        i->render_rect.h
    },
    clr, fill, stroke);

    std::string text = i->view ? ((view_item*)i->view)->name : i->name;
    if (i->view && ((view_item*)i->view)->type == "text") {
        text = ((text_view*)i->view)->text;
    }
    rencache_draw_text(NULL, (char*)text.c_str(), i->render_rect.x + 4, i->render_rect.y + 2, { 255, 255, 0},
        false, false, true);
}

int main(int argc, char **argv)
{
    ren_init();
    rencache_init();

    RenImage *tmp = ren_create_image(80,80);
    ren_begin_frame(tmp);
    ren_draw_rect({0,0,80,80}, {150,0,150});
    ren_draw_rect({20,20,20,20}, {255,0,0});
    ren_end_frame();

    view_item_ptr root_view = test3();
    layout_item_ptr root = root_view->layout();

    view_item_list view_list;
    layout_item_list render_list;
    event_list events;

    // RenFont *font = ren_create_font("Fira Code 14");
    RenFont *font = ren_create_font("Source Code Pro 16");
    ren_set_default_font(font);

    // rencache_show_debug(true);

    int pw, ph;

    while(ren_is_running()) {
        ren_listen_events(&events);

        view_list.clear();
        view_input_list(view_list, root_view);
        view_input_events(view_list, events);

        int w, h;
        ren_get_window_size(&w, &h);

        if (pw != w || ph != h) {
            pw = w;
            ph = h;

            layout_run(root, { 0, 0, w, h });
            render_list.clear();
            layout_render_list(render_list, root);
        }

        ren_begin_frame();
        rencache_begin_frame(w, h);

        rencache_draw_rect({x:0,y:0,width:w,height:h}, { 150, 150, 150});
        // rencache_draw_text(font, "Hello World", 20, 20, { 255, 255, 0 });
        // rencache_draw_text(font, "Hello World", 20, 40, { 255, 255, 0 });

        rencache_draw_image(tmp, {240,240,80,80});

        for(auto i : render_list) {
            render_item(i);
        }

        rencache_end_frame();
        ren_end_frame();
    }

    rencache_shutdown();
    ren_shutdown();
}