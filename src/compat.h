/**
 *	This file is part of devilspie2
 *	Copyright (C) 2025 Darren Salt
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

#ifndef __HEADER_COMPAT_
#define __HEADER_COMPAT_

#if defined(__GNUC__) || defined(__clang__)
# define ATTR_MALLOC __attribute__((malloc))
#else
# define ATTR_MALLOC
#endif

#endif /* __HEADER_COMPAT_ */
