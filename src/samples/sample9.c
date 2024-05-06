/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"
#include "i18n.h"

#define TITLE "Sample9"

static void init(void);

app_t app = {
    .class_name = "sample9",
    .init = init,
    .window_sizing = {
        .min_w =  9.0f,
        .min_h =  5.5f,
        .ini_w = 10.0f,
        .ini_h =  6.0f
    }
};

static ui_point_t em;
static int32_t panel_border = 1;
static int32_t frame_border = 1;

static ui_image_t image;
static uint32_t pixels[1024][1024];

static fp64_t zoom = 0.5;
static fp64_t sx = 0.25; // [0..1]
static fp64_t sy = 0.25; // [0..1]

static struct { fp64_t x; fp64_t y; } stack[52];
static int top = 1; // because it is already zoomed in once above

static ui_slider_t zoomer;

static ui_label_t toast_filename = ui_label(0.0, "filename placeholder");

static ui_label_t label_single_line = ui_label(0.0, "Mandelbrot Explorer");

static ui_label_t label_multiline = ui_label(19.0,
    "Click inside or +/- to zoom;\n"
    "right mouse click to zoom out;\n"
    "use touchpad or keyboard "
    ui_glyph_leftwards_white_arrow ui_glyph_upwards_white_arrow
    ui_glyph_downwards_white_arrow ui_glyph_rightwards_white_arrow
    " to pan");

static ui_label_t about = ui_label(34.56f,
    "\nClick inside Mandelbrot Julia Set fractal to zoom in into interesting "
    "areas. Right mouse click to zoom out.\n"
    "Use Win + Shift + S to take a screenshot of something "
    "beautiful that caught your eye."
    "\n\n"
    "This sample also a showcase of controls like toggle, message box, "
    "tooltips, clipboard copy, full screen switching, open file "
    "dialog and on-the-fly locale switching for simple and possibly "
    "incorrect Simplified Chinese localization."
    "\n\n"
    "Press ESC or click the "
    ui_glyph_multiplication_sign
    " button in right top corner "
    "to dismiss this message or just wait - it will disappear by "
    "itself in 10 seconds.\n");

#ifdef SAMPLE9_USE_STATIC_UI_VIEW_MACROS

static_ui_mbx(mbx, // message box
    "\"Pneumonoultramicroscopicsilicovolcanoconiosis\"\n"
    "is it the longest English language word or not?", {
    traceln("option=%d", option); // -1 or index of { "&Yes", "&No" }
}, "&Yes", "&No");

#else

static void mbx_callback(ui_mbx_t* unused(mbx), int32_t option) {
    static const char* name[] = { "Cancel", "Yes", "No" };
    traceln("option: %d \"%s\"", option, name[option + 1]);
}

static ui_mbx_t mbx = ui_mbx( // message box
    "\"Pneumonoultramicroscopicsilicovolcanoconiosis\"\n"
    "is it the longest English language word or not?", mbx_callback,
    "&Yes", "&No");

#endif

static const char* filter[] = {
    "All Files", "*",
    "Image Files", ".png;.jpg",
    "Text Files", ".txt;.doc;.ini",
    "Executables", ".exe"
};

static void open_file(ui_button_t* unused(b)) {
    const char* fn = app.open_filename(
        app.known_folder(ui.folder.home),
        filter, countof(filter)); //  all files filer: null, 0
    if (fn[0] != 0) {
        strprintf(toast_filename.view.text, "%s", fn);
        traceln("%s", fn);
        app.show_toast(&toast_filename.view, 2.0);
    }
}

static_ui_button(button_open_file, "&Open", 7.5, {
    open_file(button_open_file);
});

static void flip_full_screen(ui_button_t* b) {
    b->pressed = !b->pressed;
    app.full_screen(b->pressed);
    if (b->pressed) {
        app.toast(1.75, "Press ESC to exit full screen");
    }
}

static_ui_button(button_full_screen, ui_glyph_two_joined_squares, 1, {
    flip_full_screen(button_full_screen);
});

static void flip_locale(ui_button_t* b) {
    b->pressed = !b->pressed;
    nls.set_locale(b->pressed ? "zh-CN" : "en-US");
    // TODO: label_multiline does not get localized automatically.
    //       investigate why... (comment out next line and put some printfs around)
    ui_view.localize(&label_multiline.view);
    app.layout(); // because center panel layout changed
}

