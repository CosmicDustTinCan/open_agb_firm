#include "types.h"
#include "arm11/drivers/lgyfb.h"
#include "arm11/drivers/interrupt.h"
#include "drivers/corelink_dma-330.h"
#include "arm11/drivers/lcd.h"
#if OAF_SCALE_SELECT < 2
#include "lgyfb_240x160_dma330.h"
#else
#include "lgyfb_360x240_dma330.h"
#endif
#include "kevent.h"


static KHandle g_frameReadyEvent = 0;



static void lgyFbDmaIrqHandler(UNUSED u32 intSource)
{
	DMA330_ackIrq(0);
	DMA330_run(0, program);

	// We can't match the GBA refreshrate exactly so keep the LCDs around 90%
	// ahead of the GBA output which gives us a time window of around 1.6 ms to
	// render the frame and hopefully reduces output lag as much as possible.
	u32 vtotal;
	if(REG_LCD_PDC0_VPOS > 414 - 41) vtotal = 415; // Slower than GBA.
	else                             vtotal = 414; // Faster than GBA.
	REG_LCD_PDC0_VTOTAL = vtotal;

	signalEvent(g_frameReadyEvent, false);
}

static void setScaleMatrix(LgyFbScaler *const regs, u32 len, u32 patt, const s16 *const inMatrix, u8 inBits, u8 outBits)
{
	regs->len  = len - 1;
	regs->patt = patt;

	const u8 inMax = (inBits == 0 ? 0u : (0xFF00u>>inBits) & 0xFFu);
	const u8 outMax = (1u<<outBits) - 1;
	vu32 (*const outMatrix)[8] = regs->matrix;
	for(u32 y = 0; y < 6; y++)
	{
		for(u32 x = 0; x < len; x++)
		{
			const s32 mEntry = inMatrix[len * y + x];

			// Correct the color range using the scale matrix hardware.
			// For example when converting RGB555 to RGB8 LgyFb lazily shifts the 5 bits up
			// so 0b00011111 becomes 0b11111000. This creates wrong spacing between colors.
			// + 8 for rounding up.
			if(inMax != 0) outMatrix[y][x] = mEntry * outMax / inMax + 8;
			else           outMatrix[y][x] = mEntry + 8;
		}
	}
}

