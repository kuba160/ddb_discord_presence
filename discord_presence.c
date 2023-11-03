// Copyright (C) 2018-2022 Jakub Wasylków <kuba_160@protonmail.com>
// SPDX-FileCopyrightText: 2018-2022 Jakub Wasylków <kuba_160@protonmail.com>
//
// SPDX-License-Identifier: Zlib

#include "artwork/lastfm.h"
#include "discord_rpc.h"
#include <deadbeef/deadbeef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DB_misc_t plugin;
//#define trace(...) { deadbeef->log ( __VA_ARGS__); }
#define trace(...) \
    { deadbeef->log_detailed(&plugin.plugin, 0, __VA_ARGS__); }
#define trace_err(...) \
    { deadbeef->log(__VA_ARGS__); }

#define APPLICATION_ID "424928021309554698"
#define MAX_LEN 256

DB_functions_t *deadbeef;
char discord_enabled = 0;
int playback_resume = 0;
int playback_resume_status = 0;

static void
discordInit () {
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    // handlers are not used in our case
    //handlers.ready = handleDiscordReady;
    //handlers.disconnected = handleDiscordDisconnected;
    //handlers.errored = handleDiscordError;
    //handlers.joinGame = handleDiscordJoin;
    //handlers.spectateGame = handleDiscordSpectate;
    //handlers.joinRequest = handleDiscordJoinRequest;
    Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
}

static char *
nowplaying_format_string (char *script) {
    DB_playItem_t *nowplaying = deadbeef->streamer_get_playing_track();
    if (!nowplaying) {
        return NULL;
    }
    ddb_playlist_t *nowplaying_plt = deadbeef->plt_get_curr();
    char *code_script = deadbeef->tf_compile(script);
    ddb_tf_context_t context;
    {
        memset(&context, 0, sizeof(ddb_tf_context_t));
        context._size = sizeof(ddb_tf_context_t);
        context.it = nowplaying;
        context.plt = nowplaying_plt;
        context.iter = PL_MAIN;
    }
    char *out = malloc(MAX_LEN);
    if (out && code_script) {
        *out = 0;
        deadbeef->tf_eval(&context, code_script, out, MAX_LEN);
        trace("nowplaying_format_string: \"%s\"\n", out);
    }
    deadbeef->pl_item_unref(nowplaying);
    if (nowplaying_plt) {
        deadbeef->plt_unref(nowplaying_plt);
    }
    if (code_script) {
        deadbeef->tf_free(code_script);
    }
    return out;
}

static float
nowplaying_length () {
    DB_playItem_t *nowplaying = deadbeef->streamer_get_playing_track();
    if (nowplaying) {
        float item_length = deadbeef->pl_get_item_duration(nowplaying);
        deadbeef->pl_item_unref(nowplaying);
        return item_length;
    }
    return 0.0;
}

#define STATUS_PAUSED 1
#define STATUS_SONGCHANGED 2
#define STATUS_SEEKED 3

int playback_status;
float song_len;

