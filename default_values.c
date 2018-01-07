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
#include "stats.h"

// comment to remove debugger tracing
#define TRACE_GDB

#ifdef __MINGW32__
#undef TRACE_GDB
#endif

#ifdef TRACE_GDB
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
static int
detect_gdb () {
	int pid = fork(), res, status;
	if (pid == -1)
		return -1;
	else if (!pid) {
		int ppid = getppid();
		if (ptrace(PTRACE_ATTACH, ppid, NULL, NULL) == 0) {
          waitpid(ppid, NULL, 0);
          ptrace(PTRACE_CONT, NULL, NULL);
          ptrace(PTRACE_DETACH, getppid(), NULL, NULL);
          res = 0;
        }
      	else
          res = 1;
      	_exit(res);
	}
  	else {
      waitpid(pid, &status, 0);
      res = WEXITSTATUS(status);
    }
 	return res;
}
#endif

extern DB_functions_t *deadbeef;
extern DB_output_t *output;

void stats_time_played (void * value) {
	if (output)
		if (output->state () == OUTPUT_STATE_PLAYING){
   			(*((int *) value)) += 1;
		}
}

int stats_times_run (void * value) {
   	(*((int *) value))++;
   	return 0;
}

#ifdef TRACE_GDB
int stats_times_run_dbg (void * value) {
   	if (detect_gdb() == 1){
		(*((int *) value))++;
		stats_save ();
	}
   	return 0;
}
#endif

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

struct stat_entry totaltime = {
	.plugin = "general",
	.name = "time_played",
	.description = 0, // Not used
	.type = TYPE_INT,
	.loop = stats_time_played
};

struct stat_entry timesrun = {
	.plugin = "general",
	.name = "times_run",
	.description = "Times started",
	.type = TYPE_INT,
	.once = stats_times_run
};

#ifdef TRACE_GDB
struct stat_entry timesrun_dbg = {
	.plugin = "general",
	.name = "times_run_dbg",
	.description = "Times started with GDB",
	.type = TYPE_INT,
	.once = stats_times_run_dbg
};
#endif

struct stat_entry timesskipped = {
	.plugin = "general",
	.name = "times_skipped",
	.description = "Songs skipped",
	.type = TYPE_INT,
	.message = stats_times_skipped
};

int stats_default () {
	stats_entry_add (totaltime);
	stats_entry_add (timesrun);
	#ifdef TRACE_GDB
	stats_entry_add (timesrun_dbg);
	#endif
	stats_entry_add (timesskipped);
	return 0;
}
