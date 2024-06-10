#ifndef SCREEN_H
#define SCREEN_H

#include "gfx.h"

#include <stdlib.h>

struct event {
    int key;
    enum {
        EV_DRAW,
        EV_LBUTTONDOWN,
        EV_LBUTTONUP,
        EV_RBUTTONDOWN,
        EV_RBUTTONUP,
        EV_MBUTTONDOWN,
        EV_MBUTTONUP,
        EV_WHEELUP,
        EV_WHEELDOWN,
        EV_MOUSEMOVE,
        EV_KEYDOWN,
        EV_MAX
    } type;
};

enum button {
    BT_LEFT,
    BT_MIDDLE,
    BT_RIGHT,
    BT_MAX
};

extern struct mouse {
    /* true if the button is pressed */
    bool bt[BT_MAX];
    /* previous mouse position */
    int px, py;
    /* current mouse position */
    int x, y;
} Mouse;

extern Point Cursor;

void SetCursor(int x, int y);
void HideCursor(void);

extern bool UIDirty;
extern bool UIRunning;

enum color_pair {
    CP_NORMAL,
    CP_ALT1,
    CP_ALT2,
};

int InitScreen(void);

/*
 * run the main loop of the ui
 *
 * screen must have been initialised
 */
int RunUI(void);

void RenderUI(void);

void GetEvent(struct event *ev);

/*
 * get a well fitting rect for a dialog window
 */
void GetDialogRect(Rect *r);

/*
 * exits the program but tries to quicksave everything
 */
void Panic(const char *msg);

/*
 * shows a dialog window with given title and format message,
 * the variable arguments have a dual purpose:
 * 1. Format arguments (e.g. for %s, %d...)
 * 2. User options in the form ...[.]...
 *
 * The variable arguments MUST end with NULL!
 *
 * It returns the option the user chose or -1 if an invalid option
 * was chosen or there was no option to choose from.
 *
 * Examples:
 * Dialog("Simple", "This is a simple dialog", NULL)
 * Dialog("Quit", "Do you really want to quit?", "[Y]es", "[N]o", NULL)
 * Dialog("Error", "Failed allocation %zu bytes", size, "[O]k", NULL)
 */
int Dialog(const char *title, const char *format, ...);

/*
 * Dialog(title, "An internal process failed: %s", msg, "[O]k", NULL)
 */
void Notify(const char *title, const char *msg);

/*
 * These have the exact same behaviour as their lowercase variant but
 * show a dialog to the user when an error occurs
 */
void *Malloc(size_t size);
void *Calloc(size_t nmemb, size_t size);
void *Realloc(void *ptr, size_t size);
void *Strdup(const char *s);
void Free(void *ptr);
WINDOW *Newpad(int nlines, int ncols);

void EndScreen(void);

#endif