static void
updateDiscordPresence (void *_) {
    trace("updateDiscordPresence\n");
    // Arguments:
    //     playback_status: as above
    //     song_len: set only if playback_status=STATUS_SONGCHANGED

    // HACK, fix for playback resumption on start
    if (playback_resume && (playback_status == STATUS_SONGCHANGED || playback_status == STATUS_SEEKED)) {
        playback_resume--;
        if (playback_resume_status) {
            playback_status = STATUS_PAUSED;
            if (deadbeef->conf_get_int("discord_presence.hide_on_pause", 1)) {
                Discord_ClearPresence();
                return;
            }
        }
    }

    DB_playItem_t *nowplaying = deadbeef->streamer_get_playing_track();
    if (!nowplaying) {
        return;
    }

    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));

    char script[MAX_LEN];

    // title_text
    char *title_text;
    deadbeef->conf_get_str("discord_presence.title_script", "%title%$if(%ispaused%,' ('paused')')", script, MAX_LEN);
    title_text = nowplaying_format_string(script);
    discordPresence.details = title_text;

    // state_text
    char *state_text;
    if (deadbeef->conf_get_int("discord_presence.playlist_on_state", 0)) {
        state_text = malloc(MAX_LEN);
        if (state_text) {
            ddb_playlist_t *nowplaying_plt = deadbeef->plt_get_for_idx(deadbeef->streamer_get_current_playlist());
            if (nowplaying_plt) {
                deadbeef->plt_get_title(nowplaying_plt, state_text, MAX_LEN);
                deadbeef->plt_unref(nowplaying_plt);
            }
        }
    }
    else {
        deadbeef->conf_get_str("discord_presence.state_script", "%artist%", script, MAX_LEN);
        state_text = nowplaying_format_string(script);
    }
    discordPresence.state = state_text;

    // icon_text
    char *icon_text;
    deadbeef->conf_get_str("discord_presence.icon_script", "%artist% \'/\' %album%", script, MAX_LEN);
    icon_text = nowplaying_format_string(script);
    discordPresence.largeImageText = icon_text;

    // tracknum
    int nowplaying_num = 0;
    int nowplaying_all = 0;
    if ((nowplaying_num = deadbeef->conf_get_int("discord_presence.show_tracknum", 1))) {
        // using nowplaying num as type selection
        while (1) {
            if (nowplaying_num == 3 || nowplaying_num == 2) {
                // Set index in album
                DB_metaInfo_t *track = deadbeef->pl_meta_for_key(nowplaying, "track");
                DB_metaInfo_t *totaltracks = deadbeef->pl_meta_for_key(nowplaying, "numtracks");

                if (track && totaltracks) {
                    // assuming value is not NULL
                    nowplaying_num = atoi(track->value);
                    nowplaying_all = atoi(totaltracks->value);
                    break;
                }
                if (nowplaying_num == 2) {
                    break;
                }
                /* fall through on type=3 */
            }
            // Set index in playlist
            nowplaying_num = deadbeef->pl_get_idx_of(nowplaying) + 1;
            ddb_playlist_t *nowplaying_plt = deadbeef->plt_get_for_idx(deadbeef->streamer_get_current_playlist());
            if (nowplaying_plt) {
                nowplaying_all = deadbeef->plt_get_item_count(nowplaying_plt, PL_MAIN);
                deadbeef->plt_unref(nowplaying_plt);
            }
            break;
        }
    }
    discordPresence.partySize = nowplaying_num;
    discordPresence.partyMax = nowplaying_all;

    // HACK: disable timestamp if seeked but paused
    if (playback_status == STATUS_SEEKED) {
        if (deadbeef->get_output() &&
            deadbeef->get_output()->state() != DDB_PLAYBACK_STATE_PLAYING) {
            playback_status = STATUS_PAUSED;
            if (deadbeef->conf_get_int("discord_presence.hide_on_pause", 1)) {
                Discord_ClearPresence();
                free(title_text);
                free(state_text);
                free(icon_text);
                return;
            }
        }
    }

    // time played
    discordPresence.startTimestamp = 0;
    discordPresence.endTimestamp = 0;
    if (playback_status != STATUS_PAUSED && deadbeef->conf_get_int("discord_presence.end_timestamp", 0) != 2) {
        // startTimestamp
        discordPresence.startTimestamp = time(0);
        if (playback_status != STATUS_SONGCHANGED) {
            discordPresence.startTimestamp -= (int)(nowplaying_length() * deadbeef->playback_get_pos() / 100);
        }

        // endTimestamp (calculate if needed)
        if (deadbeef->conf_get_int("discord_presence.end_timestamp", 0)) {
            discordPresence.instance = 1;
            if (playback_status == STATUS_SONGCHANGED)
                discordPresence.endTimestamp = discordPresence.startTimestamp + (int)song_len;
            else
                discordPresence.endTimestamp = discordPresence.startTimestamp + (int)nowplaying_length();
        }
    }

    // misc
    discordPresence.largeImageKey = "default";
    discordPresence.smallImageKey = 0;

    static const char *artist_keys[4] = {"artist", "album artist", "composer", "performer"};
#define ARTIST_KEY_COUNT (sizeof(artist_keys) / sizeof(artist_keys[0]))

    char lastfm_link[MAX_LEN];
    if (deadbeef->conf_get_int("discord_presence.lastfm_cover", 1)) {
        char lastfm_album[MAX_LEN];
        *lastfm_album = 0;
        deadbeef->pl_get_meta(nowplaying, "album", lastfm_album, MAX_LEN);

        if (lastfm_album[0]) {
            char lastfm_artist[MAX_LEN];
            for (int i = 0; i < ARTIST_KEY_COUNT; ++i) {
                *lastfm_artist = 0;
                deadbeef->pl_get_meta(nowplaying, artist_keys[i], lastfm_artist, MAX_LEN);
                if (lastfm_artist[0]) {
                    int ret = fetch_from_lastfm(lastfm_artist, lastfm_album, lastfm_link, MAX_LEN);
                    if (ret > 0) {
                        discordPresence.largeImageKey = lastfm_link;
                        break;
                    }
                }
            }
        }
    }

    if (playback_status == STATUS_PAUSED) {
        if (deadbeef->conf_get_int("discord_presence.paused_icon", 1))
            discordPresence.smallImageKey = "paused_circle";
    }
    //discordPresence.partyId = 0;
    //discordPresence.matchSecret = 0;
    //discordPresence.joinSecret = 0;
    //discordPresence.spectateSecret = 0;

    Discord_UpdatePresence(&discordPresence);

    if (title_text)
        free(title_text);
    if (state_text)
        free(state_text);
    if (icon_text)
        free(icon_text);

    deadbeef->pl_item_unref(nowplaying);
}

void
create_update_thread (int ps, float slen) {
    playback_status = ps;
    song_len = slen;
    deadbeef->thread_start(updateDiscordPresence, NULL);
}

