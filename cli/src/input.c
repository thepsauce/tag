#include "macros.h"
#include "input.h"
#include "gfx.h"
#include "scroller.h"
#include "screen.h"

#include <ctype.h>
#include <string.h>

struct input Input;

void RenderInput(void)
{
    struct text *const text = &Input.text;
    text->flags |= DT_DRAWBOX;
    DrawText(stdscr, text);
    SetCursor(text->r.x + 1 + text->cur.x - text->scroll.x,
            text->r.y + 1 + text->cur.y - text->scroll.y);
}

int InputHandle(int key)
{
    struct text *const text = &Input.text;

    if (key >= 0x20 && key <= 0xff && key != 0x7f) {
        char buf[16];
        const char ch = (char) key;
        const size_t c = GlyphByteCount(&ch);
        buf[0] = ch;
        for (size_t i = 1; i < c; i++) {
            buf[i] = getch();
        }
        buf[c] = '\0';
        if (InsertText(text, buf) == ERR) {
            return ERR;
        }
        return NotifyScroller();
    }

    switch (key) {
        bool sp;
        size_t i;
    case '\n':
    case '\x1b':
    case CONTROL('C'):
        Input.shown = false;
        break;
    case CONTROL('W'):
        if (text->index == text->len) {
            break;
        }
        sp = !!isspace(text->s[text->index]);
        for (text->index++; text->index < text->len; text->index++) {
            if (sp != !!isspace(text->s[text->index])) {
                break;
            }
        }
        text->flags |= DT_ADJVCT;
        break;
    case CONTROL('B'):
        if (text->index == 0) {
            break;
        }
        sp = !!isspace(text->s[--text->index]);
        for (; text->index > 0; text->index--) {
            if (sp != !!isspace(text->s[text->index - 1])) {
                break;
            }
        }
        text->flags |= DT_ADJVCT;
        break;
    case CONTROL('D'):
    case CONTROL('H'):
        if (text->index == 0) {
            break;
        }
        i = text->index;
        sp = !!isspace(text->s[--i]);
        for (; i > 0; i--) {
            if (sp != !!isspace(text->s[i - 1])) {
                break;
            }
        }
        DeleteBytes(text, -1, text->index - i);
        NotifyScroller();
        break;
    case CONTROL('E'):
        if (text->index == text->len) {
            break;
        }
        i = text->index;
        sp = !!isspace(text->s[i]);
        for (i++; i < text->len; i++) {
            if (sp != !!isspace(text->s[i])) {
                break;
            }
        }
        DeleteBytes(text, 1, i - text->index);
        NotifyScroller();
        break;

    case 0x7f:
    case KEY_DC:
        DeleteGlyphs(text, 1, 1);
        NotifyScroller();
        break;
    case KEY_BACKSPACE:
        DeleteGlyphs(text, -1, 1);
        NotifyScroller();
        break;
    case KEY_LEFT:
        MoveTextCursor(text, -1, 1);
        NotifyScroller();
        break;
    case KEY_RIGHT:
        MoveTextCursor(text, 1, 1);
        NotifyScroller();
        break;
    case KEY_UP:
    case KEY_DOWN:
        text->cur.x = text->vct;
        text->cur.y += key == KEY_UP ? -1 : 1;
        text->flags |= DT_ADJIND;
        break;
    case KEY_HOME:
    case KEY_END:
        text->cur.x = key == KEY_HOME ? 0 : INT32_MAX;
        text->flags |= DT_ADJVCT;
        text->flags |= DT_ADJIND;
        break;
    }
    return OK;
}
