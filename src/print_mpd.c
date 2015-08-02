// vim:ts=8:expandtab
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_version.h>

#include <mpd/client.h>

#include "i3status.h"

#define COPY_CROP(dst, src)                            \
    do {                                               \
        const char *tsrc = src;                        \
        const unsigned int maxcplen = sizeof(dst) - 4; \
        if (src) {                                     \
            strncpy(dst, tsrc, maxcplen + 1);          \
            if (strlen(tsrc) > maxcplen) {             \
                strcpy(dst + maxcplen, "...");         \
            }                                          \
        }                                              \
    } while (0)

void print_mpd(yajl_gen json_gen, char *buffer, const char *host, int port, const char *password, const char *format) {
    const char *walk;
    char *outwalk = buffer;
    static char titlebuf[40];
    static char artistbuf[40];
    static char albumbuf[40];
    static char trackbuf[10];
    static char datebuf[10];

    static struct mpd_connection *conn;
    struct mpd_status *status = NULL;
    enum mpd_state state;
    struct mpd_song *song;

    /* First run */
    if (conn == NULL) {
        conn = mpd_connection_new(host, port, 1500);
        if (conn == NULL) {
            START_COLOR("color_bad");
            outwalk += sprintf(outwalk, "%s", "ERROR");
            goto print_end;
        }

        if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
            START_COLOR("color_bad");
            outwalk += sprintf(outwalk, "%s", "CONNECT ERROR");
            mpd_connection_free(conn);
            conn = NULL;
            goto print_end;
        }

        if (password != NULL && strcmp(password, "") != 0 &&
            !mpd_run_password(conn, password)) {
            START_COLOR("color_bad");
            outwalk += sprintf(outwalk, "%s", "PASS ERROR");
            mpd_connection_free(conn);
            conn = NULL;
            goto print_end;
        }
    }

    if ((status = mpd_run_status(conn))) {
        state = mpd_status_get_state(status);
    }

    if (!status || (state != MPD_STATE_PLAY && state != MPD_STATE_PAUSE)) {
        START_COLOR("color_bad");
        outwalk += sprintf(outwalk, "%s", "Stopped");
        mpd_connection_free(conn);
        conn = NULL;
        goto print_end;
    }

    mpd_status_free(status);

    if (state == MPD_STATE_PLAY)
        START_COLOR("color_good");
    else if (state == MPD_STATE_PAUSE)
        START_COLOR("color_degraded");

    song = mpd_run_current_song(conn);

    COPY_CROP(titlebuf, mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
    COPY_CROP(artistbuf, mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
    COPY_CROP(albumbuf, mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
    COPY_CROP(trackbuf, mpd_song_get_tag(song, MPD_TAG_TRACK, 0));
    COPY_CROP(datebuf, mpd_song_get_tag(song, MPD_TAG_DATE, 0));

    mpd_song_free(song);

    for (walk = format; *walk != '\0'; walk++) {
        if (*walk != '%') {
            *(outwalk++) = *walk;
            continue;
        }

        if (BEGINS_WITH(walk + 1, "title")) {
            if (*titlebuf)
                outwalk += sprintf(outwalk, "%s", titlebuf);
            walk += strlen("title");
        } else if (BEGINS_WITH(walk + 1, "artist")) {
            if (*artistbuf)
                outwalk += sprintf(outwalk, "%s", artistbuf);
            walk += strlen("artist");
        } else if (BEGINS_WITH(walk + 1, "album")) {
            if (*albumbuf)
                outwalk += sprintf(outwalk, "%s", albumbuf);
            walk += strlen("album");
        } else if (BEGINS_WITH(walk + 1, "track")) {
            if (*trackbuf)
                outwalk += sprintf(outwalk, "%s", trackbuf);
            walk += strlen("track");
        } else if (BEGINS_WITH(walk + 1, "date")) {
            if (*datebuf)
                outwalk += sprintf(outwalk, "%s", datebuf);
            walk += strlen("date");
        }
    }

print_end:

    END_COLOR;
    OUTPUT_FULL_TEXT(buffer);

    return;
}
