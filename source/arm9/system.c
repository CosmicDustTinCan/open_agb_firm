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

#include "types.h"
#include "arm9/hardware/interrupt.h"
#include "arm9/hardware/ndma.h"
#include "hardware/corelink_dma-330.h"
#include "arm9/hardware/timer.h"
#include "hardware/pxi.h"
#include "arm9/hardware/crypto.h"



void WEAK __systemInit(void)
{
	IRQ_init();
	leaveCriticalSection(0); // Enables interrupts
	TIMER_init();
	NDMA_init();
	//DMA330_init();
	//AES_init();
	//RSA_init();
	PXI_init();
}

void WEAK __systemDeinit(void)
{
	NDMA_init();
	IRQ_init();
}
