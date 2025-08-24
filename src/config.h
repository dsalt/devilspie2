/**
 *	This file is part of devilspie2
 *	Copyright (C) 2013-2017 Andreas Rönnquist
 *
 *	devilspie2 is free software: you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as published
 *	by the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	devilspie2 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with devilspie2.
 *	If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __HEADER_CONFIG_
#define __HEADER_CONFIG_

#include "glib.h"

int load_config(gchar *config_filename);

void clear_file_lists();

gboolean is_in_any_list(const gchar *filename);

typedef enum {
	W_OPEN,
	W_CLOSE,
	W_FOCUS,
	W_BLUR,
	W_NAME_CHANGED,
	W_NUM_EVENTS /* keep this at the end */
} win_event_type;

extern GSList *event_lists[W_NUM_EVENTS];
extern const char *const event_names[W_NUM_EVENTS];

// Our git version which is defined through some magic in the build system
extern const char *gitversion;

#endif /*__HEADER_CONFIG_*/
