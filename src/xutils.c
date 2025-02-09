/**
 *	This file is part of devilspie2
 *	Copyright (C) 2001 Havoc Pennington, 2011-2019 Andreas Rönnquist
 *	Copyright (C) 2019-2025 Darren Salt
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
#include <X11/Xatom.h>

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <string.h>

// FIXME: retrieve screen position via wnck
#include <X11/extensions/Xinerama.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <locale.h>

#include "intl.h"
#include "xutils.h"


#if (GTK_MAJOR_VERSION >= 3)
#define HAVE_GTK3
#endif


static GHashTable *atom_hash = NULL;
static GHashTable *reverse_atom_hash = NULL;


/**
 *
 */
Atom my_wnck_atom_get(const char *atom_name)
{
	Atom retval;

	g_return_val_if_fail (atom_name != NULL, None);

	if (!atom_hash) {
		atom_hash = g_hash_table_new (g_str_hash, g_str_equal);
		reverse_atom_hash = g_hash_table_new (NULL, NULL);
	}

	retval = GPOINTER_TO_UINT (g_hash_table_lookup (atom_hash, atom_name));
	if (!retval) {
		retval = XInternAtom (gdk_x11_get_default_xdisplay(), atom_name, FALSE);

		if (retval != None) {
			char *name_copy;

			name_copy = g_strdup (atom_name);

			g_hash_table_insert (atom_hash,
			                     name_copy,
			                     GUINT_TO_POINTER (retval));
			g_hash_table_insert (reverse_atom_hash,
			                     GUINT_TO_POINTER (retval),
			                     name_copy);
		}
	}
	return retval;
}


/**
 *
 */
void devilspie2_change_state(Screen *screen, Window xwindow,
                             gboolean add,
                             Atom     state1,
                             Atom     state2)
{
	XEvent xev;

#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */

	xev.xclient.type = ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = True;
	xev.xclient.display = gdk_x11_get_default_xdisplay();
	xev.xclient.window = xwindow;
	xev.xclient.message_type = my_wnck_atom_get ("_NET_WM_STATE");
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = add ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
	xev.xclient.data.l[1] = state1;
	xev.xclient.data.l[2] = state2;

	XSendEvent (gdk_x11_get_default_xdisplay(),
	            RootWindowOfScreen (screen),
	            False,
	            SubstructureRedirectMask | SubstructureNotifyMask,
	            &xev);
}


/**
 *
 */
void devilspie2_error_trap_push()
{
#if GTK_CHECK_VERSION(3, 0, 0)
	gdk_x11_display_error_trap_push(gdk_display_get_default());
#else
	gdk_error_trap_push();
#endif
}


/**
 *
 */
int devilspie2_error_trap_pop()
{
#if GTK_CHECK_VERSION(3, 0, 0)
	return gdk_x11_display_error_trap_pop(gdk_display_get_default());
#else
	XSync(gdk_x11_get_default_xdisplay(),False);
	return gdk_error_trap_pop();
#endif
}


/**
 *
 */
static void set_decorations(Window xid /*WnckWindow *window*/, gboolean decorate)
{
#define PROP_MOTIF_WM_HINTS_ELEMENTS 5
#define MWM_HINTS_DECORATIONS (1L << 1)
	struct {
		unsigned long flags;
		unsigned long functions;
		unsigned long decorations;
		long inputMode;
		unsigned long status;
	} hints = {0,};

	hints.flags = MWM_HINTS_DECORATIONS;
	hints.decorations = decorate ? 1 : 0;

	/* Set Motif hints, most window managers handle these */
	XChangeProperty(gdk_x11_get_default_xdisplay(), xid /*wnck_window_get_xid (window)*/,
	                my_wnck_atom_get ("_MOTIF_WM_HINTS"),
	                my_wnck_atom_get ("_MOTIF_WM_HINTS"), 32, PropModeReplace,
	                (unsigned char *)&hints, PROP_MOTIF_WM_HINTS_ELEMENTS);


	//Window   xid;
	XWindowAttributes attrs;

	//xid = wnck_window_get_xid (window);
	XGetWindowAttributes(gdk_x11_get_default_xdisplay(), xid, &attrs);

	/* Apart from OpenBox, which doesn't respect it changing after mapping.
	 * Instead it has this workaround.
	 */
	devilspie2_change_state (attrs.screen,
	                         xid /*wnck_window_get_xid(window)*/, !decorate,
	                         my_wnck_atom_get ("_OB_WM_STATE_UNDECORATED"), 0);

}


/**
 *
 */
