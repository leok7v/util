#include "ut/ut.h"
#include "ui/ui.h"
#include "ut/ut_win32.h"

#define UI_WM_ANIMATE  (WM_APP + 0x7FFF)
#define UI_WM_OPENING  (WM_APP + 0x7FFE)
#define UI_WM_CLOSING  (WM_APP + 0x7FFD)
#define UI_WM_TAP      (WM_APP + 0x7FFC)
#define UI_WM_DTAP     (WM_APP + 0x7FFB) // fp64_t tap (aka click)
#define UI_WM_PRESS    (WM_APP + 0x7FFA)

bool ui_point_in_rect(const ui_point_t* p, const ui_rect_t* r) {
    return r->x <= p->x && p->x < r->x + r->w &&
           r->y <= p->y && p->y < r->y + r->h;
}

bool ui_intersect_rect(ui_rect_t* i, const ui_rect_t* r0,
                                     const ui_rect_t* r1) {
    ui_rect_t r = {0};
    r.x = maximum(r0->x, r1->x);  // Maximum of left edges
    r.y = maximum(r0->y, r1->y);  // Maximum of top edges
    r.w = minimum(r0->x + r0->w, r1->x + r1->w) - r.x;  // Width of overlap
    r.h = minimum(r0->y + r0->h, r1->y + r1->h) - r.y;  // Height of overlap
    bool b = r.w > 0 && r.h > 0;
    if (!b) {
        r.w = 0;
        r.h = 0;
    }
    if (i != null) { *i = r; }
    return b;
}

extern ui_if ui = {
    .visibility = { // window visibility see ShowWindow link below
        .hide      = SW_HIDE,
        .normal    = SW_SHOWNORMAL,
        .minimize  = SW_SHOWMINIMIZED,
        .maximize  = SW_SHOWMAXIMIZED,
        .normal_na = SW_SHOWNOACTIVATE,
        .show      = SW_SHOW,
        .min_next  = SW_MINIMIZE,
        .min_na    = SW_SHOWMINNOACTIVE,
        .show_na   = SW_SHOWNA,
        .restore   = SW_RESTORE,
        .defau1t   = SW_SHOWDEFAULT,
        .force_min = SW_FORCEMINIMIZE
    },
    .message = {
        .character             = WM_CHAR,
        .key_pressed           = WM_KEYDOWN,
        .key_released          = WM_KEYUP,
        .left_button_pressed   = WM_LBUTTONDOWN,
        .left_button_released  = WM_LBUTTONUP,
        .right_button_pressed  = WM_RBUTTONDOWN,
        .right_button_released = WM_RBUTTONUP,
        .mouse_move            = WM_MOUSEMOVE,
        .mouse_hover           = WM_MOUSEHOVER,
        .left_double_click     = WM_LBUTTONDBLCLK,
        .right_double_click    = WM_RBUTTONDBLCLK,
        .animate               = UI_WM_ANIMATE,
        .opening               = UI_WM_OPENING,
        .closing               = UI_WM_CLOSING,
        .tap                   = UI_WM_TAP,
        .dtap                  = UI_WM_DTAP,
        .press                 = UI_WM_PRESS
    },
    .mouse = {
        .button = {
            .left  = MK_LBUTTON,
            .right = MK_RBUTTON
        }
    },
    .key = {
        .up     = VK_UP,
        .down   = VK_DOWN,
        .left   = VK_LEFT,
        .right  = VK_RIGHT,
        .home   = VK_HOME,
        .end    = VK_END,
        .pageup = VK_PRIOR,
        .pagedw = VK_NEXT,
        .insert = VK_INSERT,
        .del    = VK_DELETE,
        .back   = VK_BACK,
        .escape = VK_ESCAPE,
        .enter  = VK_RETURN,
        .minus  = VK_OEM_MINUS,
        .plus   = VK_OEM_PLUS,
        .f1     = VK_F1,
        .f2     = VK_F2,
        .f3     = VK_F3,
        .f4     = VK_F4,
        .f5     = VK_F5,
        .f6     = VK_F6,
        .f7     = VK_F7,
        .f8     = VK_F8,
        .f9     = VK_F9,
        .f10    = VK_F10,
        .f11    = VK_F11,
        .f12    = VK_F12,
        .f13    = VK_F13,
        .f14    = VK_F14,
        .f15    = VK_F15,
        .f16    = VK_F16,
        .f17    = VK_F17,
        .f18    = VK_F18,
        .f19    = VK_F19,
        .f20    = VK_F20,
        .f21    = VK_F21,
        .f22    = VK_F22,
        .f23    = VK_F23,
        .f24    = VK_F24,
    },
    .folder = {
        .home      = 0, // c:\Users\<username>
        .desktop   = 1,
        .documents = 2,
        .downloads = 3,
        .music     = 4,
        .pictures  = 5,
        .videos    = 6,
        .shared    = 7, // c:\Users\Public
        .bin       = 8, // c:\Program Files
        .data      = 9  // c:\ProgramData
    },
    .point_in_rect = ui_point_in_rect,
    .intersect_rect = ui_intersect_rect
};

// https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow