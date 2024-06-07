#include "input.h"
#include "screen.h"

#include <string.h>

struct input Input;

void RenderInput(void)
{
    Rect r = Input.r;
    DrawBox(Input.t, &r);
    r.x++;
    r.y++;
    r.w -= 2;
    r.h -= 2;
    if (Input.p != NULL) {
        Point p;
        if (DrawStringExt(stdscr, &r, DS_WRAP | DS_DSEQ, &Input.p[Input.scroll], Input.index - Input.scroll, Input.len - Input.scroll, &p) == 0) {
            SetCursor(r.x + p.x, r.y + p.y);
        } else {
            HideCursor();
        }
    }
    mvprintw(1, 0, "%zu", Input.len);
}

static inline int AcceptInput(const char *s)
{
    const size_t l = strlen(s);
    if (Input.len + l + 1 > Input.cap) {
        Input.cap *= 2;
        Input.cap += l + 1;
        char *const p = Realloc(Input.p, Input.cap);
        if (p == NULL) {
            return -1;
        }
        Input.p = p;
    }
    memmove(&Input.p[Input.index + l],
            &Input.p[Input.index],
            Input.len - Input.index);
    memcpy(&Input.p[Input.index], s, l);
    Input.len += l;
    Input.index += l;
    Input.p[Input.len] = '\0';
    return 0;
}

int InputHandle(int key)
{
    if (key >= 0x20 && key <= 0xff && key != 0x7f) {
        char buf[16];
        const char ch = (char) key;
        const size_t c = GlyphByteCount(&ch);
        buf[0] = ch;
        for (size_t i = 1; i < c; i++) {
            buf[i] = getch();
        }
        buf[c] = '\0';
        return AcceptInput(buf);
    }
    switch (key) {
    case 0x7f:
    case KEY_DC:
        break;
    }
    return 0;
}

