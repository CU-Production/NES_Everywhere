/*
SPDX-License-Identifier: MIT

agnes
https://http://github.com/kgabis/agnes
Copyright (c) 2022 Krzysztof Gabis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef agnes_h
#define agnes_h

#ifndef AGNES_APP_API_DECL
#if defined(_WIN32) && defined(AGNES_DLL) && defined(AGNES_APP_IMPL)
#define AGNES_APP_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(AGNES_DLL)
#define AGNES_APP_API_DECL __declspec(dllimport)
#else
#define AGNES_APP_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define AGNES_VERSION_MAJOR 0
#define AGNES_VERSION_MINOR 2
#define AGNES_VERSION_PATCH 0

#define AGNES_VERSION_STRING "0.2.0"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    AGNES_SCREEN_WIDTH = 256,
    AGNES_SCREEN_HEIGHT = 240
};

typedef struct {
    bool a;
    bool b;
    bool select;
    bool start;
    bool up;
    bool down;
    bool left;
    bool right;
} agnes_input_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} agnes_color_t;

typedef struct agnes agnes_t;
typedef struct agnes_state agnes_state_t;

AGNES_APP_API_DECL agnes_t* agnes_make(void);
AGNES_APP_API_DECL void agnes_destroy(agnes_t *agn);
AGNES_APP_API_DECL bool agnes_load_ines_data(agnes_t *agnes, void *data, size_t data_size);
AGNES_APP_API_DECL void agnes_set_input(agnes_t *agnes, const agnes_input_t *input_1, const agnes_input_t *input_2);
AGNES_APP_API_DECL void agnes_set_input_u8(agnes_t *agnes, uint8_t input_1, uint8_t input_2);
AGNES_APP_API_DECL size_t agnes_state_size(void);
AGNES_APP_API_DECL void agnes_dump_state(const agnes_t *agnes, agnes_state_t *out_res);
AGNES_APP_API_DECL bool agnes_restore_state(agnes_t *agnes, const agnes_state_t *state);
AGNES_APP_API_DECL bool agnes_tick(agnes_t *agnes, bool *out_new_frame);
AGNES_APP_API_DECL bool agnes_next_frame(agnes_t *agnes);

AGNES_APP_API_DECL agnes_color_t agnes_get_screen_pixel(const agnes_t *agnes, int x, int y);

AGNES_APP_API_DECL void* agnes_read_file(const char *filename, size_t *out_len);
AGNES_APP_API_DECL bool agnes_load_ines_data_from_path(agnes_t *agnes, const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* agnes_h */
