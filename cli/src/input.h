#ifndef INPUT_H
#define INPUT_H

#include "gfx.h"

#include <stddef.h>

extern struct input {
    Rect r;
    const char *t;
    char *p;
    size_t len, cap;
    size_t index;
    size_t scroll;
} Input;

int InputHandle(int key);
void RenderInput(void);

#endif
