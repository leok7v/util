/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"

static bool debug_layout = false;

const char* title = "Sample5";

// font scale:
static const fp64_t fs[] = {0.5, 0.75, 1.0, 1.25, 1.50, 1.75, 2.0};
// font scale index
static int32_t fx = 2; // fs[2] == 1.0

static ui_fm_t mf; // mono font
static ui_fm_t pf; // proportional font

static ui_edit_t edit0;
static ui_edit_t edit1;
static ui_edit_t edit2;
static ui_edit_t* edit[] = { &edit0, &edit1, &edit2 };

static int32_t focused(void) {
    // ui_app.focus can point to a button, thus see which edit
    // control was focused last
    int32_t ix = -1;
    for (int32_t i = 0; i < countof(edit) && ix < 0; i++) {
        if (ui_app.focus == &edit[i]->view) { ix = i; }
        if (edit[i]->focused) { ix = i; }
    }
    static int32_t last_ix = -1;
    if (ix < 0) { ix = last_ix; }
    last_ix = ix;
    return ix;
}

static void focus_back_to_edit(void) {
    const int32_t ix = focused();
    if (ix >= 0) {
        ui_app.focus = &edit[ix]->view; // return focus where it was
    }
}

static void scaled_fonts(void) {
    assert(0 <= fx && fx < countof(fs));
    if (mf.font != null) { ui_gdi.delete_font(mf.font); }
    int32_t h = (int32_t)(ui_gdi.font_height(ui_app.fonts.mono.font) * fs[fx] + 0.5);
    ui_gdi.update_fm(&mf, ui_gdi.font(ui_app.fonts.mono.font, h, -1));
    if (pf.font != null) { ui_gdi.delete_font(pf.font); }
    h = (int32_t)(ui_gdi.font_height(ui_app.fonts.regular.font) * fs[fx] + 0.5);
    ui_gdi.update_fm(&pf, ui_gdi.font(ui_app.fonts.regular.font, h, -1));
}

ui_button_on_click(full_screen, "&Full Screen", 7.5, {
    ui_app.full_screen(!ui_app.is_full_screen);
});

ui_button_on_click(quit, "&Quit", 7.5, { ui_app.close(); });

ui_button_on_click(fuzz, "Fu&zz", 7.5, {
    int32_t ix = focused();
    if (ix >= 0) {
        edit[ix]->fuzz(edit[ix]);
        fuzz->pressed = edit[ix]->fuzzer != null;
        focus_back_to_edit();
    }
});

ui_toggle_on_switch(ro, "&Read Only", 7.5, {
    int32_t ix = focused();
    if (ix >= 0) {
        edit[ix]->ro = ro->pressed;
//      traceln("edit[%d].readonly: %d", ix, edit[ix]->ro);
        focus_back_to_edit();
    }
});

ui_toggle_on_switch(mono, "&Mono", 7.5, {
    int32_t ix = focused();
    if (ix >= 0) {
        edit[ix]->set_font(edit[ix], mono->pressed ? &mf : &pf);
        focus_back_to_edit();
    } else {
        mono->pressed = !mono->pressed;
    }
});

ui_toggle_on_switch(sl, "&Single Line", 7.5, {
    int32_t ix = focused();
    if (ix == 2) {
        sl->pressed = true; // always single line
    } else if (0 <= ix && ix < 2) {
        ui_edit_t* e = edit[ix];
        e->sle = sl->pressed;
//      traceln("edit[%d].multiline: %d", ix, e->multiline);
        if (e->sle) {
            e->select_all(e);
            e->paste(e, "Hello World! Single Line Edit", -1);
        }
        ui_app.request_layout();
        focus_back_to_edit();
    }
});

static void font_plus(void) {
    if (fx < countof(fs) - 1) {
        fx++;
        scaled_fonts();
        ui_app.request_layout();
    }
}

static void font_minus(void) {
    if (fx > 0) {
        fx--;
        scaled_fonts();
        ui_app.request_layout();
    }
}

static void font_reset(void) {
    fx = 2;
    scaled_fonts();
    ui_app.request_layout();
}

ui_button_on_click(fp, "Font Ctrl+", 7.5, { font_plus(); });

ui_button_on_click(fm, "Font Ctrl-", 7.5, { font_minus(); });

static ui_label_t label = ui_label(0.0, "...");

static ui_view_t left   = ui_view(list);
static ui_view_t right  = ui_view(list);
static ui_view_t bottom = ui_view(container);

