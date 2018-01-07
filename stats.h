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

struct stat_entry {
	// plugin which stat belongs to ("general" used for internal)
	const char * plugin;
	// value name
	const char * name;
	// description to be displayed on statistics page
	const char * description;
	// pointer to value (set by stats_entry_add)
	void * value;
	// value type
	int type;
	// length of string (if type is string)
	int string_len;
	// pointer to message function (called every message)
	int (*message)(uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2, void * value);
	// pointer to loop function (called ca. every second)
	void (*loop)(void * value);
	// pointer to function to be called once
	int (*once)(void * value);
	// internal flags
	char settings;
};

void * stats_entry_add (struct stat_entry val);

void stats_save ();

#define TYPE_INT 0
#define TYPE_INT64 1
#define TYPE_FLOAT 2
#define TYPE_STRING 3
#define TYPE_DUMMY 4

#define SETTING_ALLOCATED 1