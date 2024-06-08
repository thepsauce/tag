#include "input.h"
#include "screen.h"

#include <string.h>

struct input Input;

void RenderInput(void)
{
    struct text *const text = &Input.text;
    text->flags |= DT_DRAWBOX;
    DrawText(stdscr, text);
    SetCursor(text->r.x + 1 + text->cur.x, text->r.y + 1 + text->cur.y);
    mvprintw(1, 0, "%zu", text->len);
}

static inline int AcceptInput(const char *s)
{
    struct text *const text = &Input.text;
    const size_t l = strlen(s);
    if (text->len + l + 1 > text->cap) {
        text->cap *= 2;
        text->cap += l + 1;
        char *const p = Realloc(text->s, text->cap);
        if (p == NULL) {
            return -1;
        }
        text->s = p;
    }
    memmove(&text->s[text->index + l],
            &text->s[text->index],
            text->len - text->index);
    memcpy(&text->s[text->index], s, l);
    text->len += l;
    text->index += l;
    text->s[text->len] = '\0';
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