static void set_text(int32_t ix) {
    static char last[128];
    strprintf(label.text, "%d:%d %d:%d %dx%d\n"
        "scroll %03d:%03d",
        edit[ix]->selection[0].pn, edit[ix]->selection[0].gp,
        edit[ix]->selection[1].pn, edit[ix]->selection[1].gp,
        edit[ix]->view.w, edit[ix]->view.h,
        edit[ix]->scroll.pn, edit[ix]->scroll.rn);
    if (0) {
        traceln("%d:%d %d:%d %dx%d scroll %03d:%03d",
            edit[ix]->selection[0].pn, edit[ix]->selection[0].gp,
            edit[ix]->selection[1].pn, edit[ix]->selection[1].gp,
            edit[ix]->view.w, edit[ix]->view.h,
            edit[ix]->scroll.pn, edit[ix]->scroll.rn);
    }
    // can be called before text.ui initialized
    if (!strequ(last, label.text)) {
        ui_view.invalidate(&label);
    }
    strprintf(last, "%s", label.text);
}

static void after_paint(void) {
    // because of blinking caret paint is called frequently
    int32_t ix = focused();
    if (ix >= 0) {
        bool fuzzing = edit[ix]->fuzzer != null;
        if (fuzz.pressed != fuzzing) {
            fuzz.pressed = fuzzing;
            ui_view.invalidate(&fuzz);
        }
        set_text(ix);
    }
}

static void paint_frames(ui_view_t* v) {
    ui_view_for_each(v, c, { paint_frames(c); });
    ui_color_t fc[] = {
        ui_colors.red, ui_colors.green, ui_colors.blue, ui_colors.red,
        ui_colors.yellow, ui_colors.cyan, ui_colors.magenta
    };
    static int32_t color;
    ui_gdi.push(v->x, v->y + v->h - v->fm->em.h);
    ui_gdi.frame_with(v->x, v->y, v->w, v->h, fc[color]);
    ui_color_t c = ui_gdi.set_text_color(fc[color]);
    ui_gdi.print("%s", v->text);
    ui_gdi.set_text_color(c);
    ui_gdi.pop();
    color = (color + 1) % countof(fc);
}

static void null_paint(ui_view_t* v) {
    ui_view_for_each(v, c, { null_paint(c); });
    if (v != ui_app.view) {
        v->paint = null;
    }
}

static void paint(ui_view_t* v) {
//  traceln("");
    if (debug_layout) { null_paint(v); }
    ui_gdi.set_brush(ui_gdi.brush_color);
    ui_gdi.set_brush_color(ui_colors.black);
    ui_gdi.fill(0, 0, v->w, v->h);
    int32_t ix = focused();
    for (int32_t i = 0; i < countof(edit); i++) {
        ui_view_t* e = &edit[i]->view;
        ui_color_t c = edit[i]->ro ?
            ui_colors.tone_red : ui_colors.btn_hover_highlight;
        ui_gdi.frame_with(e->x - 1, e->y - 1, e->w + 2, e->h + 2,
            i == ix ? c : ui_colors.dkgray4);
    }
    after_paint();
    if (debug_layout) { paint_frames(v); }
    if (ix >= 0) {
        ro.pressed = edit[ix]->ro;
        sl.pressed = edit[ix]->sle;
        mono.pressed = edit[ix]->view.fm->font == mf.font;
    }
}

static void open_file(const char* pathname) {
    char* file = null;
    int64_t bytes = 0;
    if (ut_mem.map_ro(pathname, &file, &bytes) == 0) {
        if (0 < bytes && bytes <= INT64_MAX) {
            edit[0]->select_all(edit[0]);
            edit[0]->paste(edit[0], file, (int32_t)bytes);
            ui_edit_pg_t start = { .pn = 0, .gp = 0 };
            edit[0]->move(edit[0], start);
        }
        ut_mem.unmap(file, bytes);
    } else {
        ui_app.toast(5.3, "\nFailed to open file \"%s\".\n%s\n",
                  pathname, ut_str.error(ut_runtime.err()));
    }
}

static void every_100ms(void) {
//  traceln("");
    static ui_view_t* last;
    if (last != ui_app.focus) { ui_app.request_redraw(); }
    last = ui_app.focus;
}

// limiting vertical height of SLE to 3 lines of text:

static void edit2_before_measure(ui_view_t* v) { // _3_lines_sle
    // UX design decision:
    // 3 vertical visible runs SLE is friendlier in UX term
    // than not implemented horizontal scroll.
    assert(v == &edit[2]->view);
//  traceln("WxH: %dx%d <- r/o button", ro.view.w, ro.view.h);
    v->w = ro.w; // r/o button
}

