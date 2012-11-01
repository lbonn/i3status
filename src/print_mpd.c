// vim:ts=8:expandtab
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_version.h>

#include <mpd/client.h>

#include "i3status.h"

#define COPY_CROP(dest, src) \
        do { \
                const char *tmp = src;\
                if (src) { \
                        strncpy(dest, tmp, sizeof(dest)-4); \
                        if (strlen(tmp) > sizeof(dest)-4) { \
                                strcpy(dest + sizeof(dest)-4, "..."); \
                        } \
                } \
        } while(0)

void print_mpd(yajl_gen json_gen, char *buffer, const char *host, int port, const char *password, const char *format) {
        const char *walk;
        char *outwalk = buffer;
        char titlebuf[40];
        char artistbuf[40];
        char albumbuf[40];
        char trackbuf[10];
        char datebuf[10];

        memset(titlebuf, '\0', sizeof(titlebuf));
        memset(artistbuf, '\0', sizeof(artistbuf));
        memset(albumbuf, '\0', sizeof(albumbuf));
        memset(trackbuf, '\0', sizeof(trackbuf));
        memset(datebuf, '\0', sizeof(datebuf));

        static struct mpd_connection *conn;
        struct mpd_status *status = NULL;
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

        status = mpd_run_status(conn);

        if ((status != NULL) &&
                        (mpd_status_get_state(status) == MPD_STATE_PLAY ||
                         mpd_status_get_state(status) == MPD_STATE_PAUSE)) {

                if (mpd_status_get_state(status) == MPD_STATE_PLAY)
                        START_COLOR("color_good");
                else if (mpd_status_get_state(status) == MPD_STATE_PAUSE)
                        START_COLOR("color_degraded");

                mpd_status_free(status);
                song = mpd_run_current_song(conn);

                COPY_CROP(titlebuf, mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
                COPY_CROP(artistbuf, mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
                COPY_CROP(albumbuf, mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
                COPY_CROP(trackbuf, mpd_song_get_tag(song, MPD_TAG_TRACK, 0));
                COPY_CROP(datebuf, mpd_song_get_tag(song, MPD_TAG_DATE, 0));

                for (walk = format; *walk != '\0'; walk++) {
                        if (*walk != '%') {
                                *(outwalk++) = *walk;
                                continue;
                        }

                        if (BEGINS_WITH(walk+1, "title")) {
                                if (*titlebuf)
                                        outwalk += sprintf(outwalk, "%s", titlebuf);
                                walk += strlen("title");
                        }
                        else if (BEGINS_WITH(walk+1, "artist")) {
                                if (*artistbuf)
                                        outwalk += sprintf(outwalk, "%s", artistbuf);
                                walk += strlen("artist");
                        }
                        else if (BEGINS_WITH(walk+1, "album")) {
                                if (*albumbuf)
                                        outwalk += sprintf(outwalk, "%s", albumbuf);
                                walk += strlen("album");
                        }
                        else if (BEGINS_WITH(walk+1, "track")) {
                                if (*trackbuf)
                                        outwalk += sprintf(outwalk, "%s", trackbuf);
                                walk += strlen("track");
                        }
                        else if (BEGINS_WITH(walk+1, "date")) {
                                if (*datebuf)
                                        outwalk += sprintf(outwalk, "%s", datebuf);
                                walk += strlen("date");
                        }
                }

                mpd_song_free(song);
        } else {
                START_COLOR("color_bad");
                outwalk += sprintf(outwalk, "%s", "Stopped");
                mpd_connection_free(conn);
                conn = NULL;
        }

print_end:

        END_COLOR;
        OUTPUT_FULL_TEXT(buffer);

        return;
}
