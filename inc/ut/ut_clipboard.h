#pragma once
#include "ut/ut_std.h"

begin_c

typedef struct image_s image_t;

typedef struct {
    errno_t (*put_text)(const char* s);
    errno_t (*get_text)(char* text, int32_t* bytes);
    errno_t (*put_image)(image_t* im); // only for Windows apps
    void (*test)(void);
} ut_clipboard_if;

extern ut_clipboard_if ut_clipboard;

end_c
