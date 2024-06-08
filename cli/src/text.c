#include "gfx.h"

#include <ctype.h>
#include <string.h>

int DrawText(WINDOW *win, struct text *text)
{
    Rect r;
    Point cur;
    size_t minsel, maxsel;

    r = text->r;
    if (text->flags & DT_DRAWBOX) {
        DrawBox(text->t, &r);
        r.x++;
        r.y++;
        r.w -= 2;
        r.h -= 2;
    }

    if (r.w <= 0) {
        return 1;
    }

    /* get caret position */
    cur.x = 0;
    cur.y = 0;
    for (size_t i = 0;; ) {
        if (text->flags & DT_ADJIND) {
            if (text->cur.x <= cur.x && text->cur.y == cur.y) {
                text->index = i;
                text->cur.x = cur.x;
                text->flags &= ~DT_ADJIND;
            }
        } else if (i == text->index) {
            text->cur.x = cur.x;
            text->cur.y = cur.y;
        }

        if (i == text->len) {
            if (text->flags & DT_ADJIND) {
                if (text->cur.y >= cur.y) {
                    text->index = i;
                    text->cur.x = cur.x;
                    text->cur.y = cur.y;
                    text->flags &= ~DT_ADJIND;
                }
            }
            break;
        }

        const char ch = text->s[i];
        if (ch == '\n') {
            if (text->flags & DT_ADJIND) {
                if (text->cur.y == cur.y) {
                    text->index = i;
                    text->cur.x = cur.x;
                    text->flags &= ~DT_ADJIND;
                }
            }
            cur.x = 0;
            cur.y++;
            i++;
        } else {
            size_t l;
            int32_t cw;

            if (!IsValidGlyph(&text->s[i], text->len - i)) {
                l = 1;
                cw = 2;
            } else {
                l = GlyphByteCount(&text->s[i]);
                if (ch == '\t') {
                    cw = 0;
                    do {
                        cw++;
                    } while ((cur.x + cw) % 4);
                    l = 1;
                } else if (iscntrl(ch)) {
                    l = 1;
                    cw = 2;
                } else {
                    l = GlyphByteCount(&text->s[i]);
                    cw = GlyphWidth(&text->s[i]);
                }
            }
            if (cur.x + cw > r.w) {
                cur.x = cw;
                cur.y++;
            } else if (cur.x + cw == r.w) {
                cur.x = 0;
                cur.y++;
            } else {
                cur.x += cw;
            }
            i += l;
        }
    }
    text->cur.y = cur.y;
    if (text->flags & DT_ADJVCT) {
        text->flags &= ~DT_ADJVCT;
        text->vct = text->cur.x;
    }
    if (text->flags & DT_ADJSEL) {
        if (!(text->flags & DT_SEL)) {
            text->sel = text->index;
        }
        text->flags &= ~DT_ADJSEL;
        if (text->len > 0) {
            text->flags |= DT_SEL;
        }
    }

    if (text->cur.x < text->scroll.x) {
        text->scroll.x = text->cur.x;
    } else if (text->cur.x >= r.w + text->scroll.x) {
        text->scroll.x = text->cur.x - (r.w - 1);
    }

    if (text->cur.y < text->scroll.y) {
        text->scroll.y = text->cur.y;
    } else if (text->cur.y >= r.h + text->scroll.y) {
        text->scroll.y = text->cur.y - (r.h - 1);
    }

    if (text->flags & DT_SEL) {
        if (text->sel > text->index) {
            minsel = text->index;
            maxsel = text->sel;
        } else {
            minsel = text->sel;
            maxsel = text->index;
        }
    } else {
        minsel = 1;
        maxsel = 0;
    }

    text->cur.x -= text->scroll.x;
    text->cur.y -= text->scroll.y;

    /* render text */
    cur.x = 0;
    cur.y = 0;
    wattr_set(win, 0, 1, NULL);
    cur.x -= text->scroll.x;
    cur.y -= text->scroll.y;
    wmove(win, r.y + cur.y, r.x + cur.x);
    for (size_t i = 0; i < text->len; ) {
        const char ch = text->s[i];
        char b[12];
        size_t l = 0;
        int32_t cw;

        if (cur.y > text->scroll.y + r.h - 1) {
            break;
        }

        wcolor_set(win, 1, NULL);
        if (i >= minsel && i <= maxsel) {
            if (i == text->index) {
                wcolor_set(win, 0, NULL);
            } else {
                wattr_on(win, A_REVERSE, NULL);
            }
        } else {
            wattr_off(win, A_REVERSE, NULL);
        }

        if (ch == '\n') {
            if (cur.y >= 0) {
                wattr_set(win, 0, 0, NULL);
                wprintw(win, "%*s", r.w - cur.x, "");
            }
            cur.x = 0;
            cur.y++;
            wmove(win, r.y + cur.y, r.x);
            i++;
            continue;
        }
        if (ch == '\t') {
            cw = 0;
            do {
                b[l++] = ' ';
                cw++;
            } while ((cur.x + l) % 4);
            b[l] = 0;
            l = 1;
        } else if (iscntrl(ch)) {
            b[0] = '^';
            b[1] = ch == 0x7f ? '?' : ch + 'A' - 1;
            b[2] = 0;
            l = 1;
            cw = 2;
        } else {
            if (!IsValidGlyph(&text->s[i], text->len - i)) {
                l = 1;
                b[0] = '^';
                b[1] = '?';
                b[2] = 0;
                cw = 2;
            } else {
                l = GlyphByteCount(&text->s[i]);
                memcpy(b, &text->s[i], l);
                b[l] = 0;
                cw = GlyphWidth(b);
            }
        }
        if (cur.x + cw > r.w) {
            cur.x = 0;
            cur.y++;
            wmove(win, r.y + cur.y, r.x);
        }
        if (cur.y >= 0) {
            waddstr(win, b);
        }
        cur.x += cw;
        i += l;
    }
    if (text->len == text->index) {
        if (text->len >= minsel && text->len <= maxsel) {
            wcolor_set(win, 0, NULL);
        } else {
            wcolor_set(win, 1, NULL);
        }
        waddch(win, ' ');
    }
    return 0;
}