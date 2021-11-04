#pragma once

/*
 *   This file is part of open_agb_firm
 *   Copyright (C) 2021 derrek, profi200
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


// Note on port numbers:
// To make things easier 2 ports are assigned to each controller.
// There are a maximum of 2 controllers mapped at the same time
// and 3 (on DSi 2) controllers in total.
// Also see /regs/toshsd.h.
//
// Examples:
// Port 0 is port 0 on controller 1, port 3 is port 1 on controller 2.
#ifdef _3DS
#define TOSHSD_C3_MAP     (0u) // Controller 3 memory mapping. 0=ARM9 0x10007000 or 1=ARM11 0x10100000.

#ifdef ARM9
#define TOSHSD_SLOT_PORT  (2u) // Can be on port 0 or 2.
#define TOSHSD_eMMC_PORT  (1u) // Port 1 only.
#elif ARM11
#define TOSHSD_SLOT_PORT  (2u) // Port 2 only.
#define TOSHSD_eMMC_PORT  (3u) // Not connected/accessible.
#endif // #ifdef ARM9

#elif TWL

#define TOSHSD_SLOT_PORT  (0u)
#define TOSHSD_eMMC_PORT  (1u)
#endif // #ifdef _3DS