gboolean decorate_window(Window xid)
{
	devilspie2_error_trap_push();

	set_decorations(xid, TRUE);

	if (devilspie2_error_trap_pop()) {
		g_printerr("decorate_window %s\n", _("Failed!"));
		return FALSE;
	}

	return TRUE;
}


/**
 *
 */
gboolean undecorate_window(Window xid)
{
	devilspie2_error_trap_push();

	set_decorations(xid, FALSE);

	if (devilspie2_error_trap_pop()) {
		g_printerr("decorate_window %s\n", _("Failed!"));
		return FALSE;
	}

	return TRUE;
}


/**
 *
 */
gboolean get_decorated(Window xid /*WnckWindow *window*/)
{
	Display *disp = gdk_x11_get_default_xdisplay();
	Atom type_ret;
	Atom hints_atom = XInternAtom(disp, "_MOTIF_WM_HINTS", False);
	int format_ret;
	int err, result = 0;
	unsigned long nitems_ret, bytes_after_ret, *prop_ret;

	devilspie2_error_trap_push();
	XGetWindowProperty(disp, xid, hints_atom, 0,
	                PROP_MOTIF_WM_HINTS_ELEMENTS, 0, hints_atom,
	                &type_ret, &format_ret, &nitems_ret,
	                &bytes_after_ret, (unsigned char **)&prop_ret);

	err = devilspie2_error_trap_pop ();
	if (err != Success || result != Success)
		return FALSE;

	return type_ret != hints_atom || nitems_ret < 3 || prop_ret[2] != 0;
}


/**
 *
 */
Screen *devilspie2_window_get_xscreen(Window xid)
{
	XWindowAttributes attrs;

	XGetWindowAttributes(gdk_x11_get_default_xdisplay(), xid, &attrs);

	return attrs.screen;
}


/**
 *
 */
char* my_wnck_get_string_property(Window xwindow, Atom atom, gboolean *utf8)
{
	Atom type;
	int format;
	gulong nitems;
	gulong bytes_after;
	unsigned char *property;
	int err, result;
	char *retval;
	Atom XA_UTF8_STRING;
	gboolean is_utf8 = True;

	if (utf8)
		*utf8 = False;

	devilspie2_error_trap_push();
	property = NULL;
	result = XGetWindowProperty (gdk_x11_get_default_xdisplay (),
	                             xwindow, atom,
	                             0, G_MAXLONG,
	                             False, AnyPropertyType, &type,
	                             &format, &nitems,
	                             &bytes_after, &property);

	err = devilspie2_error_trap_pop ();
	if (err != Success || result != Success)
		return NULL;

	retval = NULL;
	XA_UTF8_STRING = XInternAtom(gdk_x11_get_default_xdisplay(), "UTF8_STRING", False);

	if (utf8)
		*utf8 = False;

	if (type == XA_STRING) {
		is_utf8 = False;
		retval = g_strdup ((char*)property);
	} else if (type == XA_UTF8_STRING) {
		retval = g_strdup ((char*)property);
	} else if (type == XA_ATOM && nitems > 0 && format == 32) {
		long *pp;

		pp = (long *)property; // we can assume (long *) since format == 32
		if (nitems == 1) {
			char* prop_name;
			prop_name = XGetAtomName (gdk_x11_get_default_xdisplay (), *pp);
			if (prop_name) {
				retval = g_strdup (prop_name);
				XFree (prop_name);
			}
		} else {
			gulong i;
			char** prop_names;

			prop_names = g_new (char *, nitems + 1);
			prop_names[nitems] = NULL;
			for (i=0; i < nitems; i++) {
				prop_names[i] = XGetAtomName (gdk_x11_get_default_xdisplay (),
				                              *pp++);
			}
			retval = g_strjoinv (", ", prop_names);
			for (i=0; i < nitems; i++) {
				if (prop_names[i]) XFree (prop_names[i]);
			}
			g_free (prop_names);
		}
	} else if (type == XA_CARDINAL && nitems == 1) {
		switch(format) {
		case 32:
			retval = g_strdup_printf("%lu", *(unsigned long*)property);
			break;
		case 16:
			retval = g_strdup_printf("%u", *(unsigned int*)property);
			break;
		case 8:
			retval = g_strdup_printf("%c", *(unsigned char*)property);
			break;
		}
	} else if (type == XA_WINDOW && nitems == 1) {
		/* unsinged long is the same format used for XID by libwnck:
		 * https://git.gnome.org/browse/libwnck/tree/libwnck/window.c?h=3.14.0#n763
		 */
		retval = g_strdup_printf("%lu", (gulong) *(Window *)property);
	}

	XFree (property);
	if (utf8)
		*utf8 = is_utf8;
	return retval;
}


