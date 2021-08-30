#include "layout.h"
#include "renderer.h"
#include "render_cache.h"
#include "events.h"
#include "tests.h"

#include "app.h"
#include "search.h"
#include "statusbar.h"
#include "explorer.h"
#include "editor.h"
#include "operation.h"
#include "keyinput.h"
#include "util.h"

#include "text.h"

#include "theme.h"

static std::map<int, color_info_t> colorMap;

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

int start = 0;
void render_editor(layout_item_ptr item)
{
    // rencache_draw_rect({
    //     item->render_rect.x,
    //     item->render_rect.y,
    //     item->render_rect.w,
    //     item->render_rect.h
    // },
    // { 255, 255, 255 }, true, 1.0f);
    
    int fw, fh;
    ren_get_font_extents(NULL, &fw, &fh, NULL, 1, true);

    editor_ptr editor = app_t::instance()->currentEditor;

    if (start >= editor->document.lastBlock()->lineNumber) {
        start = editor->document.lastBlock()->lineNumber;
    }

    cursor_t cursor = editor->document.cursor();
    block_ptr block = editor->document.blockAtLine(start);

    block_list::iterator it = editor->document.blocks.begin();
    it += start;

    editor->highlight(start, 38);
    
    app_t* app = app_t::instance();
    theme_ptr theme = app->theme;

    int l=0;
    while(it != editor->document.blocks.end() && l<38) {
        block_ptr block = *it++;
        if (!block->data) {
            break;
        }
        struct blockdata_t* blockData = block->data.get();

        std::string text = block->text() + "\n";
        const char *line = text.c_str();

        for(auto &s : blockData->spans) {
            color_info_t fg = colorMap[s.colorIndex];

            std::string span_text = text.substr(s.start, s.length);

            // printf("%s:\n", span_text.c_str());

            rencache_draw_text(NULL, (char*)span_text.c_str(), 
                item->render_rect.x + (s.start * fw),
                item->render_rect.y + (l * fh), { (int)fg.red,(int)fg.green,(int)fg.blue },
                false, false, true);
   
        }

        l++;
    }

}

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
    if (item->view && item->view->is_clicked()) {
        printf(">>click\n");
        start++;
    }

    if (item->view && ((view_item*)item->view)->type == "editor") {
        render_editor(item);
        // return;
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

    for(auto child : item->children) {
        render_item(child);
    }

    rencache_state_restore();
}

int main(int argc, char **argv)
{
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
    // explorer_t::instance()->setRootFromFile(file);

    ren_init();
    rencache_init();

    RenImage *tmp = ren_create_image(80,80);
    ren_begin_frame(tmp);
    ren_draw_rect({0,0,80,80}, {150,0,150});
    ren_draw_rect({20,20,20,20}, {255,0,0});
    ren_end_frame();

    view_item_ptr root_view = test4();
    layout_item_ptr root = root_view->layout();

    view_item_list view_list;
    layout_item_list render_list;
    event_list events;

    RenFont *font = ren_create_font("Fira Code 12");
    // RenFont *font = ren_create_font("Source Code Pro 16");
    ren_set_default_font(font);

    // rencache_show_debug(true);

    color_info_t fg = colorMap[app.bgApp];

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

            ren_begin_frame();
            ren_draw_rect({x:0,y:0,width:w,height:h}, { (int)fg.red,(int)fg.green,(int)fg.blue });
            ren_end_frame();
        }

        ren_begin_frame();
        rencache_begin_frame(w, h);

        rencache_draw_rect({x:0,y:0,width:w,height:h}, { (int)fg.red,(int)fg.green,(int)fg.blue });

        // rencache_draw_rect({x:0,y:0,width:w,height:h}, { 50, 50, 50});
        // rencache_draw_text(font, "Hello World", 20, 20, { 255, 255, 0 });
        // rencache_draw_text(font, "Hello World", 20, 40, { 255, 255, 0 });
        // rencache_draw_image(tmp, {240,240,80,80});

        /*
        for(auto i : render_list) {
            render_item(i);
        }
        */

        render_item(root);

        rencache_end_frame();
        ren_end_frame();
    }

    rencache_shutdown();
    ren_shutdown();

    app.shutdown();
}