void LGYFB_init(KHandle frameReadyEvent)
{
	if(DMA330_run(0, program)) return;

	g_frameReadyEvent = frameReadyEvent;

	LgyFb *const lgyFbTop = getLgyFbRegs(true);
#if OAF_SCALE_SELECT < 2
	lgyFbTop->size  = LGYFB_SIZE(240u, 160u);
#else
	lgyFbTop->size  = LGYFB_SIZE(360u, 240u);
#endif
	lgyFbTop->stat  = LGYFB_IRQ_MASK;
	lgyFbTop->irq   = 0;
	lgyFbTop->alpha = 0xFF;

	/*
	 * Scale matrix limitations:
	 * First pattern bit must be 1 and last 0 (for V-scale) or it loses sync with the DS/GBA input.
	 * Vertical scaling is fucked with identity matrix.
	 *
	 * Matrix ranges:
	 * in[-3] -1024-1023 (0xFC00-0x03FF)
	 * in[-2] -4096-4095 (0xF000-0x0FFF)
	 * in[-1] -32768-32767 (0x8000-0x7FFF)
	 * in[0]  -32768-32767 (0x8000-0x7FFF)
	 * in[1]  -4096-4095 (0xF000-0x0FFF)
	 * in[2]  -1024-1023 (0xFC00-0x03FF)
	 *
	 * Note: At scanline start the in FIFO is all filled with the first pixel.
	 * Note: The first column only allows 1 non-zero entry.
	 * Note: Bits 0-3 of each entry are ignored by the hardware.
	 * Note: 16384 (0x4000) is the maximum brightness of a pixel.
	 *       The sum of all entries in a column should be 16384 or clipping will occur.
	 * Note: The window of (the 6) input pixels is post-increment.
	 *       When the matching pattern bit is 0 it does not move forward.
	 */
#if OAF_SCALE_SELECT < 2
	// Identity.
	static const s16 scaleMatrix[6 * 6] =
	{
		// Identity (no scaling). Don't use for vertical scaling!
		      0,       0,       0,       0,       0,       0,
		      0,       0,       0,       0,       0,       0,
		      0,       0,       0,       0,       0,       0,
		 0x4000,  0x4000,  0x4000,  0x4000,  0x4000,  0x4000,
		      0,       0,       0,       0,       0,       0,
		      0,       0,       0,       0,       0,       0
	};
	setScaleMatrix(&lgyFbTop->h, 6, 0b00111111, scaleMatrix, 5, 8);
#else
	// Other
	static const s16 scaleMatrix[6 * 12 /*6 * 6*/] =
	{
		// Original from AGB_FIRM.
		/*      0,       0,       0,       0,       0,       0, // in[-3]
		      0,       0,       0,       0,       0,       0, // in[-2]
		      0,  0x2000,  0x4000,       0,  0x2000,  0x4000, // in[-1]
		 0x4000,  0x2000,       0,  0x4000,  0x2000,       0, // in[0]
		      0,       0,       0,       0,       0,       0, // in[1]
		      0,       0,       0,       0,       0,       0*/  // in[2]
		// out[0] out[1] out[2]  out[3]  out[4]  out[5]  out[6]  out[7]

		// Pixel duplication. Y'all got any more of them pixels?
		/*      0,       0,       0,       0,       0,       0,
		      0,       0,       0,       0,       0,       0,
		      0,       0,  0x4000,       0,       0,  0x4000,
		 0x4000,  0x4000,       0,  0x4000,  0x4000,       0,
		      0,       0,       0,       0,       0,       0,
		      0,       0,       0,       0,       0,       0*/

		// Sharp interpolated. Basically the same as AGB_FIRM.
		/*      0,       0,       0,       0,       0,       0,
		      0,       0,       0,       0,       0,       0,
		      0,       0,  0x2000,       0,       0,  0x2000,
		 0x4000,  0x4000,  0x2000,  0x4000,  0x4000,  0x2000,
		      0,       0,       0,       0,       0,       0,
		      0,       0,       0,       0,       0,       0*/

		// Sharp + edge enhance. Looks fine but doesn't prevent crumbly edges.
		      0,       0,       0,       0,       0,       0,
		      0,       0,       0,       0,       0,       0,
		      0,  0x24B0,  0x4000,       0,  0x24B0,  0x4000,
		 0x4000,  0x2000,       0,  0x4000,  0x2000,       0,
		      0,  -0x4B0,       0,       0,  -0x4B0,       0,
		      0,       0,       0,       0,       0,       0,

		      0,       0,       0,       0,       0,       0,
		      0,       0,       0,       0,       0,       0,
		      0,       0,  0x24B0,       0,       0,  0x24B0,
		 0x4000,  0x4000,  0x2000,  0x4000,  0x4000,  0x2000,
		      0,       0,  -0x4B0,       0,       0,  -0x4B0,
		      0,       0,       0,       0,       0,       0

		// Interpolated + edge filter. Visibly distorts text.
		/*      0,       0,       0,       0,       0,       0,
		      0,       0,       0,       0,       0,       0,
		      0,  0x1555,  0x2AAA,       0,  0x1555,  0x2AAA,
		 0x3FFF,  0x2AAA,  0x1555,  0x3FFF,  0x2AAA,  0x1555,
		      0,       0,       0,       0,       0,       0,
		      0,       0,       0,       0,       0,       0*/
	};
	setScaleMatrix(&lgyFbTop->v, 6, 0b00011011, scaleMatrix, 5, 8);
	setScaleMatrix(&lgyFbTop->h, 6, 0b00011011, scaleMatrix + (6 * 6), 0, 0);
#endif

	// With RGB8 output solid red and blue are converted to 0xF8 and green to 0xFA.
	// The green bias exists on the whole range of green colors.
	// Some results:
	// RGBA8:   Same as RGB8 but with useless alpha component.
	// RGB8:    Observed best format. Invisible dithering and best color accuracy.
	// RGB565:  A little visible dithering. Good color accuracy.
	// RGB5551: Lots of visible dithering. Good color accuracy (a little worse than 565).
#if OAF_SCALE_SELECT < 2
	lgyFbTop->cnt = LGYFB_DMA_EN | LGYFB_OUT_SWIZZLE | LGYFB_OUT_FMT_8880 |
	                LGYFB_HSCALE_EN | LGYFB_EN;
#else
	lgyFbTop->cnt = LGYFB_DMA_EN | LGYFB_OUT_SWIZZLE | LGYFB_OUT_FMT_8880 |
	                LGYFB_HSCALE_EN | LGYFB_VSCALE_EN | LGYFB_EN;
#endif

	IRQ_registerIsr(IRQ_CDMA_EVENT0, 13, 0, lgyFbDmaIrqHandler);
}

void LGYFB_deinit(void)
{
	getLgyFbRegs(true)->cnt = 0;

	DMA330_kill(0);

	IRQ_unregisterIsr(IRQ_CDMA_EVENT0);
	g_frameReadyEvent = 0;
}

#ifndef NDEBUG
#include "fsutil.h"
/*void LGYFB_dbgDumpFrame(void)
{
	GX_displayTransfer((u32*)0x18200000, 160u<<16 | 256u, (u32*)0x18400000, 160u<<16 | 256u, 1u<<12 | 1u<<8);
	GFX_waitForEvent(GFX_EVENT_PPF, false);
	fsQuickWrite("sdmc:/lgyfb_dbg_frame.bgr", (void*)0x18400000, 256 * 160 * 3);*/
	/*GX_displayTransfer((u32*)0x18200000, 240u<<16 | 512u, (u32*)0x18400000, 240u<<16 | 512u, 1u<<12 | 1u<<8);
	GFX_waitForEvent(GFX_EVENT_PPF, false);
	fsQuickWrite("sdmc:/lgyfb_dbg_frame.bgr", (void*)0x18400000, 512 * 240 * 3);
}*/
#endif
