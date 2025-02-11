/**
 *	This file is part of devilspie2
 *	Copyright (C) 2005 Ross Burton, 2011-2017 Andreas Rönnquist
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

#include <string.h>

#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <locale.h>

#include "script.h"
#include "script_functions.h"

#include "error_strings.h"

#include "config.h"


#if (GTK_MAJOR_VERSION >= 3)
#define HAVE_GTK3
#endif

/**
 *
 */
GMainLoop *loop = NULL;

static gboolean debug = FALSE;
static gboolean emulate = FALSE;
static gboolean show_version = FALSE;

// libwnck Version Information is only availible if you have
// libwnck 3.0 or later
static gboolean show_wnck_version = FALSE;

static gboolean show_lua_version = FALSE;

static gchar *script_folder = NULL;
static gchar *temp_folder = NULL;

GFileMonitor *mon = NULL;

gchar *config_filename = NULL;

WnckHandle *my_wnck_handle = NULL;

/**
 *
 */
static void load_list_of_scripts(WnckScreen *screen G_GNUC_UNUSED, WnckWindow *window,
                                 GSList *file_list)
{
	GSList *temp_file_list = file_list;
	// set the window to work on
	set_current_window(window);

	// for every file in the folder - load the script
	if (event_lists[W_OPEN] != NULL) {

		while(temp_file_list) {
			gchar *filename = (gchar*)temp_file_list->data;

			// is it a Lua file?
			if (g_str_has_suffix((gchar*)filename, ".lua")) {

				// init the script, run it
				if (!run_script(global_lua_state, filename))
					/**/;

			}
			temp_file_list=temp_file_list->next;
		}
	}
	return;

}


static void window_name_changed_cb(WnckWindow *window)
{
	WnckScreen * screen = wnck_window_get_screen(window);
	if(screen == NULL) return;

	// Handle duplicate name-change events
	// Simple method: just track the most recent event regardless of window
	static WnckWindow *previous = NULL;
	static char *prevname = NULL;

	const char *newname = wnck_window_get_name(window);
	if (window == previous && prevname && !strcmp (prevname, newname))
		return;
	// Store the info for the next event
	free(prevname);
	prevname = strdup(newname);
	previous = window;

	load_list_of_scripts(screen, window, event_lists[W_NAME_CHANGED]);
}

/**
 *
 */
static void window_opened_cb(WnckScreen *screen, WnckWindow *window)
{
	load_list_of_scripts(screen, window, event_lists[W_OPEN]);
	/*
	Attach a listener to each window for window-specific changes
	Safe to do this way as long as the 'user data' parameter is NULL
	*/
	g_signal_connect(window, "name-changed", (GCallback)window_name_changed_cb, NULL);
}


/**
 *
 */
static void window_closed_cb(WnckScreen *screen, WnckWindow *window)
{
	load_list_of_scripts(screen, window, event_lists[W_CLOSE]);
}


/**
 *
 */
static void window_changed_cb(WnckScreen *screen, WnckWindow *window)
{
	WnckWindow *cur;

	load_list_of_scripts(screen, window, event_lists[W_BLUR]);
	cur = wnck_screen_get_active_window(screen);
	load_list_of_scripts(screen, cur, event_lists[W_FOCUS]);
}


/**
 *
 */
void init_screens()
{
	int i;
	int num_screens;

#ifndef GDK_VERSION_3_10
	num_screens = gdk_display_get_n_screens(gdk_display_get_default());
#else
	num_screens = 1;
#endif

	for (i=0; i<num_screens; i++) {
		WnckScreen *screen = wnck_handle_get_screen(my_wnck_handle, i);

		g_signal_connect(screen, "window-opened",
		                 (GCallback)window_opened_cb, NULL);
		g_signal_connect(screen, "window-closed",
		                 (GCallback)window_closed_cb, NULL);
		g_signal_connect(screen, "active-window-changed",
		                 (GCallback)window_changed_cb, NULL);
	}
}


/**
 * atexit handler - kill the script
 */
void devilspie_exit()
{
	clear_file_lists();
	g_free(temp_folder);
	if (mon)
		g_object_unref(mon);
	g_free(config_filename);
}


/**
 * handle signals that are sent to the application
 */
static void signal_handler(int sig)
{
	printf("\n%s %d (%s)\n", _("Received signal:"), sig, strsignal(sig));

	done_script_error_messages();

	if (sig == SIGINT) {
		exit(EXIT_FAILURE);
	}
}


