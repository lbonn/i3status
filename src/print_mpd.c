// vim:ts=8:expandtab
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_version.h>

#include <mpd/client.h>

#include "i3status.h"

static unsigned int utf8_count_bytes(char c) {
    unsigned char uc = (unsigned char)c;

    if ((uc & 0xfc) == 0xfc) {
        return 6;
    }
    if ((uc & 0xf8) == 0xf8) {
        return 5;
    }
    if ((uc & 0xf0) == 0xf0) {
        return 4;
    }
    if ((uc & 0xe0) == 0xe0) {
        return 3;
    }
    if ((uc & 0xc0) == 0xc0) {
        return 2;
    }

    return 1;
}

static void _copy_crop(char *dst, const char *src, size_t dst_len) {
    static const char *ellipsis = "...";
    size_t crop_len = dst_len - strlen(ellipsis) - 1;
    size_t k;

    size_t ck = 0;
    unsigned int utf8_bytes_rem = 0;

    /* no source: empty string */
    if (!src) {
        dst[0] = 0;
        return;
    }

    /* copy and respect these conditions:
     * - strlen(dst) < crop_len
     * - unterminated utf8 chars are discarded
     */
    for (k = 0; k < crop_len; k++) {
        if (src[k] == 0) {
            break;
        }

        dst[k] = src[k];

        if (utf8_bytes_rem == 0) {
            /* expecting a new char */
            utf8_bytes_rem = utf8_count_bytes(src[k]) - 1;
        } else {
            utf8_bytes_rem -= 1;
        }

        if (utf8_bytes_rem == 0) {
            /* end of a char */
            ck = k + 1;
        }
    }

    /* add ellipsis */
    if (strlen(src) > ck) {
        strcpy(dst + ck, ellipsis);
        ck += 3;
    }

    dst[ck] = 0;
}

#define copy_crop(dst, src) _copy_crop(dst, src, sizeof(dst))

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

    copy_crop(titlebuf, mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
    copy_crop(artistbuf, mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
    copy_crop(albumbuf, mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
    copy_crop(trackbuf, mpd_song_get_tag(song, MPD_TAG_TRACK, 0));
    copy_crop(datebuf, mpd_song_get_tag(song, MPD_TAG_DATE, 0));

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
