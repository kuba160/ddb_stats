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

#include <deadbeef/deadbeef.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "stats.h"

#ifndef __MINGW32__
#define TRACE_GDB
#endif

#ifdef TRACE_GDB
#include <sys/ptrace.h>
#include <sys/wait.h>
#endif

DB_functions_t *deadbeef;
DB_output_t *output;

// type casting
#define P_INT(x) *((int *)(x))
#define P_INT64(x) *((int64_t *)(x))
#define STRING(x) ((char *)(x))

//#define trace(...) { deadbeef->log ( __VA_ARGS__); }
#define trace(...) { fprintf(stdout, __VA_ARGS__); }

static intptr_t stats_tid;
static int thread_terminate = 0;

#define ENTRY_MAX 1024
struct stat_entry * list[ENTRY_MAX];
int list_count = 0;

// defined in default_values.c
extern int stats_default ();
// defined in gen_html.c
extern int stats_gen_html (char * filename, struct stat_entry * (*table)[ENTRY_MAX], int list_count ); 

static int stats_value_build (struct stat_entry ext, char * buf, int buf_size) {
	return snprintf (buf, buf_size, "stats_%s_%s", ext.plugin, ext.name);
}

void
stats_save () {
	int i;
	char buf[255];
	for (i = 0; i < list_count; i++) {
		stats_value_build (*(list[i]), buf, 255);
		//deadbeef->log ("saving %s\n",buf);
		switch (list[i]->type) {
			case TYPE_INT:
				trace ("%s: %d\n", buf, P_INT(list[i]->value));
				deadbeef->conf_set_int (buf, P_INT(list[i]->value));
				break;
			case TYPE_INT64:
				deadbeef->conf_set_int64 (buf, P_INT64(list[i]->value));
				break;
			case TYPE_STRING:
				deadbeef->conf_set_str (buf, STRING(list[i]->value));
				break;
			default:
				deadbeef->log ("stats: unsupported type (todo)\n");
		}
	}
}

static int
stats_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    // maybe consider to use DB_EV_SONGFINISHED ?
    case DB_EV_SONGCHANGED:
        stats_save();
        break;
    case DB_EV_OUTPUTCHANGED:
		output = deadbeef->get_output ();
        break;
    }
    int i;
    for (i = 0; i < list_count; i++) {
    	if (list[i]->message)
    		list[i]->message (id, ctx, p1, p2, list[i]->value);
    }
    return 0;
}

static void
stats_thread (void *context) {
	while (!thread_terminate) {
		int i;
		for (i = 0; i < list_count; i++) {
			if (list[i]->once) {
				list[i]->once (list[i]->value);
				list[i]->once = 0;
			}
			if (list[i]->loop) {
				list[i]->loop (list[i]->value);
			}
		}
        sleep (1);
    }
}

static int
stats_start () {
	stats_default ();
	stats_tid = deadbeef->thread_start (stats_thread, NULL);
	return 0;
}

static int
stats_stop () {
	thread_terminate = 1;
	stats_save();

	int i;
	char buf[255];
	for (i = 0; i < list_count; i++) {
		stats_value_build (*(list[i]), buf, 255);
		switch (list[i]->type) {
			case TYPE_INT:
				trace ("STATS: %s %d\n",buf,P_INT(list[i]->value) );
				break;
			default:
				deadbeef->log ("STATS: unsupported type (todo)\n");
				break;
		}
	}
	for (i = 0; i < list_count; i++) {
		if (list[i]->value && list[i]->type != TYPE_DUMMY && (list[i]->settings & SETTING_ALLOCATED))
			free (list[i]->value);
		free (list[i]);
	}
	return 0;
}

static int
stats_action_show_stats (DB_plugin_action_t *act, int ctx) {
	stats_save ();
	char filename[1024];
	strcpy (filename, deadbeef->get_config_dir());
	strcat (filename, "/stats.html");

	int ret = stats_gen_html (filename, &list, list_count);
	if (ret) {
		deadbeef->log ("Generating html file failed with code %d\n", ret);
		return ret;
	}

	char browser_buf[1024];
	#ifdef __MINGW32__
	strcpy (browser_buf, "cmd /c start ");
	strcat (browser_buf, filename);
	#else
	deadbeef->conf_get_str ("stats.browser","xdg-open",browser_buf, sizeof(browser_buf));
	strcat (browser_buf, " ");
	strcat (browser_buf,filename);
	#endif
	trace ("stats: > %s\n",browser_buf);
	system (browser_buf);
	return 0;
}

void * stats_entry_add (struct stat_entry val) {
	if (!(val.name && val.plugin)) {
		return NULL;
	}
	if (list_count >= ENTRY_MAX) {
		deadbeef->log ("stats: maximum stats count reached\n");
		return NULL;
	}
	struct stat_entry * mem = malloc (sizeof(struct stat_entry));
	if (!mem) {
		deadbeef->log ("failed to alloc\n");
		return NULL;
	}

	memcpy (mem, &val, sizeof(struct stat_entry));
	mem->settings = 0;
	char buf[255];
	stats_value_build (*mem, buf, 255);
	if (mem->type == TYPE_INT) {
		if (!mem->value) {
			mem->settings += SETTING_ALLOCATED;
			mem->value = malloc(sizeof(int));
		}
		if (mem->value)
			P_INT(mem->value) = deadbeef->conf_get_int (buf, 0);
		trace ("stats: new entry %s with value %d\n", buf, P_INT(mem->value));
	}
	else if (mem->type == TYPE_STRING) {
		if (!mem->value) {
			mem->settings += SETTING_ALLOCATED;
			mem->value = calloc(1, mem->string_len);
		}
		if (mem->value) 
			deadbeef->conf_get_str (buf, "", mem->value, mem->string_len);
		trace ("stats: new entry %s with value %s\n",buf, ((char *) mem->value));
	}
	else
		deadbeef->log ("stats: type %d not supported yet?\n");

	list[list_count] = mem;
	return list[list_count++]->value;
}

static DB_plugin_action_t add_stats_action = {
    .name = "stats_add",
    .title = "Help/Statistics",
    .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
    .callback2 = stats_action_show_stats,
    .next = NULL
};

static DB_plugin_action_t *
stats_get_actions (DB_playItem_t *it) {
	if (!it)
		return &add_stats_action;
	return NULL;
}

static const char settings_dlg[] =
    "property \"Web browser\" entry stats.browser xdg-open;\n";

DB_misc_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 10,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.version_major = 0,
    .plugin.version_minor = 7,
    .plugin.id = "stats",
    .plugin.name ="Statistics for DeaDBeeF",
    .plugin.descr = "Plugin which collects data for stats",
    .plugin.copyright =
        "Statistics plugin for DeaDBeeF\n"
        "Copyright (C) 2018 Jakub Wasylków <kuba_160@protonmail.com>\n"
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
        "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .plugin.message = stats_message,
    .plugin.start = stats_start,
    .plugin.stop = stats_stop,
    .plugin.get_actions = stats_get_actions,
    .plugin.configdialog = settings_dlg
};

DB_plugin_t *
stats_load (DB_functions_t *ddb) {
    deadbeef = ddb;
    return DB_PLUGIN(&plugin);
}
