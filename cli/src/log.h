#ifndef LOG_H
#define LOG_H

#include "screen.h"

#include <stdio.h>

extern FILE *Log;
extern WINDOW *LogWindow;

int InitLogSystem(void);

#define Log(fmt, ...) \
({ \
    fprintf(Log, "%s:%u: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
\
    wattr_set(LogWindow, A_BOLD, CP_NORMAL, NULL); \
    wprintw(LogWindow, "%s:%u:", __FILE__, __LINE__); \
    wattr_set(LogWindow, A_NORMAL, CP_NORMAL, NULL); \
    wprintw(LogWindow, " " fmt "\n", ##__VA_ARGS__); \
})

#define ErrLog(fmt, ...) \
({ \
    fprintf(Log, "error at %s:%u: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
\
    wcolor_set(LogWindow, CP_ELOG, NULL); \
    wprintw(LogWindow, "error at"); \
    wattr_set(LogWindow, A_BOLD, CP_NORMAL, NULL); \
    wprintw(LogWindow, "%s:%u:", __FILE__, __LINE__); \
    wattr_set(LogWindow, A_NORMAL, CP_NORMAL, NULL); \
    wprintw(LogWindow, " " fmt "\n", ##__VA_ARGS__); \
})

#endif
