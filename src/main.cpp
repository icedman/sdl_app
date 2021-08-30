#include "layout.h"
#include "renderer.h"
#include "render_cache.h"
#include "events.h"
#include "tests.h"

#include "text.h"
#include "button.h"

void render_item(layout_item_ptr item)
{
    if (!item->visible) {
        return;
    }

    rencache_state_save();
    rencache_set_clip_rect({
        item->render_rect.x - 1,
        item->render_rect.y - 1,
        item->render_rect.w + 2,
        item->render_rect.h + 2
    });

    bool fill = false;
    float stroke = 1.0f;
    RenColor clr = { item->rgb.r, item->rgb.g, item->rgb.b };
    if (item->view && item->view->is_hovered()) {
        clr = { 150, 0, 150 };
    }
    if (item->view && item->view->is_pressed()) {
        clr = { 255, 0, 0 };
        // fill = true;
        stroke = 1.5f;
    }
    
    // printf("%l %d %d %d %d\n", ct, item->render_rect.x, item->render_rect.y, item->render_rect.w, item->render_rect.h);
    rencache_draw_rect({
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
    rencache_draw_text(NULL, (char*)text.c_str(), item->render_rect.x + 4, item->render_rect.y + 2, { 255, 255, 0},
        false, false, true);

    if (item->view && item->view->is_clicked()) {
        button_view *btn = (button_view*)item->view;
        text_view *txt = (text_view*)btn->text.get();
        printf(">>click %s\n", txt->text.c_str());
    }

    for(auto child : item->children) {
        child->render_rect = child->rect;
        child->render_rect.x += item->render_rect.x + item->scroll_x;
        child->render_rect.y += item->render_rect.y + item->scroll_y;
        render_item(child);
    }

    rencache_state_restore();
}

int main(int argc, char **argv)
{
    ren_init();
    rencache_init();

    // RenImage *tmp = ren_create_image(80,80);
    // ren_begin_frame(tmp);
    // ren_draw_rect({0,0,80,80}, {150,0,150});
    // ren_draw_rect({20,20,20,20}, {255,0,0});
    // ren_end_frame();

    view_item_ptr root_view = test4();
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
            // render_list.clear();
            // layout_render_list(render_list, root); // << this positions items on the screen
        }

        ren_begin_frame();
        rencache_begin_frame(w, h);

        rencache_set_clip_rect({0,0,w,h});
        rencache_draw_rect({x:0,y:0,width:w,height:h}, { 150, 150, 150});
        // rencache_draw_text(font, "Hello World", 20, 20, { 255, 255, 0 });
        // rencache_draw_text(font, "Hello World", 20, 40, { 255, 255, 0 });

        // rencache_draw_image(tmp, {240,240,80,80});

        render_item(root);

        rencache_end_frame();
        ren_end_frame();
    }

    rencache_shutdown();
    ren_shutdown();
}