/**
 *
 */
void my_wnck_set_string_property(Window xwindow, Atom atom, const gchar *const string, gboolean utf8)
{
	const unsigned char *const str = (const unsigned char *)string;
	Display *display = gdk_x11_get_default_xdisplay();
	Atom type = utf8 ? XInternAtom(display, "UTF8_STRING", False) : XA_STRING;

	devilspie2_error_trap_push();
	XChangeProperty (display, xwindow, atom, type, 8, PropModeReplace, str, strlen(string));
	devilspie2_error_trap_pop ();
}


/**
 *
 */
void my_wnck_set_cardinal_property(Window xwindow, Atom atom, int32_t value)
{
	devilspie2_error_trap_push();
	XChangeProperty (gdk_x11_get_default_xdisplay (),
	                 xwindow, atom, XA_CARDINAL, 32,
	                 PropModeReplace, (unsigned char *)&value, 1);
	devilspie2_error_trap_pop ();
}


/**
 *
 */
void my_wnck_delete_property(Window xwindow, Atom atom)
{
	devilspie2_error_trap_push();
	XDeleteProperty (gdk_x11_get_default_xdisplay (), xwindow, atom);
	devilspie2_error_trap_pop ();
}


/**
 *
 */
gboolean
my_wnck_get_cardinal_list (Window xwindow, Atom atom,
                           gulong **cardinals, int *len)
{
	Atom type;
	int format;
	gulong nitems;
	gulong bytes_after;
	gulong *nums;
	int err, result;

	*cardinals = NULL;
	*len = 0;

	devilspie2_error_trap_push();
	type = None;
	result = XGetWindowProperty(gdk_x11_get_default_xdisplay (),
	                            xwindow,
	                            atom,
	                            0, G_MAXLONG,
	                            False, XA_CARDINAL, &type, &format, &nitems,
	                            &bytes_after, (void*)&nums);

	err = devilspie2_error_trap_pop();

	if ((err != Success) || (result != Success))
		return FALSE;

	if (type != XA_CARDINAL) {
		XFree (nums);
		return FALSE;
	}

	*cardinals = g_new(gulong, nitems);
	memcpy(*cardinals, nums, sizeof (gulong) * nitems);
	*len = nitems;

	XFree(nums);

	return TRUE;
}


/**
 *	Get viewport start coordinates to the x and y integers,
 * returns 0 on success and non-zero on error.
 */
int devilspie2_get_viewport_start(Window xid, int *x, int *y)
{
	gulong *list;
	int len;

	int result = -1;

	my_wnck_get_cardinal_list(RootWindowOfScreen(devilspie2_window_get_xscreen(xid)),
	                          my_wnck_atom_get("_NET_DESKTOP_VIEWPORT"),
	                          &list, &len);

	if (len > 0) {
		*x = list[0];
		*y = list[1];

		result = 0;
	}

	g_free(list);

	return result;
}


/**
 *
 */
void my_window_set_window_type(Window xid, gchar *window_type)
{
	Display *display = gdk_x11_get_default_xdisplay();

	Atom atoms[10];

	/*
	_NET_WM_WINDOW_TYPE_DESKTOP, ATOM
	_NET_WM_WINDOW_TYPE_DOCK, ATOM
	_NET_WM_WINDOW_TYPE_TOOLBAR, ATOM
	_NET_WM_WINDOW_TYPE_MENU, ATOM
	_NET_WM_WINDOW_TYPE_UTILITY, ATOM
	_NET_WM_WINDOW_TYPE_SPLASH, ATOM
	_NET_WM_WINDOW_TYPE_DIALOG, ATOM
	_NET_WM_WINDOW_TYPE_NORMAL, ATOM
	*/

	gchar *type = NULL;

	//	Make it a recognized _NET_WM_TYPE

	if (g_ascii_strcasecmp(window_type, "WINDOW_TYPE_DESKTOP") == 0) {
		type = g_strdup("_NET_WM_WINDOW_TYPE_DESKTOP");

	} else if (g_ascii_strcasecmp(window_type, "WINDOW_TYPE_DOCK") == 0) {
		type = g_strdup("_NET_WM_WINDOW_TYPE_DOCK");

	} else if (g_ascii_strcasecmp(window_type, "WINDOW_TYPE_TOOLBAR") == 0) {
		type = g_strdup("_NET_WM_WINDOW_TYPE_TOOLBAR");

	} else if (g_ascii_strcasecmp(window_type, "WINDOW_TYPE_MENU") == 0) {
		type = g_strdup("_NET_WM_WINDOW_TYPE_MENU");

	} else if (g_ascii_strcasecmp(window_type, "WINDOW_TYPE_UTILITY") == 0) {
		type = g_strdup("_NET_WM_WINDOW_TYPE_UTILITY");

	} else if (g_ascii_strcasecmp(window_type, "WINDOW_TYPE_SPLASH") == 0) {
		type = g_strdup("_NET_WM_WINDOW_TYPE_SPLASH");

	} else if (g_ascii_strcasecmp(window_type, "WINDOW_TYPE_DIALOG") == 0) {
		type = g_strdup("_NET_WM_WINDOW_TYPE_DIALOG");

	} else if (g_ascii_strcasecmp(window_type, "WINDOW_TYPE_NORMAL") == 0) {
		type = g_strdup("_NET_WM_WINDOW_TYPE_NORMAL");

	} else {
		type = g_strdup(window_type);
	}

	atoms[0] = XInternAtom(display, type, False);

	XChangeProperty(gdk_x11_get_default_xdisplay(), xid,
	                XInternAtom(display, "_NET_WM_WINDOW_TYPE", False), XA_ATOM, 32,
	                PropModeReplace, (unsigned char *) &atoms, 1);

	g_free(type);
}


