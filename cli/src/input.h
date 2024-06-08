#ifndef INPUT_H
#define INPUT_H

#include "gfx.h"

#include <stddef.h>

extern struct input {
    struct text text;
} Input;

int InputHandle(int key);
void RenderInput(void);

#endif
