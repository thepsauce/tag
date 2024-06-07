#ifndef GEOM_H
#define GEOM_H

#include <stdint.h>
#include <stdbool.h>

typedef struct point {
    int32_t x, y;
} Point;

typedef struct rect {
    int32_t x, y, w, h;
} Rect;

/*
 * checks if the width or height is 0
 */
bool IsRectEmpty(const Rect *rect);

bool RectContains(const Rect *r, const Point *p);

/*
 * sets rect to one that perfectly surrounds both r1 and r2
 */
void RectUnion(const Rect *r1, const Rect *r2, Rect *rect);

/*
 * cuts r1 into pieces by removing r2 from it,
 * rects must have enough space to hold 4 rectangles
 */
int CutRect(const Rect *r1, const Rect *r2, Rect *rects);

/*
 * returns if two rectangles intersect and store the intersection
 * in rect (leaves rect untouched if they do not intersect)
 *
 * rect can be null
 */
bool IntersectRect(const Rect *r1, const Rect *r2, Rect *rect);

#endif