/**
 *
 */
void my_window_set_opacity(Window xid, double value)
{
	Display *display = gdk_x11_get_default_xdisplay();

	unsigned int opacity = (uint)(0xffffffff * value);
	Atom atom_net_wm_opacity = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", False);


	XChangeProperty(gdk_x11_get_default_xdisplay(), xid,
	                atom_net_wm_opacity, XA_CARDINAL, 32,
	                PropModeReplace, (unsigned char *) &opacity, 1L);

}


/**
 *
 */
void adjust_for_decoration(WnckWindow *window, int *x, int *y, int *w, int *h)
{
	GdkRectangle geom, geom_undec;

	wnck_window_get_geometry(window, &geom.x, &geom.y, &geom.width, &geom.height);
	wnck_window_get_client_window_geometry(window, &geom_undec.x, &geom_undec.y, &geom_undec.width, &geom_undec.height);

	if (x) *x -= geom_undec.x - geom.x;
	if (y) *y -= geom_undec.y - geom.y;
	if (w) *w -= geom_undec.width - geom.width;
	if (h) *h -= geom_undec.height - geom.height;
}


/**
 *
 */
void set_window_geometry(WnckWindow *window, int x, int y, int w, int h, gboolean adjusting_for_decoration)
{
	if (window) {
		WnckScreen *screen = wnck_window_get_screen(window);
		int sw = wnck_screen_get_width(screen);
		int sh = wnck_screen_get_height(screen);

		int gravity = WNCK_WINDOW_GRAVITY_CURRENT;
		if (x >= 0 && y >= 0)
			gravity = WNCK_WINDOW_GRAVITY_NORTHWEST;
		if (x >= 0 && y < 0)
			gravity = WNCK_WINDOW_GRAVITY_SOUTHWEST;
		if (x < 0 && y >= 0)
			gravity = WNCK_WINDOW_GRAVITY_NORTHEAST;
		if (x < 0 && y < 0)
			gravity = WNCK_WINDOW_GRAVITY_SOUTHEAST;
		if (x < 0)
			x = sw + x;
		if (y < 0)
			y = sh + y;

		if (adjusting_for_decoration)
			adjust_for_decoration(window, &x, &y, &w, &h);

		wnck_window_set_geometry(window,
		                         gravity,
		                         WNCK_WINDOW_CHANGE_X +
		                         WNCK_WINDOW_CHANGE_Y +
		                         WNCK_WINDOW_CHANGE_WIDTH +
		                         WNCK_WINDOW_CHANGE_HEIGHT,
		                         x, y, w, h);
	}

}


/**
 *
 */
int get_monitor_count(void)
{
	// FIXME: retrieve monitor count via wnck
	// For now, use Xinerama directly
	Display *dpy = gdk_x11_get_default_xdisplay();

	if (!XineramaIsActive(dpy))
		return 0;

	// Normally, we'd use the return value, but we only want the number of entries
	int monitor_count = 0;
	XineramaQueryScreens(dpy, &monitor_count);

	return monitor_count;
}


/**
 *
 */
