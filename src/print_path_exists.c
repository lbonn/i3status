#include <stdio.h>
#include <string.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_version.h>
#include <sys/stat.h>
#include "i3status.h"

void print_path_exists(yajl_gen json_gen, char *buffer, const char *title, const char *path, const char *format) {
        const char *walk;
        char *outwalk = buffer;
        struct stat st;
        const bool exists = (stat(path, &st) == 0);

        INSTANCE(path);

        START_COLOR((exists ? "color_good" : "color_bad"));

        for (walk = format; *walk != '\0'; walk++) {
                if (*walk != '%') {
                        *(outwalk++) = *walk;
                        continue;
                }

                if (strncmp(walk+1, "title", strlen("title")) == 0) {
                        outwalk += sprintf(outwalk, "%s", title);
                        walk += strlen("title");
                } else if (strncmp(walk+1, "status", strlen("status")) == 0) {
                        outwalk += sprintf(outwalk, "%s", (exists ? "yes" : "no"));
                        walk += strlen("status");
                }
        }

        END_COLOR;
        OUTPUT_FULL_TEXT(buffer);
}
