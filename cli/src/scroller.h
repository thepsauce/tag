#ifndef SCROLLER_H
#define SCROLLER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

enum sort {
    SORT_NOSORT,

    SORT_ALPHA,
    SORT_TIME,

    /* reverse */
    SORT_RALPHA,
    SORT_RTIME,
};

extern struct scroller {
    size_t *rei;
    size_t num, cap;
    size_t index;
    size_t scroll;
    int height;
    uint8_t *Comps;
    char *filter;
} Scroller;

void RenderScroller(void);
int NotifyScroller(void);
bool MoveScroller(size_t dy, int dir);

#endif
