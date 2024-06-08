#include "macros.h"
#include "screen.h"
#include "gfx.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <wchar.h>

struct brushes Brushes;

struct brush *CreateBrush(const char *name, WINDOW *pat, bool o)
{
    struct brush **p;
    struct brush *br;

    p = Realloc(Brushes.p, sizeof(*p) * (Brushes.n + 1));
    if (p == NULL) {
        return NULL;
    }
    Brushes.p = p;

    br = Malloc(sizeof(*br));
    if (br == NULL) {
        return NULL;
    }
    br->name = Strdup(name);
    if (br->name == NULL) {
        Free(br);
        return NULL;
    }
    br->pat = pat;
    br->o = o;

    Brushes.p[Brushes.n++] = br;
    return br;
}

void DeleteBrush(struct brush *br)
{
    Free(br->name);
    delwin(br->pat);
    for (uint32_t i = 0; i < Brushes.n; i++) {
        if (Brushes.p[i] == br) {
            Brushes.n--;
            memmove(&Brushes.p[i], &Brushes.p[i + 1],
                    sizeof(*Brushes.p) * (Brushes.n - i));
            break;
        }
    }
}

void SetBrushColor(struct brush *br, int color)
{
    int my, mx;

    WINDOW *const pat = br->pat;
    getmaxyx(pat, my, mx);
    for (int y = 0; y < my; y++) {
        for (int x = 0; x < mx; x++) {
            const chtype ch = mvwinch(pat, y, x);
            mvwchgat(pat, y, x, 1, ch & A_ATTRIBUTES, color, NULL);
        }
    }
}

void DrawCircle(WINDOW *win, const cchar_t *cc, int x, int y, int d)
{
    int r;
    int xi, yi;
    int p;

    r = d >> 1;
    mvwadd_wch(win, y, x + r, cc);
    if (r == 0) {
        return;
    }

    mvwadd_wch(win, y - r, x, cc);
    mvwadd_wch(win, y + r, x, cc);
    mvwadd_wch(win, y, x - r, cc);
    xi = r;
    yi = 0;
    p = 1 - r;
    while (xi > yi) {
        yi++;
        if (p <= 0) {
            p += 2 * yi + 1;
        } else {
            xi--;
            p += 2 * (yi - xi) + 1;
        }

        if (xi < yi) {
            break;
        }

        mvwadd_wch(win, y + yi, x + xi, cc);
        mvwadd_wch(win, y + yi, x - xi, cc);
        mvwadd_wch(win, y - yi, x + xi, cc);
        mvwadd_wch(win, y - yi, x - xi, cc);

        if (xi != yi) {
            mvwadd_wch(win, y + xi, x + yi, cc);
            mvwadd_wch(win, y + xi, x - yi, cc);
            mvwadd_wch(win, y - xi, x + yi, cc);
            mvwadd_wch(win, y - xi, x - yi, cc);
        }
    }
}

void FillCircle(WINDOW *win, const cchar_t *cc, int x, int y, int d)
{
    int r;
    int xi, yi;
    int p;

    r = d >> 1;
    if (r == 0) {
        mvwadd_wch(win, y, x + r, cc);
        return;
    }

    mvwhline_set(win, y, x - r, cc, 2 * r + 1);
    xi = r;
    yi = 0;
    p = 1 - r;
    while (xi > yi) {
        yi++;
        if (p <= 0) {
            p += 2 * yi + 1;
        } else {
            xi--;
            p += 2 * (yi - xi) + 1;
        }

        if (xi < yi) {
            break;
        }

        mvwhline_set(win, y - yi, x - xi, cc, 2 * xi + 1);
        mvwhline_set(win, y + yi, x - xi, cc, 2 * xi + 1);

        if (xi != yi) {
            mvwhline_set(win, y - xi, x - yi, cc, 2 * yi + 1);
            mvwhline_set(win, y + xi, x - yi, cc, 2 * yi + 1);
        }
    }
}