static ui_button_t button_locale = ui_button(
    ui_glyph_kanji_onna_female "A", 1, flip_locale);

static_ui_button(button_about, "&About", 7.5, {
    app.show_toast(&about.view, 10.0);
});

static_ui_button(button_mbx, "&Message Box", 7.5, {
    app.show_toast(&mbx.view, 0);
});

#ifdef SAMPLE9_USE_STATIC_UI_VIEW_MACROS

// ui_toggle label can include "___" for "ON ": "OFF" state
static_ui_toggle(scroll, "Scroll &Direction:", 0, {});

#else

static ui_toggle_t scroll = ui_toggle("Scroll &Direction:",
                                      /* min_w_em: */ 0.0f,
                                      /* callback:*/ null);

#endif

static ui_view_t panel_top    = ui_view(container);
static ui_view_t panel_bottom = ui_view(container);
static ui_view_t panel_center = ui_view(container);
static ui_view_t panel_right  = ui_view(container);

static void panel_paint(ui_view_t* view) {
    if (view->color == ui_color_transparent) {
        view->color = app.view->color;
    }
    ui_gdi.push(view->x, view->y);
    ui_gdi.set_clip(view->x, view->y, view->w, view->h);
    ui_gdi.fill_with(view->x, view->y, view->w, view->h, ui_colors.dkgray1);
    ui_pen_t p = ui_gdi.create_pen(ui_colors.dkgray4, panel_border);
    ui_gdi.set_pen(p);
    ui_gdi.move_to(view->x, view->y);
    if (view == &panel_right) {
        ui_gdi.line(view->x + view->w, view->y);
        ui_gdi.line(view->x + view->w, view->y + view->h);
        ui_gdi.line(view->x, view->y + view->h);
        ui_gdi.line(view->x, view->y);
    } else if (view == &panel_top || view == &panel_bottom) {
        ui_gdi.line(view->x, view->y + view->h);
        ui_gdi.line(view->x + view->w, view->y + view->h);
        ui_gdi.move_to(view->x + view->w, view->y);
        ui_gdi.line(view->x, view->y);
    } else {
        assert(view == &panel_center);
        ui_gdi.line(view->x, view->y + view->h);
    }
    int32_t x = view->x + panel_border + ut_max(1, em.x / 8);
    int32_t y = view->y + panel_border + ut_max(1, em.y / 4);
    ui_pen_t s = ui_gdi.set_colored_pen(view->color);
    ui_gdi.set_brush(ui_gdi.brush_hollow);
    ui_gdi.rounded(x, y, em.x * 12, em.y, ut_max(1, em.y / 4), ut_max(1, em.y / 4));
    ui_gdi.set_pen(s);
    ui_color_t color = ui_gdi.set_text_color(view->color);
    ui_gdi.x = view->x + panel_border + ut_max(1, em.x / 2);
    ui_gdi.y = view->y + panel_border + ut_max(1, em.y / 4);
    ui_gdi.text("%d,%d %dx%d %s", view->x, view->y, view->w, view->h, view->text);
    ui_gdi.set_text_color(color);
    ui_gdi.set_clip(0, 0, 0, 0);
    ui_gdi.delete_pen(p);
    ui_gdi.pop();
}

static void right_layout(ui_view_t* view) {
    int x = view->x + em.x;
    int y = view->y + em.y * 2;
    ui_view_for_each(view, c, {
        c->x = x;
        c->y = y;
        y += c->h + em.y / 2;
    });
}

static void text_after(ui_view_t* view, const char* format, ...) {
    ui_gdi.x = view->x + view->w + view->em.x;
    ui_gdi.y = view->y;
    va_list va;
    va_start(va, format);
    ui_gdi.vtextln(format, va);
    va_end(va);
}

