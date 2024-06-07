#ifndef GFX_H
#define GFX_H

#include "geom.h"

#define NCURSES_WIDECHAR 1
#define TRACE
#include <ncurses.h>

#define DS_DSEQ 0x1
#define DS_WRAP 0x2
#define DS_DRY 0x4

/*
 * utf8 helper functions, const char *s is always a valid utf8 string
 */

/*
 * get the count of bytes of the next glyph
 */
size_t GlyphByteCount(const char *s);

/*
 * same but in reverse
 *
 * so s + GlyphByteCountR(s + GlyphByteCount(s))
 * is the same as s assuming *s != '\0'
 */
size_t GlyphByteCountR(const char *s);

/*
 * get the width of the next glyph (usually 1 or 2)
 */
int GlyphWidth(const char *s);

struct fitting {
    char *s, *e;
    int w;
};

/*
 * returns if the string fits but always updates fit by placing the start
 * and end of the string that fit into it and the occupied width, this function
 * stops upon a '\n'
 */
bool StringFitting(const char *s, size_t n, int max, int flags, struct fitting *fit);

/*
 * for a null terminated string
 */
bool StringFitting0(const char *s, int max, int flags, struct fitting *fit);

/*
 * get the width of the entire string. If the DS_DSEQ flag is enabled,
 * then the interpretation of dollar sequences is enabled
 */
int StringWidth(const char *s, size_t n, int flags);

/*
 * also takes DS_WRAP as flag to wrap text
 */
int DrawStringExt(WINDOW *win, const Rect *r, int flags, const char *s, size_t ind, size_t len, Point *cur, ...);
#define DrawString(r, f, s, ...) \
({ \
    const char * const _s = (s); \
    Point _p; \
    DrawStringExt(stdscr, (r), (f), _s, 0, strlen(_s), &_p, #__VA_ARGS__); \
})

/* --- --- --- --- ---
 * Dollar sequences
 * $r - Change attribute to red
 * $a - Attribute from va_args
 * $0 - Reset attributes
 *
 * More options instead of r:
 * $r red, $g green, $b blue, $c cyan, $y yellow, $m magenta,
 * $d bold, $i italic
 * --- --- --- --- ---
 */

/*
 * a paint brush
 */
struct brush {
    /* whether this brush should be shown in the brush window */
    bool v;
    /* brush identifier */
    char *name;
    /* brush pattern */
    WINDOW *pat;
    /* whether the brush pattern should overlap (not copy spaces) */
    bool o;
};

extern struct brushes {
    /* whether the brush window is visible */
    bool v;
    /* screen coordinates of the brush window */
    int x, y, w, h;
    /* selected brush */
    unsigned sel;
    /* list of all brushes */
    struct brush **p;
    /* number of brushes */
    unsigned n;
} Brushes;

/*
 * creates a brush with given name and pattern and adds it
 * to the brush list
 */
struct brush *CreateBrush(const char *name, WINDOW *pat, bool o);
void DeleteBrush(struct brush *br);
void SetBrushColor(struct brush *br, int color);

/*
 * a paint brush stroke
 */
struct stroke {
    struct brush *br;
    /* points on the path of the stroke */
    Point *p;
    /* number of those points */
    uint32_t n;
    /* the previous region before the brush stroke */
    WINDOW *cache;
    /* region this brush stroke encloses */
    Rect rect;
};

/*
 * draw a line but each point is drawn using the given pattern, this pattern
 * is drawn centered
 */
void DrawPatLine(WINDOW *pat, WINDOW *win, int x1, int y1, int x2, int y2, bool o);

/*
 * draw a brush stroke into given window while offsetting it by (dx, dy)
 */
void DrawStroke(const struct stroke *st, WINDOW *win, int dx, int dy);

/*
 * simply add a point but also update rect
 */
int AddPoint(struct stroke *st, Point pt);

/*
 * draw the outline of a circle where (x, y) is the center and
 * d is the diameter
 */
void DrawCircle(WINDOW *win, const cchar_t *cc, int x, int y, int d);

/*
 * same as DrawCircle but in addition it draws horizontal lines to
 * fill the circle
 */
void FillCircle(WINDOW *win, const cchar_t *cc, int x, int y, int d);

/*
 * draws a line with given width, it uses FillCircle for the initial point and
 * then DrawCircle for the rest
 */
void DrawLine(WINDOW *win, const cchar_t *cc, int width, int x1, int y1, int x2, int y2);

/*
 * uses special characters to draw a box around the inside of given rectangle
 * coordinates. The title is drawn at the top left and the inside of the
 * rectangle is cleared
 */
void DrawBox(const char *title, Rect *r);

#endif
