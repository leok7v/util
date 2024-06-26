#pragma once
#include "ut/ut_std.h"

begin_c

// see:
// https://pubs.opengroup.org/onlinepubs/7908799/xsh/dlfcn.h.html

typedef struct {
    // mode:
    int32_t const local;
    int32_t const lazy;
    int32_t const now;
    int32_t const global;
    // "If the value of file is null, dlopen() provides a handle on a global
    //  symbol object." posix
    void* (*open)(const char* filename, int32_t mode);
    void* (*sym)(void* handle, const char* name);
    void  (*close)(void* handle);
    void (*test)(void);
} ut_loader_if;

extern ut_loader_if ut_loader;

end_c
