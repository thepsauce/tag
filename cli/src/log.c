#include "log.h"

#include <stdlib.h>

FILE *Log;
WINDOW *LogWindow;

int InitLogSystem(void)
{
    Log = fopen("log.txt", "w");
    if (Log == NULL) {
        return -1;
    }

    LogWindow = newpad(80, 1024);
    if (LogWindow == NULL) {
        return -1;
    }
    return 0;
}