/**
 *
 */
void print_list(GSList *list)
{
	GSList *temp_list;
	if (list != NULL) {
		temp_list = list;

		while(temp_list) {
			gchar *file_name = temp_list->data;

			if (file_name) {
				if (g_str_has_suffix((gchar*)file_name, ".lua")) {
					printf("%s\n", (gchar*)file_name);
				}
			}
			temp_list = temp_list->next;
		}
	}
}


/**
 *
 */
void print_script_lists()
{
	gboolean have_any_files = FALSE;
	win_event_type i;

	if (debug)
		printf("------------\n");

	for (i = 0; i < W_NUM_EVENTS; i++) {
		if (event_lists[i])
			have_any_files = TRUE;
		// If we are running debug mode - print the list of files:
		if (debug) {
			printf(_("List of Lua files handling \"%s\" events in folder:"),
				   event_names[i]);
			printf("\n");
			if (event_lists[i]) {
				print_list(event_lists[i]);
			}
		}
	}

	if (!have_any_files) {
		printf("%s\n\n", _("No script files found in the script folder - exiting."));
		exit(EXIT_SUCCESS);
	}
}


/**
 *
 */
void folder_changed_callback(GFileMonitor *mon G_GNUC_UNUSED,
                             GFile *first_file,
                             GFile *second_file G_GNUC_UNUSED,
                             GFileMonitorEvent event,
                             gpointer user_data)
{
	gchar *our_filename = (gchar*)(user_data);

	// If a file is created or deleted, we need to check the file lists again
	if ((event == G_FILE_MONITOR_EVENT_CREATED) ||
	    (event == G_FILE_MONITOR_EVENT_DELETED)) {

		clear_file_lists();

		set_current_window(NULL);
		load_config(our_filename);

		if (debug)
			printf("Files in folder updated!\n - new lists:\n\n");

		print_script_lists();

		if (debug)
			printf("-----------\n");
	}

	// Also monitor if our devilspie2.lua file is changed - since it handles
	// which files are window close or window open scripts.
	if (event == G_FILE_MONITOR_EVENT_CHANGED) {
		if (first_file) {
			gchar *short_filename = g_file_get_basename(first_file);

			if (g_strcmp0(short_filename, "devilspie2.lua")==0) {

				clear_file_lists();

				set_current_window(NULL);
				load_config(our_filename);

				print_script_lists();

				if (debug)
					printf("----------");
			}
		}
	}
}


/**
 * Program main entry
 */
int main(int argc, char *argv[])
{
	static const GOptionEntry options[]= {
		{ "debug",        'd', 0, G_OPTION_ARG_NONE,   &debug,
		  N_("Print debug info to stdout"), NULL
		},
		{ "emulate",      'e', 0, G_OPTION_ARG_NONE,   &emulate,
		  N_("Don't apply any rules, only emulate execution"), NULL
		},
		{ "folder",       'f', 0, G_OPTION_ARG_STRING, &script_folder,
		  N_("Search for scripts in this folder"), N_("FOLDER")
		},
		{ "version",      'v', 0, G_OPTION_ARG_NONE,   &show_version,
		  N_("Show Devilspie2 version and quit"), NULL
		},
		// libwnck Version Information is only availible if you have
		// libwnck 3.0 or later
		{ "wnck-version", 'w', 0, G_OPTION_ARG_NONE,   &show_wnck_version,
		  N_("Show libwnck version and quit"), NULL
		},
		{ "lua-version",  'l', 0, G_OPTION_ARG_NONE,   &show_lua_version,
		  N_("Show Lua version and quit"), NULL
		},
		{ NULL }
	};

	GError *error = NULL;
	GOptionContext *context;

	// Init gettext stuff
	setlocale(LC_ALL, "");

	bindtextdomain(PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "");
	textdomain(PACKAGE);

	gchar *devilspie2_description = g_strdup_printf(_("apply rules on windows"));

	gchar *full_desc_string = g_strdup_printf("- %s", devilspie2_description);

	context = g_option_context_new(full_desc_string);
	g_option_context_add_main_entries(context, options, NULL);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print(_("option parsing failed: %s"), error->message);
		printf("\n");
		exit(EXIT_FAILURE);
	}
	gboolean shown = FALSE;
	if (show_version) {
		printf("Devilspie2 v%s\n", DEVILSPIE2_VERSION);
		shown = TRUE;
	}
	// libwnck Version Information is only availible if you have
	// libwnck 3.0 or later
	if (show_wnck_version) {
#ifdef _DEBUG
		printf("GTK v%d.%d.%d\n",
		       GTK_MAJOR_VERSION,
		       GTK_MINOR_VERSION,
		       GTK_MICRO_VERSION);
#endif
#ifdef HAVE_GTK3
		printf("libwnck v%d.%d.%d\n",
		       WNCK_MAJOR_VERSION,
		       WNCK_MINOR_VERSION,
		       WNCK_MICRO_VERSION);
#else
		printf("libwnck v2.x\n");
#endif
		shown = TRUE;
	}
	if (show_lua_version) {
		puts(LUA_VERSION);
		shown = TRUE;
	}
	if (shown)
		exit(0);

	gdk_init(&argc, &argv);

	g_free(full_desc_string);
	g_free(devilspie2_description);

	// if the folder is NULL, default to ~/.config/devilspie2/
	if (script_folder == NULL) {

		temp_folder = g_build_path(G_DIR_SEPARATOR_S,
		                           g_get_user_config_dir(),
		                           "devilspie2",
		                           NULL);

		// check if the folder does exist
		if (!g_file_test(temp_folder, G_FILE_TEST_IS_DIR)) {

			// - and if it doesn't, create it.
			if (g_mkdir(temp_folder, 0700) != 0) {
				printf("%s\n", _("Couldn't create the default folder for devilspie2 scripts."));
				exit(EXIT_FAILURE);
			}
		}

		script_folder = temp_folder;
	}


