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

#include <deadbeef/deadbeef.h>
#include "stats.h"

#define ENTRY_MAX 1024

#define HTML1_ENDING "</b><br></span>"
#define HTML_END "\t</div>\n</div>\n</body>\n</html>"

extern DB_functions_t *deadbeef;

extern unsigned char index1_txt[];
extern unsigned int index1_txt_len;

int stats_gen_html (char * filename, struct stat_entry * (*table)[ENTRY_MAX], int list_count ) {
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
			if (strcmp (list[i]->plugin, "general") == 0) {
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

	// table
	int times_run = 0;
	fprintf (fp, "<table align=\"center\">\n<tr>\n<th>Description</th>\n<th>Value</th>\n</tr>\n" );
	for (i = 0; i < list_count; i++) {
		if (strcmp (list[i]->plugin, "general") == 0) {
			if (!(list[i]->description))
				list[i]->description = list[i]->name;
			// written before
			if (strcmp (list[i]->name, "time_played") == 0)
				continue;
			// save value to calculate percentage for times_run_dbg
			if (strcmp (list[i]->name, "times_run") == 0)
				times_run =  *((int *) list[i]->value);
			// times_run_dbg has percentage written next to value
			if (strcmp (list[i]->name, "times_run_dbg") == 0) {
				if (times_run) {
					int percent =  (*((int *) list[i]->value)*100)/times_run;
					fprintf (fp, "<tr><td>%s</td><td>%d (%d%%)</td></tr>\n",list[i]->description, *((int *) list[i]->value), percent);
					continue;
				}
			}
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
	fprintf (fp, "</table>\n");
	
	fprintf (fp, "%s\n",HTML_END);
	fclose (fp);
	return 0;
}