static void right_paint(ui_view_t* view) {
    panel_paint(view);
    ui_gdi.push(view->x, view->y);
    ui_gdi.set_clip(view->x, view->y, view->w, view->h);
    ui_gdi.x = button_locale.x + button_locale.w + em.x;
    ui_gdi.y = button_locale.y;
    ui_gdi.println("&Locale %s", button_locale.pressed ? "zh-CN" : "en-US");
    ui_gdi.x = button_full_screen.x + button_full_screen.w + em.x;
    ui_gdi.y = button_full_screen.y;
    ui_gdi.println(app.is_full_screen ? nls.str("Restore from &Full Screen") :
        nls.str("&Full Screen"));
    ui_gdi.x = label_multiline.view.x;
    ui_gdi.y = label_multiline.view.y + label_multiline.view.h + ut_max(1, em.y / 4);
    ui_gdi.textln(nls.str("Proportional"));
    ui_gdi.println(nls.str("Monospaced"));
    ui_font_t font = ui_gdi.set_font(app.fonts.H1);
    ui_gdi.textln("H1 %s", nls.str("Header"));
    ui_gdi.set_font(app.fonts.H2); ui_gdi.textln("H2 %s", nls.str("Header"));
    ui_gdi.set_font(app.fonts.H3); ui_gdi.textln("H3 %s", nls.str("Header"));
    ui_gdi.set_font(font);
    ui_gdi.println("%s %dx%d", nls.str("Client area"), app.crc.w, app.crc.h);
    ui_gdi.println("%s %dx%d", nls.str("Window"), app.wrc.w, app.wrc.h);
    ui_gdi.println("%s %dx%d", nls.str("Monitor"), app.mrc.w, app.mrc.h);
    ui_gdi.println("%s %d %d", nls.str("Left Top"), app.wrc.x, app.wrc.y);
    ui_gdi.println("%s %d %d", nls.str("Mouse"), app.mouse.x, app.mouse.y);
    ui_gdi.println("%d x paint()", app.paint_count);
    ui_gdi.println("%.1fms (%s %.1f %s %.1f)", app.paint_time * 1000.0,
        nls.str("max"), app.paint_max * 1000.0, nls.str("avg"),
        app.paint_avg * 1000.0);
    text_after(&zoomer.view, "%.16f", zoom);
    text_after(&scroll, "%s", scroll.pressed ?
        nls.str("Natural") : nls.str("Reverse"));
    ui_gdi.set_clip(0, 0, 0, 0);
    ui_gdi.pop();
}

static void center_paint(ui_view_t* view) {
    ui_gdi.set_clip(view->x, view->y, view->w, view->h);
    ui_gdi.fill_with(view->x, view->y, view->w, view->h, ui_colors.black);
    int x = (view->w - image.w) / 2;
    int y = (view->h - image.h) / 2;
//  ui_gdi.alpha_blend(view->x + x, view->y + y, image.w, image.h, &image, 0.5);
    ui_gdi.draw_image(view->x + x, view->y + y, image.w, image.h, &image);
    ui_gdi.set_clip(0, 0, 0, 0);
}

static void measure(ui_view_t* view) {
    ui_point_t em_mono = ui_gdi.get_em(app.fonts.mono);
    em = ui_gdi.get_em(app.fonts.regular);
    view->em = em;
    panel_border = ut_max(1, em_mono.y / 4);
    frame_border = ut_max(1, em_mono.y / 8);
    assert(panel_border > 0 && frame_border > 0);
    const int32_t w = app.width;
    const int32_t h = app.height;
    // measure ui elements
    panel_top.w = (int32_t)(0.70 * w);
    panel_top.h = em.y * 2;
    panel_bottom.w = panel_top.w;
    panel_bottom.h = em.y * 2;
    panel_right.w = w - panel_bottom.w;
    panel_right.h = h;
    panel_center.w = panel_bottom.w;
    panel_center.h = h - panel_bottom.h - panel_top.h;
}

static void layout(ui_view_t* unused(view)) {
    assert(view->em.x > 0 && view->em.y > 0);
    const int32_t h = app.height;
    panel_top.x = 0;
    panel_top.y = 0;
    panel_bottom.x = 0;
    panel_bottom.y = h - panel_bottom.h;
    panel_right.x = panel_bottom.w;
    panel_right.y = 0;
    panel_center.x = 0;
    panel_center.y = panel_top.h;
}

static void refresh(void);

static void zoom_out(void) {
    assert(top > 0);
    top--;
    sx = stack[top].x;
    sy = stack[top].y;
    zoom *= 2;
}