#if (GTK_MAJOR_VERSION >= 3)
	if (!GDK_IS_X11_DISPLAY(gdk_display_get_default())) {
		puts(_("An X11 display is required for devilspie2."));
		if (getenv("WAYLAND_DISPLAY"))
			puts(_("Wayland & XWayland are not supported.\nSee https://github.com/dsalt/devilspie2/issues/7"));
		puts("");
		return EXIT_FAILURE;
	}
#endif

	if (init_script_error_messages()!=0) {
		printf("%s\n", _("Couldn't init script error messages!"));
		exit(EXIT_FAILURE);
	}

	// set the current window to NULL, we don't need to be able to modify
	// the windows when reading the config
	set_current_window(NULL);

	config_filename =
	    g_build_filename(script_folder, "devilspie2.lua", NULL);

	if (load_config(config_filename) != 0) {

		devilspie_exit();
		return EXIT_FAILURE;
	}

	if (debug) {

		if (emulate) {
			printf("%s\n\n", _("Running devilspie2 in debug and emulate mode."));
		} else {
			printf("%s\n\n", _("Running devilspie2 in debug mode."));
		}

		printf(_("Using scripts from folder: %s"), script_folder);

		printf("\n");

		devilspie2_debug = TRUE;
	}

	// Should we only run an emulation (don't modify any windows)
	if (emulate) devilspie2_emulate = emulate;

	GFile *directory_file;
	directory_file = g_file_new_for_path(script_folder);
//	mon = g_file_monitor_directory(directory_file, G_FILE_MONITOR_WATCH_MOUNTS,
	mon = g_file_monitor_directory(directory_file, G_FILE_MONITOR_NONE,
	                               NULL, NULL);
	if (!mon) {
		printf("%s\n", _("Couldn't create directory monitor!"));
		return EXIT_FAILURE;
	}

	g_signal_connect(mon, "changed", G_CALLBACK(folder_changed_callback),
	                 (gpointer)(config_filename));

	global_lua_state = init_script();
	print_script_lists();

	if (debug) printf("------------\n");

	// remove stuff cleanly
	atexit(devilspie_exit);

	struct sigaction signal_action;

	sigemptyset(&signal_action.sa_mask);
	signal_action.sa_flags = 0;
	signal_action.sa_handler = signal_handler;

	if (sigaction(SIGINT, &signal_action, NULL) == -1) {
		exit(EXIT_FAILURE);
	}

	my_wnck_handle = wnck_handle_new(WNCK_CLIENT_TYPE_PAGER);
	init_screens();

	loop=g_main_loop_new(NULL, TRUE);
	g_main_loop_run(loop);

	return EXIT_SUCCESS;
}
