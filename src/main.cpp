#include "layout.h"
#include "renderer.h"
#include "render_cache.h"
#include "events.h"
#include "tests.h"

int main(int argc, char **argv)
{
    ren_init();

    view_item_ptr root_view = test3();
    layout_item_ptr root = root_view->layout();

    view_item_list view_list;
    layout_item_list render_list;
    event_list events;

    // RenFont *font = ren_create_font("Fira Code 14");
    RenFont *font = ren_create_font("Source Code Pro 16");

    // rencache_show_debug(true);

    int pw, ph;

    while(ren_is_running()) {
        ren_listen_events(&events);

        view_list.clear();
        view_input_list(view_list, root_view);
        view_input_events(view_list, events);

        ren_begin_frame();
        rencache_begin_frame();
        
        int w, h;
        ren_get_size(&w, &h);

        if (pw != w || ph != h) {
            pw = w;
            ph = h;

            layout_run(root, { 0, 0, w, h });
            render_list.clear();
            layout_render_list(render_list, root);
        }

        rencache_draw_rect({x:0,y:0,width:w,height:h}, { 150, 150, 150});
        // rencache_draw_text(font, "Hello World", 20, 20, { 255, 255, 0 });
        // rencache_draw_text(font, "Hello World", 20, 40, { 255, 255, 0 });

        for(auto i : render_list) {
            bool fill = false;
            RenColor clr = { i->rgb.r, i->rgb.g, i->rgb.b };
            if (i->view && i->view->is_hovered()) {
                clr = { 150, 0, 150 };
            }
            if (i->view && i->view->is_pressed()) {
                clr = { 255, 0, 0 };
                fill = true;
            }
            // printf("%l %d %d %d %d\n", ct, i->render_rect.x, i->render_rect.y, i->render_rect.w, i->render_rect.h);
            rencache_draw_rect({
                i->render_rect.x,
                i->render_rect.y,
                i->render_rect.w,
                i->render_rect.h
            },
            clr, fill, 1.0f);
            // break;

            std::string text = i->view ? ((view_item*)i->view)->name : i->name;
            rencache_draw_text(font, (char*)text.c_str(), i->render_rect.x + 4, i->render_rect.y + 2, { 255, 255, 0});
        }

        rencache_end_frame();  
        ren_end_frame();
    }

    ren_shutdown();
}