static void zoom_in(int x, int y) {
    assert(top < countof(stack));
    stack[top].x = sx;
    stack[top].y = sy;
    top++;
    zoom /= 2;
    sx += zoom * x / image.w;
    sy += zoom * y / image.h;
}

static void mouse(ui_view_t* unused(view), int32_t m, int64_t unused(flags)) {
    int mx = app.mouse.x - panel_center.x;
    int my = app.mouse.y - panel_center.y;
    if (0 <= mx && mx < panel_center.w && 0 <= my && my < panel_center.h) {
        int x = app.mouse.x - (panel_center.w - image.w) / 2 - panel_center.x;
        int y = app.mouse.y - (panel_center.h - image.h) / 2 - panel_center.y;
        if (0 <= x && x < image.w && 0 <= y && y < image.h) {
            if (m == ui.message.right_button_pressed) {
                if (zoom < 1) { zoom_out(); refresh(); }
            } else if (m == ui.message.left_button_pressed) {
                if (top < countof(stack)) { zoom_in(x, y); refresh(); }
            }
        }
        app.redraw();
    }
}

static void zoomer_callback(ui_view_t* v) {
    ui_slider_t* slider = (ui_slider_t*)v;
    fp64_t z = 1;
    for (int i = 0; i < slider->value; i++) { z /= 2; }
    while (zoom > z) { zoom_in(image.w / 2, image.h / 2); }
    while (zoom < z) { zoom_out(); }
    refresh();
}

static void mouse_wheel(ui_view_t* unused, int32_t dx, int32_t dy) {
    (void)unused;
    if (!scroll.pressed) { dy = -dy; }
    if (!scroll.pressed) { dx = -dx; }
    sx = sx + zoom * dx / image.w;
    sy = sy + zoom * dy / image.h;
    refresh();
}

static void character(ui_view_t* view, const char* utf8) {
    char ch = utf8[0];
    if (ch == 'q' || ch == 'Q') {
        app.close();
    } else if (ch == 033 && app.is_full_screen) {
        button_full_screen_callback(&button_full_screen);
    } else if (ch == '+' || ch == '=') {
        zoom /= 2; refresh();
    } else if (ch == '-' || ch == '_') {
        zoom = ut_min(zoom * 2, 1.0); refresh();
    } else if (ch == '<' || ch == ',') {
        mouse_wheel(view, +image.w / 8, 0);
    } else if (ch == '>' || ch == '.') {
        mouse_wheel(view, -image.w / 8, 0);
    } else if (ch == 3) { // Ctrl+C
        ut_clipboard.put_image(&image);
    }
}

static void keyboard(ui_view_t* view, int64_t vk) {
    if (vk == ui.key.up) {
        mouse_wheel(view, 0, +image.h / 8);
    } else if (vk == ui.key.down) {
        mouse_wheel(view, 0, -image.h / 8);
    } else if (vk == ui.key.left) {
        mouse_wheel(view, +image.w / 8, 0);
    } else if (vk == ui.key.right) {
        mouse_wheel(view, -image.w / 8, 0);
    }
}

static void init_panel(ui_view_t* panel, const char* text, ui_color_t color,
        void (*paint)(ui_view_t*)) {
    strprintf(panel->text, "%s", text);
    panel->color = color;
    panel->paint = paint;
}

