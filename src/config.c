/**
 *	This file is part of devilspie2
 *	Copyright (C) 2013-2015 Andreas Rönnquist
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
GSList *file_window_open_list = NULL;
GSList *file_window_close_list = NULL;


/**
 * filename_list_sortfunc
 *   function to sort the inserted filenames, to be able to determine
 *   which order files are loaded.
 */
gint filename_list_sortfunc(gconstpointer a,gconstpointer b)
{
	gchar *file1 = (gchar*)a;
	gchar *file2 = (gchar*)b;

	return g_ascii_strcasecmp(file1, file2);
}


/**
 *
 */
GSList *add_lua_file_to_list(GSList *list, gchar *filename)
{
	gchar *temp_filename = g_strdup(filename);

	list=g_slist_insert_sorted(list,
	                           temp_filename,
	                           filename_list_sortfunc);

	return list;
}



/**
 *
 */
GSList *get_table_of_strings(lua_State *luastate,
                             gchar *script_folder,
                             gchar *table_name)
{
	GSList *list=NULL;

	if (luastate) {

		lua_getglobal(luastate, table_name);

		// Do we have a value?
		if (lua_isnil(luastate, -1)) {
			goto EXITPOINT;
		}

		// Is it a table?
		if (!lua_istable(luastate, -1)) {
			goto EXITPOINT;
		}

		lua_pushnil(luastate);

		while(lua_next(luastate, -2)) {
			if (lua_isstring(luastate, -1)) {
				char *temp = (char *)lua_tostring(luastate, -1);

				gchar *added_filename = g_build_path(G_DIR_SEPARATOR_S,
				                                     script_folder,
				                                     temp,
				                                     NULL);

				list = add_lua_file_to_list(list, added_filename);
			}
			lua_pop(luastate, 1);
		}
		lua_pop(luastate, 1);
	}

EXITPOINT:

	return list;
}


/**
 *  is_in_list
 * Go through _one_ list, and check if the filename is in this list
 */
gboolean is_in_list(GSList *list, gchar *filename)
{
	gboolean result=FALSE;

	if (list) {
		GSList *temp_list = list;

		while (temp_list) {
			gchar *list_filename = (gchar*)temp_list->data;
			if (list_filename) {

				if (g_ascii_strcasecmp(list_filename, filename)==0) {
					result=TRUE;
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
gboolean is_in_any_list(gchar *filename)
{
	gboolean result = FALSE;

	if (is_in_list(file_window_close_list, filename)) result = TRUE;

	return result;
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

		printf("%s", _("script_folder isn't a folder."));
		printf("\n");
		return -1;
	}

	int total_number_of_files = 0;

	config_lua_state = init_script();

	if (g_file_test(filename, G_FILE_TEST_EXISTS)) {

		if (load_script(config_lua_state, filename)!=0) {
			printf(_("Error: %s"), filename);
			printf("\n");
			result = -1;
			goto EXITPOINT;
		}

		run_script(config_lua_state);

		file_window_close_list = get_table_of_strings(config_lua_state,
		                         script_folder,
		                         "scripts_window_close");
	}

	// add the files in the folder to our linked list
	while ((current_file = g_dir_read_name(dir))) {

		gchar *temp_filename = g_build_path(G_DIR_SEPARATOR_S,
		                                    script_folder,
		                                    current_file,
		                                    NULL);

		// we only bother with *.lua in the folder
		if (g_str_has_suffix(current_file, ".lua")) {

			if (!is_in_any_list(temp_filename)) {
				temp_window_open_file_list =
				    add_lua_file_to_list(temp_window_open_file_list, temp_filename);
			}
			total_number_of_files++;
		}

		g_free(temp_filename);
	}

	file_window_open_list = temp_window_open_file_list;
EXITPOINT:
	if (config_lua_state)
		done_script(config_lua_state);

	return result;
}



/**
 *
 */
void unalloacte_file_list(GSList *file_list)
{
	if (file_list) {

		while(file_list) {

			gchar *file_name = (gchar*)file_list->data;

			if (file_name) g_free(file_name);

			file_list = file_list->next;
		}
	}
}


/**
 *
 */
void clear_file_lists()
{
	if (file_window_open_list) {
		unalloacte_file_list(file_window_open_list);
	}

	if (file_window_close_list) {
		unalloacte_file_list(file_window_close_list);
	}
}