int get_monitor_index_geometry(WnckWindow *window, const GdkRectangle *window_r_in, GdkRectangle *monitor_r)
{
	// monitor_r is always filled in unless the return value is -1

	// FIXME: retrieve monitor info via wnck
	// For now, use Xinerama directly
	int id = -1;
	int monitor_count = 0;
	XineramaScreenInfo *monitor_list = NULL;
	Display *dpy = gdk_x11_get_default_xdisplay();

	if (XineramaIsActive(dpy))
		monitor_list = XineramaQueryScreens(dpy, &monitor_count);

	// bail out if no Xinermama or no monitors
	if (!monitor_list || !monitor_count)
		return -1;

	// find which monitor the window's centre is on
	GdkRectangle window_r;
	if (window)
		wnck_window_get_geometry(window, &window_r.x, &window_r.y, &window_r.width, &window_r.height);
	else
		window_r = *window_r_in;

	GdkPoint centre = { window_r.x + window_r.width / 2, window_r.y + window_r.height / 2 };

	for (int i = 0; i < monitor_count; ++i) {
		if (centre.x >= monitor_list[i].x_org &&
		    centre.x <  monitor_list[i].x_org + monitor_list[i].width &&
		    centre.y >= monitor_list[i].y_org &&
		    centre.y <  monitor_list[i].y_org + monitor_list[i].height) {
			id = i;
			break;
		}
	}

	// if that fails, try intersection of rectangles
	// just use the first matching
	// FIXME?: should find whichever shows most of the window (if tied, closest to window centre)
	if (id < 0) {
		for (int i = 0; i < monitor_count; ++i) {
			GdkRectangle r = {
				monitor_list[i].x_org, monitor_list[i].y_org,
				monitor_list[i].x_org + monitor_list[i].width,
				monitor_list[i].y_org + monitor_list[i].height
			};
			if (gdk_rectangle_intersect(&window_r, &r, NULL)) {
				id = i;
				break;
			}
		}
	}

	// and if that too fails, use the default
	if (id < 0)
		id = 0; // FIXME: primary monitor

	if (monitor_r) {
		monitor_r->x = monitor_list[id].x_org;
		monitor_r->y = monitor_list[id].y_org;
		monitor_r->width = monitor_list[id].width;
		monitor_r->height = monitor_list[id].height;
	}

	return id;
}


/**
 *
 */
int get_monitor_geometry(int index, GdkRectangle *monitor_r)
{
	// if out of range, output is for monitor 0 (if present) else this:
	*monitor_r = (GdkRectangle){ 0, 0, 640, 480 };

	// FIXME: retrieve monitor info via wnck
	// For now, use Xinerama directly
	int monitor_count = 0;
	XineramaScreenInfo *monitor_list = NULL;
	Display *dpy = gdk_x11_get_default_xdisplay();

	if (XineramaIsActive(dpy))
		monitor_list = XineramaQueryScreens(dpy, &monitor_count);

	// bail out if no Xinermama or no monitors
	if (!monitor_list || !monitor_count)
		return -1; // no xinerama!

	// FIXME: default to primary monitor
	if (index < 0 || index >= monitor_count)
		index = 0;

	monitor_r->x = monitor_list[index].x_org;
	monitor_r->y = monitor_list[index].y_org;
	monitor_r->width = monitor_list[index].width;
	monitor_r->height = monitor_list[index].height;

	return index;
}


/**
 *
 */
int get_window_workspace_geometry(WnckWindow *window, GdkRectangle *geom)
{
	WnckScreen *screen = wnck_window_get_screen(window);
	WnckWorkspace *workspace = wnck_screen_get_active_workspace(screen);

	if (workspace == NULL) {
		workspace = wnck_screen_get_workspace(screen, 0);
	}

	if (workspace == NULL) {
		g_printerr(_("Could not get workspace"));
		return 1;
	}

	geom->x = 0;
	geom->y = 0;
	geom->width = wnck_workspace_get_width(workspace);
	geom->height = wnck_workspace_get_height(workspace);

	return 0;
}


/**
 * Wrapper for the above geometry-reading functions
 * Selects according to monitor number
 * Returns the monitor index, MONITOR_ALL or, on error, MONITOR_NONE
 */
int get_monitor_or_workspace_geometry(int monitor_no, WnckWindow *window, GdkRectangle *bounds)
{
	int ret;

	switch (monitor_no)
	{
	case MONITOR_ALL:
		return get_window_workspace_geometry(window, bounds) ? MONITOR_NONE : MONITOR_ALL;

	case MONITOR_WINDOW:
		ret = get_monitor_index_geometry(window, NULL, bounds);
		return ret < 0 ? MONITOR_NONE : ret;

	default:
		if (monitor_no < 0 || monitor_no >= get_monitor_count())
			return MONITOR_NONE;
		return get_monitor_geometry(monitor_no, bounds) < 0 ? MONITOR_NONE : monitor_no;
	}
}
