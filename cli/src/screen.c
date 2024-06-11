#include "macros.h"
#include "screen.h"

#include <locale.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

struct mouse Mouse;
Point Cursor = { -1, 0 };

bool UIDirty;
bool UIRunning;

int InitScreen(void)
{
    setlocale(LC_ALL, "");

    initscr();

    curses_trace(TRACE_MAXIMUM);

    start_color();
    init_pair(CP_ALT1, COLOR_RED, 0);
    init_pair(CP_ALT2, COLOR_BLUE, 0);
    init_pair(CP_FOCUS, COLOR_RED, 0);

    noecho();
    raw();
    keypad(stdscr, true);
    nl();
    timeout(80);

    mouseinterval(0);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    printf("\033[?1003h");
    fflush(stdout);

    refresh();
    return 0;
}

void GetEvent(struct event *ev)
{
    static int pl, pc;
    MEVENT me;

    RenderUI();
    do {
        move(Cursor.y, Cursor.x);
        if (Cursor.x >= 0) {
            curs_set(1);
        }
        ev->key = getch();
        curs_set(0);
        if (pl != LINES && pc != COLS) {
            UIDirty = true;
            pl = LINES;
            pc = COLS;
        }
        if (UIDirty) {
            RenderUI();
            UIDirty = false;
        }
    } while (ev->key == ERR || (ev->key == KEY_MOUSE && getmouse(&me) == ERR));

    switch (ev->key) {
    case KEY_RESIZE:
        break;
    case KEY_MOUSE:
        Mouse.px = Mouse.x;
        Mouse.py = Mouse.y;
        Mouse.x = me.x;
        Mouse.y = me.y;

        switch (me.bstate) {
        case BUTTON1_PRESSED:
            Mouse.bt[BT_LEFT] = true;
            ev->type = EV_LBUTTONDOWN;
            break;
        case BUTTON1_RELEASED:
            Mouse.bt[BT_LEFT] = false;
            ev->type = EV_LBUTTONUP;
            break;

        case BUTTON2_PRESSED:
            Mouse.bt[BT_MIDDLE] = true;
            ev->type = EV_MBUTTONDOWN;
            break;
        case BUTTON2_RELEASED:
            Mouse.bt[BT_MIDDLE] = false;
            ev->type = EV_MBUTTONUP;
            break;

        case BUTTON3_PRESSED:
            Mouse.bt[BT_RIGHT] = true;
            ev->type = EV_RBUTTONDOWN;
            break;
        case BUTTON3_RELEASED:
            Mouse.bt[BT_RIGHT] = false;
            ev->type = EV_RBUTTONUP;
            break;

        case BUTTON4_PRESSED:
            ev->type = EV_WHEELUP;
            break;
        case BUTTON5_PRESSED:
            ev->type = EV_WHEELDOWN;
            break;
        default:
            ev->type = EV_MOUSEMOVE;
        }
        break;
    default:
        ev->type = EV_KEYDOWN;
    }
}

void GetDialogRect(Rect *r)
{
    r->x = COLS / 5;
    r->y = LINES / 5;
    r->w = COLS * 3 / 5;
    r->h = LINES * 3 / 5;
}

int Dialog(const char *title, const char *format, ...)
{
    Rect r, tr;
    va_list l;
    char *msg;
    int len;
    const char *opt;
    char o[8];
    unsigned no = 0;
    struct event ev;

    r = (Rect) {
        COLS / 5, LINES / 5,
        COLS * 3 / 5, LINES * 3 / 5
    };
    DrawBox(&r);
    DrawTitle(title, &r);

    /* for format */
    va_start(l, format);
    len = vsnprintf(NULL, 0, format, l);
    va_end(l);
    if (len < 0) {
        return -1;
    }

    va_start(l, format);
    msg = alloca(len);
    vsprintf(msg, format, l);
    tr = (Rect) {
        r.x + r.w / 5, r.y + r.h / 5,
        r.w * 4 / 5, r.h * 4 / 5
    };
    DrawString(&tr, DT_WRAP | DT_UNDERL, msg);

    tr.x = r.x + 3;
    tr.y = r.y + r.h - 1;
    tr.w = r.w - tr.x;
    tr.h = 1;
    while ((opt = va_arg(l, const char*)) != NULL) {
        const char *b;

        b = strchr(opt, '[');
        if (b == NULL || no == ARRAY_SIZE(o)) {
            return -1;
        }
        o[no++] = b[1];
        DrawString(&tr, 0, opt);
        /*const int w = StringWidth(opt, strlen(opt), 0) + 1;
        tr.x += w;
        tr.w -= w;*/
    }
    va_end(l);

    do {
        GetEvent(&ev);
    } while (ev.type != EV_KEYDOWN);
    for (unsigned i = 0; i < no; i++) {
        if (ev.key == tolower(o[i])) {
            return i + 1;
        }
    }
    return 0;
}

void Panic(const char *msg)
{
    Dialog("Panic", "Fatal error: %s\nTrying to quicksave all data...",
            msg, "[O]k", NULL);
    EndScreen();
    exit(EXIT_FAILURE);
}

void Notify(const char *title, const char *msg)
{
    Dialog(title, "An internal process failed: %s", msg, "[O]k", NULL);
}

void *Malloc(size_t size)
{
    void *ptr;

    do {
        ptr = malloc(size);
    } while(size != 0 && ptr == NULL && Dialog("Memory error",
                "Your system returned NULL when trying to "
                "allocate %u bytes of memory, what do you want to do?",
                size, "[T]ry again", "[C]ancel", NULL) == 1);
    return ptr;
}

void *Calloc(size_t nmemb, size_t size)
{
    void *ptr;

    do {
        ptr = calloc(nmemb, size);
    } while(size != 0 && ptr == NULL && Dialog("Memory error",
                "Your system returned NULL when trying to "
                "allocate %u bytes of memory, what do you want to do?",
                size, "[T]ry again", "[C]ancel", NULL) == 1);
    return ptr;
}

void *Realloc(void *ptr, size_t size)
{
    do {
        ptr = realloc(ptr, size);
    } while(size != 0 && ptr == NULL && Dialog("Memory error",
                "Your system returned NULL when trying to "
                "allocate %u bytes of memory, what do you want to do?",
                size, "[T]ry again", "[C]ancel", NULL) == 1);
    return ptr;
}

void *Strdup(const char *s)
{
    char *ptr;

    do {
        ptr = strdup(s);
    } while(ptr == NULL && Dialog("Memory error",
                "Your system returned NULL when trying to "
                "allocate %u bytes of memory, what do you want to do?",
                strlen(s), "[T]ry again", "[C]ancel", NULL) == 1);
    return ptr;
}

WINDOW *Newpad(int nlines, int ncols)
{
    WINDOW *ptr;

    do {
        ptr = newpad(nlines, ncols);
    } while(ptr == NULL && Dialog("Library",
                "Failed creating off screen window of size "
                "%dx%d, what do you want to do?",
                nlines, ncols, "[T]ry again", "[C]ancel", NULL) == 1);
    return ptr;
}

void Free(void *ptr)
{
    free(ptr);
}

void EndScreen(void)
{
    printf("\033[?1003l");
    fflush(stdout);
    endwin();
}