static void opened(void) {
    app.view->measure     = measure;
    app.view->layout      = layout;
    app.view->character   = character;
    app.view->key_pressed = keyboard; // virtual_keys
    app.view->mouse_wheel = mouse_wheel;

    panel_center.mouse = mouse;

    int n = countof(pixels);
    static_assert(sizeof(pixels[0][0]) == 4, "4 bytes per pixel");
    static_assert(countof(pixels) == countof(pixels[0]), "square");
    ui_gdi.image_init(&image, n, n, (int)sizeof(pixels[0][0]), (uint8_t*)pixels);
    init_panel(&panel_top,    "top",    ui_colors.orange, panel_paint);
    init_panel(&panel_center, "center", ui_colors.off_white, center_paint);
    init_panel(&panel_bottom, "bottom", ui_colors.tone_blue, panel_paint);
    init_panel(&panel_right,  "right",  ui_colors.tone_green, right_paint);
    panel_right.layout = right_layout;
    label_single_line.highlight = true;
    label_multiline.highlight = true;
    label_multiline.hovered = true;
    strprintf(label_multiline.view.hint, "%s",
        "Ctrl+C or Right Mouse click to copy text to clipboard");
    strprintf(label_multiline.view.text, "%s",
        nls.string(str_help, label_multiline.view.text));
    toast_filename.view.font = &app.fonts.H1;
    about.view.font = &app.fonts.H3;
    button_locale.shortcut = 'l';
    button_full_screen.shortcut = 'f';
#ifdef SAMPLE9_USE_STATIC_UI_VIEW_MACROS
    ui_slider_init(&zoomer, "Zoom: 1 / (2^%d)", 7.0, 0, countof(stack) - 1,
        zoomer_callback);
#else
    zoomer = (ui_slider_t)ui_slider("Zoom: 1 / (2^%d)", 7.0, 0, countof(stack) - 1,
        zoomer_callback);
#endif
    strcopy(button_mbx.hint, "Show Yes/No message box");
    strcopy(button_about.hint, "Show About message box");
    ui_view.add(&panel_right,
        &button_locale,
        &button_full_screen,
        &zoomer,
        &scroll,
        &button_open_file,
        &button_about,
        &button_mbx,
        &label_single_line,
        &label_multiline,
        null
    );
    ui_view.add(app.view,
        &panel_top,
        &panel_center,
        &panel_right,
        &panel_bottom,
        null);
    refresh();
}

static void init(void) {
    app.title = TITLE;
    app.opened = opened;
}

static fp64_t scale0to1(int v, int range, fp64_t sh, fp64_t zm) {
    return sh + zm * v / range;
}

static fp64_t into(fp64_t v, fp64_t lo, fp64_t hi) {
    assert(0 <= v && v <= 1);
    return v * (hi - lo) + lo;
}

static void mandelbrot(ui_image_t* im) {
    for (int r = 0; r < im->h; r++) {
        fp64_t y0 = into(scale0to1(r, im->h, sy, zoom), -1.12, 1.12);
        for (int c = 0; c < im->w; c++) {
            fp64_t x0 = into(scale0to1(c, im->w, sx, zoom), -2.00, 0.47);
            fp64_t x = 0;
            fp64_t y = 0;
            int iteration = 0;
            enum { max_iteration = 100 };
            while (x* x + y * y <= 2 * 2 && iteration < max_iteration) {
                fp64_t t = x * x - y * y + x0;
                y = 2 * x * y + y0;
                x = t;
                iteration++;
            }
            static ui_color_t palette[16] = {
                ui_rgb( 66,  30,  15),  ui_rgb( 25,   7,  26),
                ui_rgb(  9,   1,  47),  ui_rgb(  4,   4,  73),
                ui_rgb(  0,   7, 100),  ui_rgb( 12,  44, 138),
                ui_rgb( 24,  82, 177),  ui_rgb( 57, 125, 209),
                ui_rgb(134, 181, 229),  ui_rgb(211, 236, 248),
                ui_rgb(241, 233, 191),  ui_rgb(248, 201,  95),
                ui_rgb(255, 170,   0),  ui_rgb(204, 128,   0),
                ui_rgb(153,  87,   0),  ui_rgb(106,  52,   3)
            };
            ui_color_t color = palette[iteration % countof(palette)];
            uint8_t* px = &((uint8_t*)im->pixels)[r * im->w * 4 + c * 4];
            px[3] = 0xFF;
            px[0] = (color >> 16) & 0xFF;
            px[1] = (color >>  8) & 0xFF;
            px[2] = (color >>  0) & 0xFF;
        }
    }
}

static void refresh(void) {
    if (sx < 0) { sx = 0; }
    if (sx > 1 - zoom) { sx = 1 - zoom; }
    if (sy < 0) { sy = 0; }
    if (sy > 1 - zoom) { sy = 1 - zoom; }
    if (zoom == 1) { sx = 0; sy = 0; }
    zoomer.value = 0;
    fp64_t z = 1;
    while (z != zoom) { zoomer.value++; z /= 2; }
    zoomer.value = ut_min(zoomer.value, zoomer.value_max);
    mandelbrot(&image);
    app.redraw();
}

