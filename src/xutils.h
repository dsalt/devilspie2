/**
 *
 *	This file is part of devilspie2.
 *	Copyright (C) 2001 Havoc Pennington, 2011 Andreas Rönnquist
 *
 *	devilspie2 is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU Lesser General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	devilspie2 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU Lesser General Public License for more details.
 *
 *	You should have received a copy of the GNU Lesser General Public License
 *	along with devilspie2.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef __HEADER_XUTILS_
#define __HEADER_XUTILS_

/**
 *
 */


Atom my_wnck_atom_get  (const char *atom_name);

void my_wnck_change_state (Screen *screen, Window xwindow, gboolean add, Atom state1, Atom state2);

Screen* my_wnck_window_get_xscreen (WnckWindow *window);

void my_wnck_error_trap_push();
int my_wnck_error_trap_pop();

//static void set_decorations (WnckWindow *window, gboolean decorate);

gboolean decorate_window(WnckWindow *window);
gboolean undecorate_window(WnckWindow *window);

#endif /*__HEADER_XUTILS_*/