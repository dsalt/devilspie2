/**
 *	This file is part of devilspie2
 *	Copyright (C) 2024 Darren Salt
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

#ifndef __HEADER_LOGGER_
#define __HEADER_LOGGER_

#include <lua.h>
#include "compat.h"

int logger_create(lua_State *);
char *logger_get_fifo_name(void);
void logger_print(const char *text);
void logger_print_always(const char *text);
void logger_printf(const char *format, ...) ATTR_FORMAT_PRINTF(1, 2);
void logger_err_print(const char *text);
void logger_err_printf(const char *format, ...) ATTR_FORMAT_PRINTF(1, 2);
void logger_shutdown(void);

#endif