static int
discord_presence_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
        case DB_EV_CONFIGCHANGED:
            if (deadbeef->conf_get_int("discord_presence.enable", 1)) {
                if (!discord_enabled) {
                    discordInit();
                    discord_enabled = 1;
                }
            }
            else {
                if (discord_enabled) {
                    Discord_ClearPresence();
                    Discord_Shutdown();
                    discord_enabled = 0;
                }
            }
            break;
        case DB_EV_SONGCHANGED:
            if (discord_enabled) {
                // do not update if track changed to NULL
                if (((ddb_event_trackchange_t *)ctx)->to != 0) {
                    // DB_EV_SONGCHANGED is emited twice (bug? ignoring the first one)
                    // this has been fixed in commit d63a53bf70f09bc8534f6394580221c0af1babeb (2018-05-23)
                    //if ( ((ddb_event_trackchange_t *)ctx)->to != ((ddb_event_trackchange_t *)ctx)->from) {
                    float nextitem_length = deadbeef->pl_get_item_duration(((ddb_event_trackchange_t *)ctx)->to);
                    create_update_thread(STATUS_SONGCHANGED, nextitem_length);
                    //}
                }
                else {
                    // update: change track if it changed to null as this happens when option "Stop after current track/album" is enabled
                    Discord_ClearPresence();
                }
            }
            break;
        case DB_EV_SEEKED:
            if (discord_enabled) {
                create_update_thread(STATUS_SEEKED, 0);
            }
            break;
        case DB_EV_PAUSED:
            if (discord_enabled) {
                if (p1 && deadbeef->conf_get_int("discord_presence.hide_on_pause", 1)) {
                    Discord_ClearPresence();
                }
                else {
                    create_update_thread(p1, 0);
                }
            }
            break;
        case DB_EV_STOP:
            if (discord_enabled) {
                Discord_ClearPresence();
            }
            break;
    }
    return 0;
}

static int
discord_presence_start () {
    int enable = deadbeef->conf_get_int("discord_presence.enable", 1);
    if (enable) {
        discordInit();
        discord_enabled = 1;
    }
    // HACK, determine if resuming track
    {
        int resume = deadbeef->conf_get_int("resume_last_session", 1);
        int plt = deadbeef->conf_get_int("resume.playlist", -1);
        int track = deadbeef->conf_get_int("resume.track", -1);
        float pos = deadbeef->conf_get_float("resume.position", -1);
        int paused = deadbeef->conf_get_int("resume.paused", 0);
        if (resume && plt >= 0 && track >= 0 && pos >= 0) {
            playback_resume = 3;
            playback_resume_status = paused;
        }
    }

    return 0;
}

static int
discord_presence_stop () {
    if (discord_enabled) {
        Discord_Shutdown();
        discord_enabled = 0;
    }
    return 0;
}

static const char settings_dlg[] =
    "property \"Enable\" checkbox discord_presence.enable 1;\n"
    "property \"Title format\" entry discord_presence.title_script \"%title%$if(%ispaused%,' ('paused')')\";\n"
    "property \"State format\" entry discord_presence.state_script \"%artist%\";\n"
    "property \"Overwrite state format with playlist name\" checkbox discord_presence.playlist_on_state 0;\n"
    "property \"Range display '(n of m)'\" select[4] discord_presence.show_tracknum 1 \"Hidden\" \"Track index in playlist\" \"Track index in album\" \"Track index in album/playlist (fallback)\";\n"
    "property \"Display time\" select[3] discord_presence.end_timestamp 0 \"Elapsed time\" \"Remaining time\" \"Don't display time\";\n"
    "property \"Icon text format\" entry discord_presence.icon_script \"%artist% \'/\' %album%\";\n"
    "property \"Show paused icon\" checkbox discord_presence.paused_icon 1;\n"
    "property \"Hide presence on pause\" checkbox discord_presence.hide_on_pause 1;\n"
    "property \"Display cover from last.fm\" checkbox discord_presence.lastfm_cover 1;\n";

DB_misc_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 10,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.version_major = 1,
    .plugin.version_minor = 5,
    .plugin.id = "discord_presence",
    .plugin.name = "Discord Rich Presence Plugin",
    .plugin.descr = "Discord Rich Presence Plugin shows your current playing track on your Discord status.\n"
                    "It connects with Discord through Discord Rich Presence API, no further authentication is needed.\n"
                    "You can configure displayed information through plugin settings.\n"
                    "For more information about title formatting please visit:\n"
                    "https://github.com/DeaDBeeF-Player/deadbeef/wiki/Title-formatting-2.0",
    .plugin.copyright =
        "Discord Rich Presence Plugin for DeaDBeeF\n"
        "Copyright (C) 2018-2021 Jakub Wasylków <kuba_160@protonmail.com>\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n",
    .plugin.website = "http://github.com/kuba160/ddb_discord_presence",
    .plugin.message = discord_presence_message,
    .plugin.start = discord_presence_start,
    .plugin.stop = discord_presence_stop,
    .plugin.configdialog = settings_dlg,
};

DB_plugin_t *
discord_presence_load (DB_functions_t *ddb) {
    deadbeef = ddb;
    return DB_PLUGIN(&plugin);
}