void DrawLine(WINDOW *win, const cchar_t *cc, int width, int x1, int y1, int x2, int y2)
{
    int err, err2;
    int dx, dy, sx, sy;

    if (x1 < x2) {
        dx = x2 - x1;
        sx = 1;
    } else {
        dx = x1 - x2;
        sx = -1;
    }

    if (y1 < y2) {
        dy = y1 - y2;
        sy = 1;
    } else {
        dy = y2 - y1;
        sy = -1;
    }

    err = dx + dy;
    FillCircle(win, cc, x1, y1, width);
    while (x1 != x2 || y1 != y2) {
        DrawCircle(win, cc, x1, y1, width);
        err2 = 2 * err;
        if (err2 >= dy) {
            err += dy;
            x1 += sx;
        }
        if (err2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void DrawPatLine(WINDOW *pat, WINDOW *win, int x1, int y1, int x2, int y2, bool o)
{
    int err, err2;
    int dx, dy, sx, sy;
    int mx, my;

    if (x1 < x2) {
        dx = x2 - x1;
        sx = 1;
    } else {
        dx = x1 - x2;
        sx = -1;
    }

    if (y1 < y2) {
        dy = y1 - y2;
        sy = 1;
    } else {
        dy = y2 - y1;
        sy = -1;
    }

    err = dx + dy;
    getmaxyx(pat, my, mx);
    while (copywin(pat, win, 0, 0, y1 - my / 2, x1 - mx / 2,
                y1 + my - 1 - my / 2, x1 + mx - 1 - mx / 2, o),
            x1 != x2 || y1 != y2) {
        err2 = 2 * err;
        if (err2 >= dy) {
            err += dy;
            x1 += sx;
        }
        if (err2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void DrawStroke(const struct stroke *st, WINDOW *win, int dx, int dy)
{
    for (uint32_t i = 0; i < st->n; i += 2) {
        Point p1, p2;

        p1 = st->p[i];
        if (i + 1 == st->n) {
            p2 = p1;
        } else {
            p2 = st->p[i + 1];
        }
        const struct brush *const br = st->br;
        DrawPatLine(br->pat, win, p1.x + dx, p1.y + dy, p2.x + dx, p2.y + dy, br->o);
    }
}

int AddPoint(struct stroke *st, Point pt)
{
    Point *p;
    int dx, dy;

    p = Realloc(st->p, sizeof(*st->p) * (st->n + 1));
    if (p == NULL) {
        return -1;
    }
    st->p = p;

    st->p[st->n++] = pt;

    if (st->n == 1) {
        st->rect.x = pt.x;
        st->rect.y = pt.y;
        st->rect.w = 0;
        st->rect.h = 0;
        return 0;
    }

    dx = pt.x - st->rect.x;
    if (dx < 0) {
        st->rect.x += dx;
        st->rect.w -= dx;
    }
    dy = pt.y - st->rect.y;
    if (dy < 0) {
        st->rect.y += dy;
        st->rect.h -= dy;
    }

    dx = pt.x - st->rect.x - st->rect.w;
    if (dx > 0) {
        st->rect.w += dx;
    }
    dy = pt.y - st->rect.y - st->rect.h;
    if (dy > 0) {
        st->rect.h += dy;
    }
    return 0;
}

bool IsValidGlyph(const char *s, size_t l)
{
	if ((unsigned) s[0] <= 0x7f) {
		return true;
	}
	if ((s[0] & 0xe0) == 0xc0) {
		return l >= 2 && (s[1] & 0xc0) == 0x80;
	}
	if ((s[0] & 0xf0) == 0xe0) {
		return l >= 3 &&
			(s[1] & 0xc0) == 0x80 &&
			(s[2] & 0xc0) == 0x80;
    }
	if ((s[0] & 0xf8) == 0xf0) {
		return l >= 4 &&
			(s[1] & 0xc0) == 0x80 &&
			(s[2] & 0xc0) == 0x80 &&
			(s[3] & 0xc0) == 0x80;
	}
	return false;
}

size_t GlyphByteCount(const char *s)
{
    size_t c = 0;
    char u;

    u = *s;
    if (!(u & 0x80)) {
        return 1;
    }
    for (c = 0; u & 0x80; c++) {
        u <<= 1;
    }
    return c;
}

size_t GlyphByteCountR(const char *s)
{
    size_t c;

    for (c = 1; (s[-c] & 0xc0) == 0x80; c++) {
        (void) 0;
    }
    return c;
}

int wcwidth(wchar_t c);

int GlyphWidth(const char *s)
{
    wchar_t wc;

    mbstowcs(&wc, s, 1);
    return wcwidth(wc);
}

void DrawBox(const char *title, Rect *r)
{
    /* draw border */
    mvhline(r->y, r->x, ACS_HLINE, r->w);
    mvhline(r->y + r->h - 1, r->x, ACS_HLINE, r->w);
    mvvline(r->y, r->x, ACS_VLINE, r->h);
    mvvline(r->y, r->x + r->w - 1, ACS_VLINE, r->h);
    /* draw corners */
    mvaddch(r->y, r->x, ACS_ULCORNER);
    mvaddch(r->y, r->x + r->w - 1, ACS_URCORNER);
    mvaddch(r->y + r->h - 1, r->x, ACS_LLCORNER);
    mvaddch(r->y + r->h - 1, r->x + r->w - 1, ACS_LRCORNER);
    /* draw title */
    if (title != NULL) {
        Rect t;
        char b[strlen(title) + 2];

        t.x = r->x + 2;
        t.y = r->y;
        t.w = r->x + r->w - r->x - 3;
        t.h = 1;
        sprintf(b, " %s ", title);
        DrawString(&t, DT_TERM, b);
    }
    /* erase content */
    for (int y = 1; y < r->h - 1; y++) {
        for (int x = 1; x < r->w - 1; x++) {
            mvaddch(r->y + y, r->x + x, ' ');
        }
    }
}