static void edit2_after_measure(ui_view_t* v) {
//  traceln("WxH: %dx%d (%dx%d) em: %d lines: %d",
//          edit[2]->view.w, edit[2]->view.h,
//          edit[2]->width, edit[2]->height,
//          edit[2]->view.fm->em.h, edit[2]->view.h / edit[2]->view.fm->em.h);
    int32_t max_lines = edit[2]->focused ? 3 : 1;
    if (v->h > v->fm->em.h * max_lines) {
        v->h = v->fm->em.h * max_lines;
    }
}

static void key_pressed(ui_view_t* unused(view), int64_t key) {
    if (ui_app.has_focus() && key == ui.key.escape) { ui_app.close(); }
    int32_t ix = focused();
    if (key == ui.key.f5) {
        if (ix >= 0) {
            ui_edit_t* e = edit[ix];
            if (ui_app.ctrl && ui_app.shift && e->fuzzer == null) {
                e->fuzz(e); // start on Ctrl+Shift+F5
            } else if (e->fuzzer != null) {
                e->fuzz(e); // stop on F5
            }
        }
    }
    if (ui_app.ctrl) {
        if (key == ui.key.minus) {
            font_minus();
        } else if (key == ui.key.plus) {
            font_plus();
        } else if (key == '0') {
            font_reset();
        }
    }
    if (ix >= 0) { set_text(ix); }
}

static void edit_enter(ui_edit_t* e) {
    assert(e->sle);
    if (!ui_app.shift) { // ignore shift ENRER:
        traceln("text: %.*s", e->para[0].bytes, e->para[0].text);
    }
}

// see edit.test.c

void ui_edit_init_with_lorem_ipsum(ui_edit_t* e);
void ui_edit_fuzz(ui_edit_t* e);
void ui_edit_next_fuzz(ui_edit_t* e);

static void opened(void) {
//  ui_app.view->measure     = measure;
//  ui_app.view->layout      = layout;
    ui_app.view->paint       = paint;
    ui_app.view->key_pressed = key_pressed;
    scaled_fonts();
    label.fm = &ui_app.fonts.mono;
    strprintf(fuzz.hint, "Ctrl+Shift+F5 to start / F5 to stop Fuzzing");
    for (int32_t i = 0; i < countof(edit); i++) {
        ui_edit_init(edit[i]);
        edit[i]->view.padding = (ui_gaps_t){0.5, 0.5, 0.5, 0.5};
        edit[i]->view.max_w = ui.infinity;
        if (i < 2) { edit[i]->view.max_h = ui.infinity; }
        edit[i]->view.fm = &pf;
        edit[i]->fuzz = ui_edit_fuzz;
        edit[i]->next_fuzz = ui_edit_next_fuzz;
        ui_edit_init_with_lorem_ipsum(edit[i]);
    }
    ui_app.focus = &edit[0]->view;
    ui_app.every_100ms = every_100ms;
    set_text(0); // need to be two lines for measure
    // edit[2] is SLE:
    edit[2]->view.before_measure = edit2_before_measure;
    edit[2]->view.after_measure  = edit2_after_measure;
    edit[2]->sle = true;
    edit[2]->select_all(edit[2]);
    edit[2]->paste(edit[2], "Single line edit", -1);
    edit[2]->enter = edit_enter;
    static ui_view_t span    = ui_view(span);
    static ui_view_t spacer1 = ui_view(spacer);
    static ui_view_t spacer2 = ui_view(spacer);
    ui_view.add(ui_app.view,
        ui_view.add(&span,
            ui_view.add(&left,
                &edit0,
                &edit1,
                &label,
            null),
            &spacer1,
            ui_view.add(&right,
                &full_screen,
                &quit,
                &fuzz,
                &fp,
                &fm,
                &mono,
                &sl,
                &ro,
                &edit2,
                &spacer2,
            null),
        null),
    null);
    span.max_h = ui.infinity;
    label.align = ui.align.left;
    edit2.view.align = ui.align.left;
    left.max_w = ui.infinity;
    left.max_h = ui.infinity;
    right.max_h = ui.infinity;
    if (ut_args.c > 1) { open_file(ut_args.v[1]); }
}

static void init(void) {
    ui_app.title = title;
    ui_app.opened = opened;
}

ui_app_t ui_app = {
    .class_name = "sample5",
    .dark_mode = true,
    .init = init,
    .window_sizing = {
        .min_w =  3.0f,
        .min_h =  2.5f,
        .ini_w =  4.0f,
        .ini_h =  5.0f
    }
};

