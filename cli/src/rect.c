#include "macros.h"
#include "geom.h"

#include <stdlib.h>

bool IsRectEmpty(const Rect *rect)
{
    return rect->w == 0 || rect->h == 0;
}

bool IntersectRect(const Rect *r1, const Rect *r2, Rect *rect)
{
    if (r1->x >= r2->x + r2->w || r1->y >= r2->y + r2->h ||
            r2->x >= r1->x + r1->w || r2->y >= r1->y + r1->h) {
        return false;
    }
    if (rect != NULL) {
        rect->w = MIN(r1->x + r1->w, r2->x + r2->w);
        rect->h = MIN(r1->y + r1->h, r2->y + r2->h);
        rect->x = MAX(r1->x, r2->x);
        rect->y = MAX(r1->y, r2->y);
        rect->w -= rect->x;
        rect->h -= rect->y;
    }
    return true;
}

void RectUnion(const Rect *r1, const Rect *r2, Rect *rect)
{
    rect->w = MAX(r1->x + r1->w, r2->x + r2->w);
    rect->h = MAX(r1->y + r1->h, r2->y + r2->h);
    rect->x = MIN(r1->x, r2->x);
    rect->y = MIN(r1->y, r2->y);
    rect->w -= rect->x;
    rect->h -= rect->y;
}

int CutRect(const Rect *r1, const Rect *r2, Rect *rects)
{
    int n = 0;
    int32_t dx1, dx2;

    /* prioritizing x axis */
    dx1 = r2->x - r1->x;
    dx2 = (r1->x + r1->w) - (r2->x + r2->w);

    const int32_t dy1 = r2->y - r1->y;
    const int32_t dy2 = (r1->y + r1->h) - (r2->y + r2->h);

    if (dx1 >= r1->w || dx2 >= r1->w || dy1 >= r1->h || dy2 >= r1->h) {
        rects[0] = *r1;
        return 1;
    }

    if (dx1 > 0) {
        rects[n].x = r1->x;
        rects[n].w = dx1;
        rects[n].y = r1->y;
        rects[n].h = r1->h;
        n++;
    } else {
        dx1 = 0;
    }
    if (dx2 > 0) {
        rects[n].x = r2->x + r2->w;
        rects[n].w = dx2;
        rects[n].y = r1->y;
        rects[n].h = r1->h;
        n++;
    } else {
        dx2 = 0;
    }

    if (dy1 > 0) {
        rects[n].x = r1->x + dx1;
        rects[n].w = r1->w - dx1 - dx2;
        rects[n].y = r1->y;
        rects[n].h = dy1;
        n++;
    }
    if (dy2 > 0) {
        rects[n].x = r1->x + dx1;
        rects[n].w = r1->w - dx1 - dx2;
        rects[n].y = r2->y + r2->h;
        rects[n].h = dy2;
        n++;
    }
    return n;
}

bool RectContains(const Rect *r, const Point *p)
{
    return p->x >= r->x && p->y >= r->y &&
        p->x < r->x + r->w && p->y < r->y + r->h;
}
