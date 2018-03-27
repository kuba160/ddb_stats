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
// Thanks to Joakim Jensen for help with html/css code

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <deadbeef/deadbeef.h>
#include "stats.h"

#define ENTRY_MAX 1024

#define HTML1_ENDING "</b><br></span>"
#define HTML_END "\t</div>\n</div>\n</body>\n</html>"

extern DB_functions_t *deadbeef;

extern unsigned char index1_txt[];
extern unsigned int index1_txt_len;

void
html_table_gen (FILE * fp, const char * plugin, struct stat_entry * list[ENTRY_MAX], int list_count) {
	fprintf (fp, "<h1>%s</h1>\n", plugin);

	fprintf (fp, "<table align=\"center\">\n<tr>\n<th>Description</th>\n<th>Value</th>\n</tr>\n" );
	int i;
	for (i = 0; i < list_count; i++) {
		if (strcmp (list[i]->plugin, plugin) == 0) {
			if (!(list[i]->description))
				list[i]->description = list[i]->name;
			// ignore hidden
			if (list[i]->settings & FLAG_HIDDEN)
				continue;
			// use custom parser if available
			if (list[i]->value_parse) {
				char buffer[255];
				int ret = list[i]->value_parse (list[i]->value, buffer, 255);
				// TODO check if whole buffer is full
				fprintf (fp, "<tr><td>%s</td><td>%s</td></tr>\n",list[i]->description, buffer);
			}
			else {
				switch (list[i]->type) {
					case TYPE_INT:
						fprintf (fp, "<tr><td>%s</td><td>%d</td></tr>\n",list[i]->description, *((int *) list[i]->value));
					    break;
					case TYPE_STRING:

						fprintf (fp, "<tr><td>%s</td><td>%s</td></tr>\n",list[i]->description, ((char *) list[i]->value));
					    break;
					default:
						deadbeef->log ("stats: unsupported type (todo)\n");
						break;
				}
			}
		}
	}
	fprintf (fp, "</table>\n");
}

static int compare (const void * a, const void * b) {
	if (*(const char **) b == 0 || *(const char **) a == 0)
		return 0;
    return strcmp (*(const char **) a, *(const char **) b);
}

int stats_gen_html (char * filename, struct stat_entry * (*table)[ENTRY_MAX], int list_count) {
	struct stat_entry * list[ENTRY_MAX];
	memcpy (&list, table, sizeof (struct stat_entry *[ENTRY_MAX]));

	FILE * fp = fopen (filename, "w");
	if (!fp)
		return 1;
	fwrite(index1_txt, index1_txt_len, 1, fp);

	int i, w = 0;
	for (i = 0; i < list_count; i++) {
		if (strcmp (list[i]->name, "time_played") == 0) {
			// just to be sure
			if (strcmp (list[i]->plugin, "General") == 0) {
				char timeplayed_string[255];
				int timeplayed_int = deadbeef->conf_get_int ("stats_general_time_played", 0);
				float timeplayed_float = timeplayed_int;
				deadbeef->pl_format_time (timeplayed_float, timeplayed_string, sizeof (timeplayed_string));
				fprintf (fp, "%s", timeplayed_string);
				w = 1;
				break;
			}
		}
	}
	if (!w) {
		fprintf (fp, "??:??");
	}
	fprintf (fp, "%s\n",HTML1_ENDING);

	html_table_gen (fp, "General", list, list_count);

	const char * names[ENTRY_MAX] = { 0 };
	memset (&names, 0, sizeof(names));
	for (i = 0; i < list_count; i++) {
		int x = 0;
		while (names[x] != 0) {
			if (strcmp (names[x], list[i]->plugin) == 0) {
				// found
				x = -1;
				break;
			}
			x++;
		}
		if (x != -1) {
			names[x] = list[i]->plugin;
		}
	}
	qsort(names, ENTRY_MAX, sizeof(char *), compare);
	i = 0;
	while ( names[i] != 0) {
		if (strcmp ("General", names[i]) != 0)
			html_table_gen (fp, names[i], list, list_count);
		i++;
	}
	fprintf (fp, "%s\n",HTML_END);
	fclose (fp);
	return 0;
}