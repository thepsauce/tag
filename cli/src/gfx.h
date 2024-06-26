#ifndef GFX_H
#define GFX_H

#include "geom.h"

#define NCURSES_WIDECHAR 1
#define TRACE
#include <ncurses.h>

/*
 * utf8 helper functions
 */

bool IsValidGlyph(const char *s, size_t l);

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

size_t ConvertDistance(const char *s, size_t n, size_t i, int dir, size_t a);

/* intepret an _ as highlighting the next letter */
#define DT_UNDERL 0x1
/* when x would overflow the width, wrap to the next line */
#define DT_WRAP 0x2
/* adjust the cursor based on the x, y position */
#define DT_ADJIND 0x10
/* start a selection */
#define DT_ADJSEL 0x20
/* draw a selection (set by DT_ADJSEL) */
#define DT_SEL 0x40
/* draw a box around the text */
#define DT_DRAWBOX 0x80
/* if the title should be bold */
#define DT_TBOLD 0x100
/* if the title should intepret _ */
#define DT_TUNDERL 0x200
/* if the vertical column tracker should be updated */
#define DT_ADJVCT 0x400
/* enables special interpretation of / (alternating color) */
#define DT_SLASH 0x800

struct text {
    int flags;
    /* position and size of the text */
    Rect r;
    /* title of the box (DT_DRAWBOX) */
    char *t;
    /* textual data */
    char *s;
    /* index within the text */
    size_t index;
    /* length and capacity of text */
    size_t len, cap;
    /* start of selection */
    size_t sel;
    /* vertical column tracking */
    int32_t vct;
    /* horizontal/vertical scrolling (DT_SCROLL) */
    Point scroll;
    /* cursor position */
    Point cur;
    /* additional attributes (DT_TERM) */
    attr_t *attrs;
    int *colors;
};

int InsertText(struct text *text, const char *s);
size_t DeleteBytes(struct text *text, int dir, size_t amount);
size_t DeleteGlyphs(struct text *text, int dir, size_t amount);
size_t MoveTextCursor(struct text *text, int dir, size_t amount);

int DrawText(WINDOW *win, struct text *text);
#define DrawString(_r, f, _s) \
({ \
    struct text _t; \
    memset(&_t, 0, sizeof(_t)); \
    _t.r = *(_r); \
    _t.s = (char*) (_s); \
    _t.flags = (f); \
    _t.len = strlen(_t.s); \
    DrawText(stdscr, &_t); \
})

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
void DrawBox(const Rect *r);
void DrawTitle(const char *title, const Rect *r);

#endif
