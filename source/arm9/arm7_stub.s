#include "arm.h"
#include "asm_macros.h"

.cpu arm7tdmi
.fpu softvfp



BEGIN_ASM_FUNC _a7_overlay_stub
	mov  r0, #1
	mov  r1, #0x4000000
	strb r0, [r1, #0x300]  @ "POSTFLG"
	ldr  pc, =0x3007E00

.pool
.global _a7_overlay_stub_size
_a7_overlay_stub_size = . - _a7_overlay_stub
END_ASM_FUNC

@ Must be located at 0x3007E00.
BEGIN_ASM_FUNC _a7_stub_start
	adr r1, _a7_stub_thumb + 1  @ 0x3007E1D
	msr CPSR_fsxc, #PSR_INT_OFF | PSR_SVC_MODE
	add sp, r1, #0x6B  @ 0x3007E88
	msr CPSR_fsxc, #PSR_INT_OFF | PSR_SYS_MODE
	add sp, r1, #0x5B  @ 0x3007E78
	mov r3, #0x4700000
	bx  r1

.thumb
_a7_stub_thumb:
	mov r0, #1
	str r0, [r3]  @ Disable BIOS overlay.
	@ The original ARM7 stub waits 256 cycles here (for the BIOS overlay disable?).
	@ The original ARM7 stub waits 1677800 cycles (100 ms) here for LCD/LgyFb sync.
	@ The original ARM7 stub waits for REG_VCOUNT = 160 here.

	lsl  r4, r0, #26  @ 0x4000000 Needed for "function" call 0xBC below.
	mov  r0, #0xFF    @ Clear WRAM, iWRAM, palette RAM, VRAM, OAM
	                  @ + reset SIO, sound and all other registers.

.global _a7_stub9_swi
_a7_stub9_swi = . - _a7_stub_start + 0x80BFE00 @ Final ARM9 mem location.
	swi  0x01       @ RegisterRamReset

	mov  r0, #0xBC  @ SoftReset (0xB4) but skipping r2 & r4 loading.
	mov  r2, #0

	@ REG_VCOUNT should be 126 at ROM entry like after BIOS intro.
_a7_stub_vcount_lp:
	ldrb r1, [r4, #6]  @ REG_VCOUNT
	cmp  r1, #126      @ Loop until REG_VCOUNT == 126.
	bne  _a7_stub_vcount_lp

	bx   r0

.align 2
.global _a7_stub_size
_a7_stub_size = . - _a7_stub_start
END_ASM_FUNC
