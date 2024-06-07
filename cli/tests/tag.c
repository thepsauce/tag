#include "macros.h"
#include "tag.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>

int main(void)
{
    struct file_list f;

    chdir("/home/steves/ext");
    if (InitAllDirectory() != 0) {
        printf("no all directory in here\n");
        return -1;
    }

    if (CacheTags() == 0) {
        for (size_t i = 0; i < TagList.num; i++) {
            printf("%s ", TagList.tags[i]);
        }
        printf("\n");
    } else {
        return -1;
    }

    if (CacheFiles(&f) == 0) {
        for (size_t i = 0; i < f.num; i++) {
            GetTags(&f.files[i]);
        }

        uint8_t *comp = NULL;
        size_t tags[3];
        tags[0] = TAG_ID("all");
        tags[1] = TAG_ID("neko");
        tags[2] = TAG_ID("swimsuit");
        for (size_t i = 0; i < ARRAY_SIZE(tags); i++) {
            comp = AddComposition(comp, tags[i]);
        }
        for (size_t i = 0; i < f.num; i++) {
            struct file *const file = &f.files[i];
            if (COMP_CONTAINS(file->tags, comp)) {
                printf("%s\n", file->name);
            }
        }
        printf("%zu\n", TagList.compn);
    }
    return 0;
}

