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
#include <glib.h>
#include <string.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <glib/gi18n.h>

#include <locale.h>


#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "script.h"
#include "script_functions.h"

#include "config.h"

/**
 *
 */
GSList *event_lists[W_NUM_EVENTS] = { NULL, NULL, NULL, NULL, NULL };
const char *const event_names[W_NUM_EVENTS] = {
	"window_open",
	"window_close",
	"window_focus",
	"window_blur",
	"window_name_change",
};


/**
 * filename_list_sortfunc
 *   function to sort the inserted filenames, to be able to determine
 *   which order files are loaded.
 */
static gint filename_list_sortfunc(gconstpointer a,gconstpointer b)
{
	gchar *file1 = (gchar*)a;
	gchar *file2 = (gchar*)b;

	return g_ascii_strcasecmp(file1, file2);
}


/**
 *
 */
static GSList *add_lua_file_to_list(GSList *list, const gchar *script_folder, const gchar *filename)
{
	gchar *temp_filename = g_strdup(filename);  // Is this really needed? Daren't remove
	gchar *added_filename = g_build_path(G_DIR_SEPARATOR_S,
											script_folder,
											temp_filename,
											NULL);

	list=g_slist_insert_sorted(list,
	                           added_filename,
	                           filename_list_sortfunc);

	return list;
}

static gboolean get_lua_table(lua_State *luastate, gchar *table_name)
{
	if (luastate == NULL) return FALSE;

	lua_getglobal(luastate, table_name);

	if (lua_isnil(luastate, -1)) return FALSE;

	return lua_istable(luastate, -1);
}

static gchar * get_single_script_name(lua_State *luastate, gchar *table_name)
{
	if (luastate == NULL) return NULL;

	lua_getglobal(luastate, table_name);

	if (lua_isnil(luastate, -1)) return NULL;

	if (lua_isstring(luastate, -1) == FALSE) return NULL;

	return (gchar *)lua_tostring(luastate, -1);
}

/**
 *
 */
static GSList *get_table_of_strings(lua_State *luastate,
                                    gchar *script_folder,
                                    gchar *table_name)
{
	GSList *list = NULL;

	if (get_lua_table(luastate, table_name)) {

		lua_pushnil(luastate);

		while(lua_next(luastate, -2)) {
			if (lua_isstring(luastate, -1)) {
				char *temp = (char *)lua_tostring(luastate, -1);

				list = add_lua_file_to_list(list, script_folder, temp);
			}
			lua_pop(luastate, 1);
		}
		lua_pop(luastate, 1);
	}
	else {
		gchar * oneFileString = get_single_script_name(luastate, table_name);
		if(oneFileString != NULL && strlen(oneFileString) > 0)
		{
			list = add_lua_file_to_list(list, script_folder, oneFileString);
		}
	}

	return list;
}


/**
 *  is_in_list
 * Go through _one_ list, and check if the filename is in this list
 */
static gboolean is_in_list(GSList *list, const gchar *filename)
{
	gboolean result = FALSE;

	if (list) {
		GSList *temp_list = list;

		while (temp_list) {
			gchar *list_filename = (gchar*)temp_list->data;
			if (list_filename) {

				if (g_ascii_strcasecmp(list_filename, filename) == 0) {
					result = TRUE;
				}
			}
			temp_list = temp_list->next;
		}
	}

	return result;
}


/**
 *  is_in_any_list
 * Go through our lists, and check if the file is already in any of them
 */
gboolean is_in_any_list(const gchar *filename)
{
	win_event_type i;

	for (i=0; i < W_NUM_EVENTS; i++) {
		if (is_in_list(event_lists[i], filename))
			return TRUE;
	}

	return FALSE;
}

/**
 * To support the use of 'require' in user scripts, the greedy loading and assigning to the 'open'
 * event needs to be suppressed.  'scripts_window_open' controls this suppression.
 * If it's defined at all, the greedy loading will be suppressed.
 * The variable can be a single file ref or multiple, or defined to be empty (table or string) 
 */
static gboolean should_greedy_load_scripts(lua_State *luastate)
{
	if(event_lists[W_OPEN] != NULL) return FALSE; // Multiple files were listed
	if(get_lua_table(luastate, "scripts_window_open")) return FALSE; // Defined, but was an empty table
	if(get_single_script_name(luastate, "scripts_window_open")) return FALSE; // Defined as an empty string

	return TRUE;
}

/**
 *  load_config
 * Load configuration from a file - From this we set up the lists of files
 * which decides what script to load on what wnck event.
 */
int load_config(gchar *filename)
{
	lua_State *config_lua_state = NULL;
	int result = 0;
	const gchar *current_file = NULL;
	GSList *temp_window_open_file_list = NULL;

	// First get list of Lua files in folder - Then read variables from
	// devilspie2.lua and put the files in the required lists.

	gchar *script_folder = g_path_get_dirname(filename);

	GDir *dir = g_dir_open(script_folder, 0, NULL);
	if (!g_file_test(script_folder, G_FILE_TEST_IS_DIR)) {

		printf("%s\n", _("script_folder isn't a folder."));
		return -1;
	}

	config_lua_state = init_script(script_folder);

	if (g_file_test(filename, G_FILE_TEST_EXISTS)) {

		if (run_script(config_lua_state, filename) != 0) {
			printf(_("Error: %s\n"), filename);
			result = -1;
			goto EXITPOINT;
		}

		event_lists[W_OPEN] = get_table_of_strings(config_lua_state,
		                         script_folder,
		                         "scripts_window_open");
		event_lists[W_CLOSE] = get_table_of_strings(config_lua_state,
		                         script_folder,
		                         "scripts_window_close");
		event_lists[W_FOCUS] = get_table_of_strings(config_lua_state,
		                         script_folder,
		                         "scripts_window_focus");
		event_lists[W_BLUR]  = get_table_of_strings(config_lua_state,
		                         script_folder,
		                         "scripts_window_blur");
		event_lists[W_NAME_CHANGED] = get_table_of_strings(config_lua_state,
		                         script_folder,
		                         "scripts_window_name_change");
	}

	/*
	Allow the user to specify a limited set of files, as there might be .lua files being used in
	"require"s or some other files the user may not want to be executed.
	*/
	if(should_greedy_load_scripts(config_lua_state))
	{
		// add the files in the folder to our linked list
		while ((current_file = g_dir_read_name(dir))) {

			// we only bother with *.lua in the folder
			// we also ignore dot files
			if (current_file[0] != '.' && g_str_has_suffix(current_file, ".lua")) {
				if (!is_in_any_list(current_file)) {
					temp_window_open_file_list =
						add_lua_file_to_list(temp_window_open_file_list, script_folder, current_file);
				}
			}
		}

		event_lists[W_OPEN] = temp_window_open_file_list;
	}
EXITPOINT:
	if (config_lua_state)
		done_script(config_lua_state);

	return result;
}



/**
 *
 */
static void unallocate_file_list(GSList *file_list)
{
	if (file_list) {
		while(file_list) {
			g_free ((gchar*)file_list->data);
			file_list = file_list->next;
		}
	}
}


/**
 *
 */
void clear_file_lists()
{
	win_event_type i = 0;

	for (i = 0; i < W_NUM_EVENTS; i++) {
		if (event_lists[i]) {
			unallocate_file_list(event_lists[i]);
			g_slist_free(event_lists[i]);
			event_lists[i] = NULL;
		}
	}
}
