#include "layout.h"
#include "renderer.h"
#include "render_cache.h"
#include "events.h"
#include "tests.h"

int main(int argc, char **argv)
{
    ren_init();

    layout_item_ptr root = layout2();

    layout_item_list render_list;
    event_list events;

    // RenFont *font = ren_create_font("Fira Code 14");
    RenFont *font = ren_create_font("Source Code Pro 16");

    // rencache_show_debug(true);

    int pw, ph;

    while(ren_is_running()) {
        ren_listen_events(&events);

        ren_begin_frame();
        rencache_begin_frame();
        
        int w, h;
        ren_get_size(&w, &h);

        if (pw != w || ph != h) {
            pw = w;
            ph = h;

            layout_run(root, { 0, 0, w - 20, h - 20 });
            render_list.clear();
            layout_render_list(render_list, root);
        }

        rencache_draw_rect({x:0,y:0,width:w,height:h}, { 150, 150, 150});
        rencache_draw_text(font, "Hello World", 20, 20, { 255, 255, 0 });
        rencache_draw_text(font, "Hello World", 20, 40, { 255, 255, 0 });

        for(auto i : render_list) {
            // printf("%l %d %d %d %d\n", ct, i->render_rect.x, i->render_rect.y, i->render_rect.w, i->render_rect.h);
            rencache_draw_rect({
                i->render_rect.x,
                i->render_rect.y,
                i->render_rect.w,
                i->render_rect.h
            },
            { i->rgb.r, i->rgb.g, i->rgb.b }, false, 1.0f);
            // break;

            std::string text = i->name;
            rencache_draw_text(font, (char*)text.c_str(), i->render_rect.x + 4, i->render_rect.y + 2, { 255, 255, 0 });
        }

        rencache_end_frame();  
        ren_end_frame();
    }

    ren_shutdown();
}