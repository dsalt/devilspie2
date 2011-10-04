/**
 *
 *	This file is part of devilspie2.
 *	Copyright 2011 Andreas Rönnquist
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
#ifndef __HEADER_SCRIPT_
#define __HEADER_SCRIPT_


/**
 *
 */
void init_script();
void done_script();

void register_cfunctions();

void run_script();
int load_script(char *filename);

extern gboolean devilspie2_debug;
extern gboolean devilspie2_emulate;

#endif /*__HEADER_SCRIPT_*/