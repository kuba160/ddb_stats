/*
    Statistics plugin for DeaDBeeF
    Copyright (C) 2018 Jakub Wasylków <kuba_160@protonmail.com>

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>
#include <string.h>
#include <deadbeef/deadbeef.h>
#include "stats.h"

extern DB_functions_t *deadbeef;
extern DB_output_t *output;

int stats_times_skipped (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2, void * value) {
    switch (id) {
        case DB_EV_NEXT:
        case DB_EV_PLAY_NUM:
        if (output)
            if (output->state() == OUTPUT_STATE_PLAYING)
                *((int *) value) += 1;
        break;
    }
    return 0;
}

float plt_get_totaltime () {
    float totaltime = 0.0;
    int last_plt_index = deadbeef->plt_get_count () - 1;
    int i;
    for (i = 0; i <= last_plt_index; i++) {
        ddb_playlist_t * plt = deadbeef->plt_get_for_idx (i);
        totaltime += deadbeef->plt_get_totaltime (plt);
        deadbeef->plt_unref (plt);
    }
    return totaltime;
}

int plt_totaltime_parse (void * value, char * out, int out_size) {
    float value_float = *((float *) value);
    float value_new = plt_get_totaltime();
    float value_diff = value_new - value_float;
    *((float *) value) = value_new;
    char buff[255];
    deadbeef->pl_format_time (value_new, buff, 255);
    char buff_diff[255];
    deadbeef->pl_format_time (value_diff, buff_diff, 255);
    if (value_diff == (float) 0)
        return snprintf (out, out_size, "%s", buff);
    else if (value_diff > (float) 0)
        return snprintf (out, out_size, "%s (+%s)", buff, buff_diff);
    deadbeef->pl_format_time (value_diff * -1, buff_diff, 255);
    return snprintf (out, out_size, "%s (-%s)", buff, buff_diff);
}

int plt_get_totaltracks () {
    int totaltracks = 0;
    int last_plt_index = deadbeef->plt_get_count () - 1;
    int i;
    for (i = 0; i <= last_plt_index; i++) {
        ddb_playlist_t * plt = deadbeef->plt_get_for_idx (i);
        totaltracks += deadbeef->plt_get_item_count (plt, PL_MAIN);
        deadbeef->plt_unref (plt);
    }
    return totaltracks;
}

int plt_totaltracks_parse (void * value, char * out, int out_size) {
    int value_int = *((int *) value);
    int value_new = plt_get_totaltracks();
    int value_diff = value_new - value_int;
    *((int *) value) = value_new;
    if (value_diff == 0)
        return snprintf (out, out_size, "%d", value_new);
    else if (value_diff > 0)
        return snprintf (out, out_size, "%d (+%d)", value_new, value_diff);
    return snprintf (out, out_size, "%d (%d)", value_new, value_diff);
}


int plt_trackavg_parse (void * value, char * out, int out_size) {
    float * pb_time = (float *) get_entry_value ("Tracks", "playback_time");
    int * pb_tracks = (int *) get_entry_value ("Tracks", "total_tracks");
    float time_per_track;
    if (pb_time && pb_tracks)
        time_per_track = *pb_time / *pb_tracks;

    char buff[255];
    deadbeef->pl_format_time (time_per_track, buff, 255);
    return snprintf (out, out_size, "%s", buff);
}

struct stat_entry timesskipped = {
    .plugin = "Tracks",
    .name = "times_skipped",
    .description = "Songs skipped",
    .type = TYPE_INT,
    .message = stats_times_skipped
};

struct stat_entry plt_totaltime = {
    .plugin = "Tracks",
    .name = "playback_time",
    .description = "Total playback time",
    .value_parse = plt_totaltime_parse,
    .type = TYPE_FLOAT,
};

struct stat_entry plt_totaltracks = {
    .plugin = "Tracks",
    .name = "total_tracks",
    .description = "Track count",
    .value_parse = plt_totaltracks_parse,
    .type = TYPE_INT,
};

struct stat_entry plt_trackavg = {
    .plugin = "Tracks",
    .name = "track_avg",
    .description = "Average track duration",
    .value_parse = plt_trackavg_parse,
    .type = TYPE_DUMMY
};

void stats_playlist () {
    stats_entry_add (timesskipped);
    stats_entry_add (plt_totaltime);
    stats_entry_add (plt_totaltracks);
    stats_entry_add (plt_trackavg);
}
