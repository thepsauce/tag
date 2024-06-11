#include "macros.h"
#include "tag.h"
#include "gfx.h"
#include "scroller.h"
#include "screen.h"

#include <ctype.h>
#include <string.h>
#include <unistd.h>

struct text TagFilter;
struct text FileFilter;
struct text TagEdit;

struct text *Inputs[] = {
    &TagFilter, &FileFilter, &TagEdit
};

int Binds[] = {
    't', 'f', 'e'
};

struct text *Focused;

void InitControls(void)
{
    static const char *titles[] = {
        "Filter tag", "Filter file", "Edit tag"
    };

    Inputs[0]->r = (Rect) { COLS / 2, 0, COLS - COLS / 2, 4 };
    Inputs[1]->r = (Rect) { COLS / 2, 4, COLS - COLS / 2, 4 };
    Inputs[2]->r = (Rect) { COLS / 2, 8, COLS - COLS / 2, LINES - 8 };
    for (size_t i = 0; i < ARRAY_SIZE(Inputs); i++) {
        Inputs[i]->t = (char*) titles[i];
        Inputs[i]->flags |= DT_DRAWBOX | DT_WRAP;
    }
    TagFilter.flags |= DT_SLASH;
    TagEdit.flags |= DT_SLASH;
}

int TextHandle(struct text *text, struct event *ev)
{
    if (ev->type != EV_KEYDOWN) {
        return 0;
    }
    const int key = ev->key;

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
        return 0;
    }

    switch (key) {
        bool sp;
        size_t i;
    case '\x1b':
    case CONTROL('C'):
        Focused = NULL;
        break;
    case '\n':
        if (Focused == &TagEdit && Scroller.num > 0) {
            FileList.files[Scroller.rei[Scroller.index]].tags = StringToComp(TagEdit.s);
        }
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
        break;

    case 0x7f:
    case KEY_DC:
        DeleteGlyphs(text, 1, 1);
        break;
    case KEY_BACKSPACE:
        DeleteGlyphs(text, -1, 1);
        break;
    case KEY_LEFT:
        MoveTextCursor(text, -1, 1);
        break;
    case KEY_RIGHT:
        MoveTextCursor(text, 1, 1);
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

void ControlsHandle(struct event *ev)
{
    switch (ev->type) {
        size_t i;
    case EV_LBUTTONDOWN:
        for (i = 0; i < ARRAY_SIZE(Inputs); i++) {
            if (INSIDE_RECT(Inputs[i]->r, Mouse)) {
                Focused = Inputs[i];
                break;
            }
        }
        if (i == ARRAY_SIZE(Inputs)) {
            Focused = NULL;
        }
        break;
    case EV_KEYDOWN:
        switch (ev->key) {
        case '\t':
            if (Focused == NULL) {
                Focused = Inputs[0];
                return;
            }
            for (i = 0; i < ARRAY_SIZE(Inputs); i++) {
                if (Inputs[i] == Focused) {
                    break;
                }
            }
            if (i + 1 == ARRAY_SIZE(Inputs)) {
                Focused = NULL;
            } else {
                Focused = Inputs[i + 1];
            }
            return;
        }
        break;
    default:
    }
    if (Focused == NULL) {
        switch (ev->type) {
        case EV_KEYDOWN:
            for (size_t i = 0; i < ARRAY_SIZE(Inputs); i++) {
                if (tolower(ev->key) == Binds[i]) {
                    Focused = Inputs[i];
                    break;
                }
            }
            switch (ev->key) {
                case 'k':
                    MoveScroller(1, -1);
                    break;
                case 'j':
                    MoveScroller(1, 1);
                    break;
                case 'g':
                    MoveScroller(SIZE_MAX, -1);
                    break;
                case 'G':
                    MoveScroller(SIZE_MAX, 1);
                    break;
                case '\n': {
                    if (Scroller.num == 0) {
                        break;
                    }
                    struct file *const file = &FileList.files[Scroller.rei[Scroller.index]];
                    if (fork() == 0) {
                        execl("/usr/bin/feh", "feh", GetFilePath(file->name), NULL);
                    }
                    break;
                }
                case 'q':
                    UIRunning = false;
                    break;
            }
            break;
        default:
        }
    } else {
        TextHandle(Focused, ev);
        if (Focused != &TagEdit) {
            NotifyScroller();
        }
    }
}

void RenderControls(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(Inputs); i++) {
        if (Inputs[i] == Focused) {
            Inputs[i]->flags |= DT_TBOLD;
            attr_set(A_NORMAL, CP_FOCUS, NULL);
        } else {
            Inputs[i]->flags &= ~DT_TBOLD;
            attr_set(A_NORMAL, CP_NORMAL, NULL);
        }
        DrawText(stdscr, Inputs[i]);
    }
    if (Focused == NULL) {
        Cursor.x = -1;
    } else {
        Cursor.x = Focused->r.x + 1 + Focused->cur.x;
        Cursor.y = Focused->r.y + 1 + Focused->cur.y;
    }
}

