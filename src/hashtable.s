	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 1
	.globl	_ProbeHT                        ; -- Begin function ProbeHT
	.p2align	2
_ProbeHT:                               ; @ProbeHT
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #144
	stp	x28, x27, [sp, #48]             ; 16-byte Folded Spill
	stp	x26, x25, [sp, #64]             ; 16-byte Folded Spill
	stp	x24, x23, [sp, #80]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #96]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #112]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #128]            ; 16-byte Folded Spill
	add	x29, sp, #128
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset w25, -72
	.cfi_offset w26, -80
	.cfi_offset w27, -88
	.cfi_offset w28, -96
	mov	x27, x7
	str	w6, [sp, #44]                   ; 4-byte Folded Spill
	str	w5, [sp, #12]                   ; 4-byte Folded Spill
	stp	x3, x4, [sp, #24]               ; 16-byte Folded Spill
	mov	x19, x2
	str	x1, [sp, #16]                   ; 8-byte Folded Spill
	mov	x26, x0
	asr	x21, x0, #32
	mov	x8, x21
	ubfiz	x8, x8, #6, #8
Lloh0:
	adrp	x23, _TranspositionMutex@PAGE
Lloh1:
	add	x23, x23, _TranspositionMutex@PAGEOFF
	add	x28, x23, x8
	mov	x0, x28
	bl	_pthread_mutex_lock
	adrp	x24, _TranspositionTable@PAGE
	ldr	x8, [x24, _TranspositionTable@PAGEOFF]
	adrp	x25, _HT_Mask@PAGE
	ldr	w9, [x25, _HT_Mask@PAGEOFF]
	and	x9, x21, x9
	add	x8, x8, x9, lsl #4
	ldp	x20, x22, [x8]
	mov	x0, x28
	bl	_pthread_mutex_unlock
	cmp	w20, w26
	b.ne	LBB0_2
; %bb.1:
	mov	x0, x26
	mov	x8, x20
	b	LBB0_9
LBB0_2:
	add	x8, x26, #1
	str	x8, [sp]                        ; 8-byte Folded Spill
	asr	x20, x8, #32
	mov	x8, x20
	ubfiz	x8, x8, #6, #8
	add	x28, x23, x8
	mov	x0, x28
	bl	_pthread_mutex_lock
	ldr	x8, [x24, _TranspositionTable@PAGEOFF]
	ldr	w9, [x25, _HT_Mask@PAGEOFF]
	and	x9, x20, x9
	add	x8, x8, x9, lsl #4
	ldp	x20, x22, [x8]
	mov	x0, x28
	bl	_pthread_mutex_unlock
	cbz	x27, LBB0_5
; %bb.3:
	cmp	w20, w26
	b.ne	LBB0_6
; %bb.4:
	mov	x8, x20
	b	LBB0_8
LBB0_5:
	mov	x8, x20
	cmp	w20, w26
	ldr	x0, [sp]                        ; 8-byte Folded Reload
	b.eq	LBB0_9
	b	LBB0_15
LBB0_6:
Lloh2:
	adrp	x8, _L_HT_Mask@GOTPAGE
Lloh3:
	ldr	x8, [x8, _L_HT_Mask@GOTPAGEOFF]
Lloh4:
	ldrsw	x8, [x8]
	and	x8, x21, x8
	add	x8, x27, x8, lsl #4
	ldp	x20, x22, [x8]
	cmp	w20, w26
	b.ne	LBB0_15
; %bb.7:
	mov	x8, x26
LBB0_8:
	ldr	x0, [sp]                        ; 8-byte Folded Reload
LBB0_9:
	lsr	x9, x20, #32
	ldp	x10, x11, [sp, #24]             ; 16-byte Folded Reload
	str	w9, [x10]
	lsr	x9, x22, #16
	ubfx	w10, w9, #28, #1
	strb	w10, [x11]
	lsr	x10, x22, #32
	asr	w10, w10, #16
	and	w11, w9, #0xfc00000
	cmp	w10, w19
	ldr	w12, [sp, #44]                  ; 4-byte Folded Reload
	ccmp	w12, #0, #4, eq
	ccmp	w11, #0, #4, ne
	b.ne	LBB0_12
; %bb.10:
	cmp	w10, w19
	b.ge	LBB0_13
; %bb.11:
	mov	w21, #3                         ; =0x3
	b	LBB0_21
LBB0_12:
	mov	w21, #5                         ; =0x5
	b	LBB0_21
LBB0_13:
	asr	w11, w9, #16
	ldr	x13, [sp, #16]                  ; 8-byte Folded Reload
	str	w22, [x13]
	mov	w12, #34465                     ; =0x86a1
	movk	w12, #1, lsl #16
	cmp	w22, w12
	b.lt	LBB0_16
; %bb.14:
	ldr	w12, [sp, #12]                  ; 4-byte Folded Reload
	sub	w12, w22, w12
Lloh5:
	adrp	x15, _TranspositionMutex@PAGE
Lloh6:
	add	x15, x15, _TranspositionMutex@PAGEOFF
	b	LBB0_18
LBB0_15:
	and	x8, x22, #0xffffffff
	orr	x8, x8, x19, lsl #48
	orr	x2, x8, #0x4000000000
	bfxil	x20, x26, #0, #32
	mov	x0, x26
	mov	x1, x20
	mov	x3, x19
	bl	_PutHTEntryBestEffort
	mov	w21, #4                         ; =0x4
	b	LBB0_21
LBB0_16:
	mov	w12, #31071                     ; =0x795f
	movk	w12, #65534, lsl #16
	cmp	w22, w12
Lloh7:
	adrp	x15, _TranspositionMutex@PAGE
Lloh8:
	add	x15, x15, _TranspositionMutex@PAGEOFF
	b.gt	LBB0_19
; %bb.17:
	ldr	w12, [sp, #12]                  ; 4-byte Folded Reload
	add	w12, w22, w12
LBB0_18:
	str	w12, [x13]
LBB0_19:
	mov	w12, #1                         ; =0x1
	tst	w11, #0x8000
	mov	w13, #2                         ; =0x2
	mov	w14, #4                         ; =0x4
	csel	w13, w14, w13, eq
	tst	w9, #0x40000000
	csel	w12, w12, w13, ne
	tst	w9, #0x20000000
	csel	w21, wzr, w12, ne
	cmp	w10, w19
	b.ne	LBB0_21
; %bb.20:
	add	w9, w11, #64
	and	w9, w9, #0xffff
	bfi	x22, x9, #32, #16
	bfxil	x20, x8, #0, #32
	asr	x23, x0, #32
	mov	x8, x23
	ubfiz	x8, x8, #6, #8
	add	x19, x15, x8
	mov	x0, x19
	bl	_pthread_mutex_lock
	ldr	x8, [x24, _TranspositionTable@PAGEOFF]
	ldr	w9, [x25, _HT_Mask@PAGEOFF]
	and	x9, x23, x9
	add	x8, x8, x9, lsl #4
	stp	x20, x22, [x8]
	mov	x0, x19
	bl	_pthread_mutex_unlock
LBB0_21:
	mov	x0, x21
	ldp	x29, x30, [sp, #128]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #112]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #96]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #80]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #64]             ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #144
	ret
	.loh AdrpAdd	Lloh0, Lloh1
	.loh AdrpLdrGotLdr	Lloh2, Lloh3, Lloh4
	.loh AdrpAdd	Lloh5, Lloh6
	.loh AdrpAdd	Lloh7, Lloh8
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function PutHTEntry
_PutHTEntry:                            ; @PutHTEntry
	.cfi_startproc
; %bb.0:
	stp	x22, x21, [sp, #-48]!           ; 16-byte Folded Spill
	stp	x20, x19, [sp, #16]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #32]             ; 16-byte Folded Spill
	add	x29, sp, #32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	mov	x19, x2
	mov	x20, x1
	asr	x22, x0, #32
	mov	x8, x22
	ubfiz	x8, x8, #6, #8
Lloh9:
	adrp	x9, _TranspositionMutex@PAGE
Lloh10:
	add	x9, x9, _TranspositionMutex@PAGEOFF
	add	x21, x9, x8
	mov	x0, x21
	bl	_pthread_mutex_lock
Lloh11:
	adrp	x8, _TranspositionTable@PAGE
Lloh12:
	ldr	x8, [x8, _TranspositionTable@PAGEOFF]
Lloh13:
	adrp	x9, _HT_Mask@PAGE
Lloh14:
	ldr	w9, [x9, _HT_Mask@PAGEOFF]
	and	x9, x22, x9
	add	x8, x8, x9, lsl #4
	stp	x20, x19, [x8]
	mov	x0, x21
	ldp	x29, x30, [sp, #32]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #16]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp], #48             ; 16-byte Folded Reload
	b	_pthread_mutex_unlock
	.loh AdrpLdr	Lloh13, Lloh14
	.loh AdrpLdr	Lloh11, Lloh12
	.loh AdrpAdd	Lloh9, Lloh10
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function PutHTEntryBestEffort
_PutHTEntryBestEffort:                  ; @PutHTEntryBestEffort
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #128
	stp	x28, x27, [sp, #32]             ; 16-byte Folded Spill
	stp	x26, x25, [sp, #48]             ; 16-byte Folded Spill
	stp	x24, x23, [sp, #64]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #80]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #96]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #112]            ; 16-byte Folded Spill
	add	x29, sp, #112
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset w25, -72
	.cfi_offset w26, -80
	.cfi_offset w27, -88
	.cfi_offset w28, -96
	str	w3, [sp, #12]                   ; 4-byte Folded Spill
	stp	x1, x2, [sp, #16]               ; 16-byte Folded Spill
	mov	x25, x0
	add	x26, x0, #1
	asr	x20, x0, #32
	mov	x8, x20
	ubfiz	x8, x8, #6, #8
Lloh15:
	adrp	x23, _TranspositionMutex@PAGE
Lloh16:
	add	x23, x23, _TranspositionMutex@PAGEOFF
	add	x21, x23, x8
	mov	x0, x21
	bl	_pthread_mutex_lock
	adrp	x27, _TranspositionTable@PAGE
	ldr	x8, [x27, _TranspositionTable@PAGEOFF]
	adrp	x22, _HT_Mask@PAGE
	ldr	w9, [x22, _HT_Mask@PAGEOFF]
	and	x9, x20, x9
	add	x8, x8, x9, lsl #4
	ldr	w28, [x8]
	ldr	x8, [x8, #8]
	str	x8, [sp]                        ; 8-byte Folded Spill
	mov	x0, x21
	bl	_pthread_mutex_unlock
	asr	x19, x26, #32
	mov	x8, x19
	ubfiz	x8, x8, #6, #8
	add	x24, x23, x8
	mov	x0, x24
	bl	_pthread_mutex_lock
	ldr	x8, [x27, _TranspositionTable@PAGEOFF]
	ldr	w9, [x22, _HT_Mask@PAGEOFF]
	and	x9, x19, x9
	add	x8, x8, x9, lsl #4
	ldp	x27, x23, [x8]
	mov	x0, x24
	bl	_pthread_mutex_unlock
	cmp	w28, w25
	b.ne	LBB2_3
; %bb.1:
	mov	x0, x21
	bl	_pthread_mutex_lock
Lloh17:
	adrp	x8, _TranspositionTable@PAGE
Lloh18:
	ldr	x8, [x8, _TranspositionTable@PAGEOFF]
	ldr	w9, [x22, _HT_Mask@PAGEOFF]
	and	x9, x20, x9
	add	x8, x8, x9, lsl #4
	ldr	x10, [sp, #16]                  ; 8-byte Folded Reload
	ldr	x9, [sp, #24]                   ; 8-byte Folded Reload
	stp	x10, x9, [x8]
LBB2_2:
	mov	x0, x21
	b	LBB2_5
LBB2_3:
	ldr	x8, [sp]                        ; 8-byte Folded Reload
	ldr	w12, [sp, #12]                  ; 4-byte Folded Reload
	ldp	x28, x22, [sp, #16]             ; 16-byte Folded Reload
	cmp	w27, w25
	b.ne	LBB2_8
LBB2_4:
	mov	x0, x24
	bl	_pthread_mutex_lock
Lloh19:
	adrp	x8, _TranspositionTable@PAGE
Lloh20:
	ldr	x8, [x8, _TranspositionTable@PAGEOFF]
Lloh21:
	adrp	x9, _HT_Mask@PAGE
Lloh22:
	ldr	w9, [x9, _HT_Mask@PAGEOFF]
	and	x9, x19, x9
	add	x8, x8, x9, lsl #4
	stp	x28, x22, [x8]
	mov	x0, x24
LBB2_5:
	bl	_pthread_mutex_unlock
LBB2_6:
	mov	w0, #1                          ; =0x1
LBB2_7:
	ldp	x29, x30, [sp, #112]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #96]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #80]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #64]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #48]             ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #32]             ; 16-byte Folded Reload
	add	sp, sp, #128
	ret
LBB2_8:
	lsr	x9, x8, #32
	asr	w10, w9, #16
	lsr	x8, x23, #32
	asr	w11, w8, #16
	cmp	w10, w11
	b.le	LBB2_10
; %bb.9:
	cmp	w11, w12
	b.le	LBB2_4
	b	LBB2_11
LBB2_10:
	cmp	w10, w12
	b.le	LBB2_14
LBB2_11:
	and	w9, w9, #0x3f
Lloh23:
	adrp	x10, _HTGeneration@PAGE
Lloh24:
	ldr	w10, [x10, _HTGeneration@PAGEOFF]
	cmp	w9, w10
	b.ne	LBB2_14
; %bb.12:
	and	w8, w8, #0x3f
	cmp	w8, w9
	b.ne	LBB2_15
; %bb.13:
	mov	w0, #0                          ; =0x0
	b	LBB2_7
LBB2_14:
	mov	x0, x21
	bl	_pthread_mutex_lock
Lloh25:
	adrp	x8, _TranspositionTable@PAGE
Lloh26:
	ldr	x8, [x8, _TranspositionTable@PAGEOFF]
Lloh27:
	adrp	x9, _HT_Mask@PAGE
Lloh28:
	ldr	w9, [x9, _HT_Mask@PAGEOFF]
	and	x9, x20, x9
	add	x8, x8, x9, lsl #4
	stp	x28, x22, [x8]
	b	LBB2_2
LBB2_15:
	mov	x0, x26
	mov	x1, x28
	mov	x2, x22
	bl	_PutHTEntry
	b	LBB2_6
	.loh AdrpAdd	Lloh15, Lloh16
	.loh AdrpLdr	Lloh17, Lloh18
	.loh AdrpLdr	Lloh21, Lloh22
	.loh AdrpLdr	Lloh19, Lloh20
	.loh AdrpLdr	Lloh23, Lloh24
	.loh AdrpLdr	Lloh27, Lloh28
	.loh AdrpLdr	Lloh25, Lloh26
	.cfi_endproc
                                        ; -- End function
	.globl	_ProbePT                        ; -- Begin function ProbePT
	.p2align	2
_ProbePT:                               ; @ProbePT
	.cfi_startproc
; %bb.0:
	stp	x24, x23, [sp, #-64]!           ; 16-byte Folded Spill
	stp	x22, x21, [sp, #16]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #32]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	mov	x20, x2
	mov	x21, x1
	mov	x22, x0
	asr	x23, x0, #32
	mov	x8, x23
	ubfiz	x8, x8, #6, #8
Lloh29:
	adrp	x9, _PawnMutex@PAGE
Lloh30:
	add	x9, x9, _PawnMutex@PAGEOFF
	add	x19, x9, x8
	mov	x0, x19
	bl	_pthread_mutex_lock
Lloh31:
	adrp	x8, _PawnTable@PAGE
Lloh32:
	ldr	x8, [x8, _PawnTable@PAGEOFF]
Lloh33:
	adrp	x9, _PT_Mask@PAGE
Lloh34:
	ldr	w9, [x9, _PT_Mask@PAGEOFF]
	and	x9, x23, x9
	lsl	x10, x9, #5
	ldr	w10, [x8, x10]
	cmp	w10, w22
	b.ne	LBB3_2
; %bb.1:
	add	x8, x8, x9, lsl #5
	ldr	w9, [x8, #4]
	mov	w10, #65535                     ; =0xffff
	cmp	w9, w10
	b.ne	LBB3_3
LBB3_2:
	mov	w20, #4                         ; =0x4
	b	LBB3_4
LBB3_3:
	str	w9, [x21]
	ldur	q0, [x8, #8]
	ldr	x8, [x8, #24]
	str	x8, [x20, #16]
	str	q0, [x20]
	mov	w20, #3                         ; =0x3
LBB3_4:
	mov	x0, x19
	bl	_pthread_mutex_unlock
	mov	x0, x20
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
	.loh AdrpLdr	Lloh33, Lloh34
	.loh AdrpLdr	Lloh31, Lloh32
	.loh AdrpAdd	Lloh29, Lloh30
	.cfi_endproc
                                        ; -- End function
	.globl	_ProbeST                        ; -- Begin function ProbeST
	.p2align	2
_ProbeST:                               ; @ProbeST
	.cfi_startproc
; %bb.0:
	stp	x24, x23, [sp, #-64]!           ; 16-byte Folded Spill
	stp	x22, x21, [sp, #16]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #32]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	mov	x19, x1
	mov	x20, x0
	asr	x22, x0, #32
	mov	x8, x22
	ubfiz	x8, x8, #6, #8
Lloh35:
	adrp	x9, _ScoreMutex@PAGE
Lloh36:
	add	x9, x9, _ScoreMutex@PAGEOFF
	add	x21, x9, x8
	mov	x0, x21
	bl	_pthread_mutex_lock
Lloh37:
	adrp	x8, _ScoreTable@PAGE
Lloh38:
	ldr	x8, [x8, _ScoreTable@PAGEOFF]
Lloh39:
	adrp	x9, _ST_Mask@PAGE
Lloh40:
	ldr	w9, [x9, _ST_Mask@PAGEOFF]
	and	x9, x22, x9
	add	x8, x8, x9, lsl #3
	ldp	w23, w22, [x8]
	mov	x0, x21
	bl	_pthread_mutex_unlock
	cmp	w23, w20
	mov	w8, #65535                      ; =0xffff
	ccmp	w22, w8, #4, eq
	b.ne	LBB4_2
; %bb.1:
	mov	w0, #4                          ; =0x4
	b	LBB4_3
LBB4_2:
	str	w22, [x19]
	mov	w0, #3                          ; =0x3
LBB4_3:
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
	.loh AdrpLdr	Lloh39, Lloh40
	.loh AdrpLdr	Lloh37, Lloh38
	.loh AdrpAdd	Lloh35, Lloh36
	.cfi_endproc
                                        ; -- End function
	.globl	_StoreHT                        ; -- Begin function StoreHT
	.p2align	2
_StoreHT:                               ; @StoreHT
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #144
	stp	x28, x27, [sp, #48]             ; 16-byte Folded Spill
	stp	x26, x25, [sp, #64]             ; 16-byte Folded Spill
	stp	x24, x23, [sp, #80]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #96]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #112]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #128]            ; 16-byte Folded Spill
	add	x29, sp, #128
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset w25, -72
	.cfi_offset w26, -80
	.cfi_offset w27, -88
	.cfi_offset w28, -96
	mov	x26, x7
	stp	w2, w6, [sp, #24]               ; 8-byte Folded Spill
	stp	x4, x5, [sp, #32]               ; 16-byte Folded Spill
	str	w3, [sp, #20]                   ; 4-byte Folded Spill
	mov	x24, x1
	mov	x21, x0
	ldr	x8, [x29, #16]
	str	x8, [sp, #8]                    ; 8-byte Folded Spill
	asr	x28, x0, #32
	mov	x8, x28
	ubfiz	x8, x8, #6, #8
Lloh41:
	adrp	x23, _TranspositionMutex@PAGE
Lloh42:
	add	x23, x23, _TranspositionMutex@PAGEOFF
	add	x27, x23, x8
	mov	x0, x27
	bl	_pthread_mutex_lock
	adrp	x25, _TranspositionTable@PAGE
	ldr	x8, [x25, _TranspositionTable@PAGEOFF]
	adrp	x19, _HT_Mask@PAGE
	ldr	w9, [x19, _HT_Mask@PAGEOFF]
	and	x9, x28, x9
	add	x8, x8, x9, lsl #4
	ldr	w22, [x8]
	ldr	x20, [x8, #8]
	mov	x0, x27
	bl	_pthread_mutex_unlock
	cmp	w22, w21
	b.ne	LBB5_2
; %bb.1:
Lloh43:
	adrp	x8, _HTStoreTried@PAGE
Lloh44:
	add	x8, x8, _HTStoreTried@PAGEOFF
	mov	w9, #1                          ; =0x1
	ldaddal	w9, w8, [x8]
	b	LBB5_5
LBB5_2:
	add	x8, x21, #1
	asr	x20, x8, #32
	mov	x8, x20
	ubfiz	x8, x8, #6, #8
	add	x27, x23, x8
	mov	x0, x27
	bl	_pthread_mutex_lock
	ldr	x8, [x25, _TranspositionTable@PAGEOFF]
	ldr	w9, [x19, _HT_Mask@PAGEOFF]
	and	x9, x20, x9
	add	x8, x8, x9, lsl #4
	ldr	w19, [x8]
	ldr	x20, [x8, #8]
	mov	x0, x27
	bl	_pthread_mutex_unlock
Lloh45:
	adrp	x8, _HTStoreTried@PAGE
Lloh46:
	add	x8, x8, _HTStoreTried@PAGEOFF
	mov	w9, #1                          ; =0x1
	ldaddal	w9, w8, [x8]
	cmp	w19, w21
	b.ne	LBB5_4
; %bb.3:
	mov	x22, x21
	b	LBB5_5
LBB5_4:
Lloh47:
	adrp	x8, _L_HT_Mask@GOTPAGE
Lloh48:
	ldr	x8, [x8, _L_HT_Mask@GOTPAGEOFF]
Lloh49:
	ldrsw	x8, [x8]
	and	x8, x28, x8
	ldr	x9, [sp, #8]                    ; 8-byte Folded Reload
	add	x8, x9, x8, lsl #4
	ldr	w22, [x8]
	ldr	x20, [x8, #8]
LBB5_5:
	ldp	x13, x3, [sp, #32]              ; 16-byte Folded Reload
	ldp	w15, w14, [sp, #24]             ; 8-byte Folded Reload
	ldr	w12, [sp, #20]                  ; 4-byte Folded Reload
	mov	w8, #31072                      ; =0x7960
	movk	w8, #65534, lsl #16
	cmp	w24, w8
	csel	w8, w26, wzr, lt
	mov	w9, #34464                      ; =0x86a0
	movk	w9, #1, lsl #16
	cmp	w24, w9
	csneg	w8, w26, w8, gt
	add	w8, w8, w24
	cmp	w22, w21
	b.ne	LBB5_9
; %bb.6:
	asr	x9, x20, #48
	cmp	w9, w3
	b.ne	LBB5_9
; %bb.7:
	ands	x9, x20, #0xfc000000000
	b.eq	LBB5_10
; %bb.8:
	mov	x10, #281200098803712           ; =0xffc000000000
	add	x9, x9, x10
	and	x20, x9, #0xffc000000000
	b	LBB5_10
LBB5_9:
	mov	x20, #0                         ; =0x0
LBB5_10:
	orr	x8, x8, x3, lsl #48
Lloh50:
	adrp	x9, _HTGeneration@PAGE
Lloh51:
	ldr	w9, [x9, _HTGeneration@PAGEOFF]
	lsr	x10, x20, #32
	orr	w9, w9, w10
	mov	w10, #49151                     ; =0xbfff
	mov	w11, #57343                     ; =0xdfff
	cmp	w24, w12
	csel	w10, w11, w10, lt
	mov	w11, #16384                     ; =0x4000
	mov	w12, #8192                      ; =0x2000
	csel	w11, w12, w11, lt
	mov	w12, #32767                     ; =0x7fff
	cmp	w24, w15
	csel	w10, w10, w12, gt
	mov	w12, #32768                     ; =0x8000
	csel	w11, w11, w12, gt
	and	w9, w9, w10
	orr	w9, w9, w11
	orr	x8, x8, x9, lsl #32
	orr	x9, x8, #0x100000000000
	cmp	w14, #0
	csel	x22, x8, x9, eq
	mov	x23, x21
	bfi	x23, x13, #32, #32
	mov	x0, x21
	mov	x1, x23
	mov	x2, x22
                                        ; kill: def $w3 killed $w3 killed $x3
	bl	_PutHTEntryBestEffort
	tbnz	w0, #0, LBB5_12
; %bb.11:
Lloh52:
	adrp	x8, _HTStoreFailed@PAGE
Lloh53:
	add	x8, x8, _HTStoreFailed@PAGEOFF
	mov	w9, #1                          ; =0x1
Lloh54:
	adrp	x10, _L_HT_Mask@GOTPAGE
Lloh55:
	ldr	x10, [x10, _L_HT_Mask@GOTPAGEOFF]
	ldaddal	w9, w8, [x8]
Lloh56:
	ldrsw	x8, [x10]
	and	x8, x28, x8
	ldr	x9, [sp, #8]                    ; 8-byte Folded Reload
	add	x8, x9, x8, lsl #4
	stp	x23, x22, [x8]
LBB5_12:
	ldp	x29, x30, [sp, #128]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #112]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #96]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #80]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #64]             ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #144
	ret
	.loh AdrpAdd	Lloh41, Lloh42
	.loh AdrpAdd	Lloh43, Lloh44
	.loh AdrpAdd	Lloh45, Lloh46
	.loh AdrpLdrGotLdr	Lloh47, Lloh48, Lloh49
	.loh AdrpLdr	Lloh50, Lloh51
	.loh AdrpLdrGotLdr	Lloh54, Lloh55, Lloh56
	.loh AdrpAdd	Lloh52, Lloh53
	.cfi_endproc
                                        ; -- End function
	.globl	_StorePT                        ; -- Begin function StorePT
	.p2align	2
_StorePT:                               ; @StorePT
	.cfi_startproc
; %bb.0:
	stp	x24, x23, [sp, #-64]!           ; 16-byte Folded Spill
	stp	x22, x21, [sp, #16]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #32]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	mov	x19, x2
	mov	x20, x1
	mov	x21, x0
	asr	x23, x0, #32
	mov	x8, x23
	ubfiz	x8, x8, #6, #8
Lloh57:
	adrp	x9, _PawnMutex@PAGE
Lloh58:
	add	x9, x9, _PawnMutex@PAGEOFF
	add	x22, x9, x8
	mov	x0, x22
	bl	_pthread_mutex_lock
Lloh59:
	adrp	x8, _PawnTable@PAGE
Lloh60:
	ldr	x8, [x8, _PawnTable@PAGEOFF]
Lloh61:
	adrp	x9, _PT_Mask@PAGE
Lloh62:
	ldr	w9, [x9, _PT_Mask@PAGEOFF]
	and	x9, x23, x9
	add	x8, x8, x9, lsl #5
	stp	w21, w20, [x8]
	ldr	x9, [x19, #16]
	ldr	q0, [x19]
	stur	q0, [x8, #8]
	str	x9, [x8, #24]
	mov	x0, x22
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	b	_pthread_mutex_unlock
	.loh AdrpLdr	Lloh61, Lloh62
	.loh AdrpLdr	Lloh59, Lloh60
	.loh AdrpAdd	Lloh57, Lloh58
	.cfi_endproc
                                        ; -- End function
	.globl	_StoreST                        ; -- Begin function StoreST
	.p2align	2
_StoreST:                               ; @StoreST
	.cfi_startproc
; %bb.0:
	stp	x22, x21, [sp, #-48]!           ; 16-byte Folded Spill
	stp	x20, x19, [sp, #16]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #32]             ; 16-byte Folded Spill
	add	x29, sp, #32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	mov	x19, x1
	mov	x20, x0
	asr	x22, x0, #32
	mov	x8, x22
	ubfiz	x8, x8, #6, #8
Lloh63:
	adrp	x9, _ScoreMutex@PAGE
Lloh64:
	add	x9, x9, _ScoreMutex@PAGEOFF
	add	x21, x9, x8
	mov	x0, x21
	bl	_pthread_mutex_lock
Lloh65:
	adrp	x8, _ScoreTable@PAGE
Lloh66:
	ldr	x8, [x8, _ScoreTable@PAGEOFF]
Lloh67:
	adrp	x9, _ST_Mask@PAGE
Lloh68:
	ldr	w9, [x9, _ST_Mask@PAGEOFF]
	and	x9, x22, x9
	add	x8, x8, x9, lsl #3
	stp	w20, w19, [x8]
	mov	x0, x21
	ldp	x29, x30, [sp, #32]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #16]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp], #48             ; 16-byte Folded Reload
	b	_pthread_mutex_unlock
	.loh AdrpLdr	Lloh67, Lloh68
	.loh AdrpLdr	Lloh65, Lloh66
	.loh AdrpAdd	Lloh63, Lloh64
	.cfi_endproc
                                        ; -- End function
	.globl	_ClearHashTable                 ; -- Begin function ClearHashTable
	.p2align	2
_ClearHashTable:                        ; @ClearHashTable
	.cfi_startproc
; %bb.0:
Lloh69:
	adrp	x8, _HT_Size@PAGE
Lloh70:
	ldr	w8, [x8, _HT_Size@PAGEOFF]
	cbz	w8, LBB8_8
; %bb.1:
Lloh71:
	adrp	x9, _TranspositionTable@PAGE
Lloh72:
	ldr	x11, [x9, _TranspositionTable@PAGEOFF]
	cmp	w8, #4
	b.hs	LBB8_3
; %bb.2:
	mov	w10, #0                         ; =0x0
	mov	x9, x11
	b	LBB8_6
LBB8_3:
	and	x10, x8, #0xfffffffc
	add	x9, x11, x10, lsl #4
	add	x11, x11, #32
	mov	x12, x10
LBB8_4:                                 ; =>This Inner Loop Header: Depth=1
	stur	wzr, [x11, #-32]
	stur	wzr, [x11, #-16]
	str	wzr, [x11]
	str	wzr, [x11, #16]
	sturh	wzr, [x11, #-20]
	sturh	wzr, [x11, #-4]
	strh	wzr, [x11, #12]
	strh	wzr, [x11, #28]
	add	x11, x11, #64
	subs	x12, x12, #4
	b.ne	LBB8_4
; %bb.5:
	cmp	x10, x8
	b.eq	LBB8_8
LBB8_6:
	sub	w8, w8, w10
LBB8_7:                                 ; =>This Inner Loop Header: Depth=1
	str	wzr, [x9]
	strh	wzr, [x9, #12]
	add	x9, x9, #16
	subs	w8, w8, #1
	b.ne	LBB8_7
LBB8_8:
	ret
	.loh AdrpLdr	Lloh69, Lloh70
	.loh AdrpLdr	Lloh71, Lloh72
	.cfi_endproc
                                        ; -- End function
	.globl	_AgeHashTable                   ; -- Begin function AgeHashTable
	.p2align	2
_AgeHashTable:                          ; @AgeHashTable
	.cfi_startproc
; %bb.0:
Lloh73:
	adrp	x8, _HTGeneration@PAGE
	ldr	w9, [x8, _HTGeneration@PAGEOFF]
	add	w9, w9, #1
	and	w9, w9, #0x3f
	str	w9, [x8, _HTGeneration@PAGEOFF]
Lloh74:
	adrp	x8, _HTStoreTried@PAGE
Lloh75:
	add	x8, x8, _HTStoreTried@PAGEOFF
	stlr	wzr, [x8]
Lloh76:
	adrp	x8, _HTStoreFailed@PAGE
Lloh77:
	add	x8, x8, _HTStoreFailed@PAGEOFF
	stlr	wzr, [x8]
	ret
	.loh AdrpAdd	Lloh76, Lloh77
	.loh AdrpAdd	Lloh74, Lloh75
	.loh AdrpAdrp	Lloh73, Lloh74
	.cfi_endproc
                                        ; -- End function
	.globl	_ClearPawnHashTable             ; -- Begin function ClearPawnHashTable
	.p2align	2
_ClearPawnHashTable:                    ; @ClearPawnHashTable
	.cfi_startproc
; %bb.0:
Lloh78:
	adrp	x8, _PT_Size@PAGE
Lloh79:
	ldr	w8, [x8, _PT_Size@PAGEOFF]
	cbz	w8, LBB10_8
; %bb.1:
Lloh80:
	adrp	x9, _PawnTable@PAGE
Lloh81:
	ldr	x11, [x9, _PawnTable@PAGEOFF]
	cmp	w8, #4
	b.hs	LBB10_3
; %bb.2:
	mov	w9, #0                          ; =0x0
	mov	x10, x11
	b	LBB10_6
LBB10_3:
	and	x9, x8, #0xfffffffc
	add	x10, x11, x9, lsl #5
	add	x11, x11, #68
	mov	w12, #65535                     ; =0xffff
	mov	x13, x9
LBB10_4:                                ; =>This Inner Loop Header: Depth=1
	stur	w12, [x11, #-64]
	stur	w12, [x11, #-32]
	str	w12, [x11]
	str	w12, [x11, #32]
	add	x11, x11, #128
	subs	x13, x13, #4
	b.ne	LBB10_4
; %bb.5:
	cmp	x9, x8
	b.eq	LBB10_8
LBB10_6:
	add	x10, x10, #4
	sub	w8, w8, w9
	mov	w9, #65535                      ; =0xffff
LBB10_7:                                ; =>This Inner Loop Header: Depth=1
	str	w9, [x10], #32
	subs	w8, w8, #1
	b.ne	LBB10_7
LBB10_8:
Lloh82:
	adrp	x8, _ST_Size@PAGE
Lloh83:
	ldr	w8, [x8, _ST_Size@PAGEOFF]
	cbz	w8, LBB10_16
; %bb.9:
Lloh84:
	adrp	x9, _ScoreTable@PAGE
Lloh85:
	ldr	x11, [x9, _ScoreTable@PAGEOFF]
	cmp	w8, #4
	b.hs	LBB10_11
; %bb.10:
	mov	w9, #0                          ; =0x0
	mov	x10, x11
	b	LBB10_14
LBB10_11:
	and	x9, x8, #0xfffffffc
	add	x10, x11, x9, lsl #3
	add	x11, x11, #20
	mov	w12, #65535                     ; =0xffff
	mov	x13, x9
LBB10_12:                               ; =>This Inner Loop Header: Depth=1
	stur	w12, [x11, #-16]
	stur	w12, [x11, #-8]
	str	w12, [x11]
	str	w12, [x11, #8]
	add	x11, x11, #32
	subs	x13, x13, #4
	b.ne	LBB10_12
; %bb.13:
	cmp	x9, x8
	b.eq	LBB10_16
LBB10_14:
	add	x10, x10, #4
	sub	w8, w8, w9
	mov	w9, #65535                      ; =0xffff
LBB10_15:                               ; =>This Inner Loop Header: Depth=1
	str	w9, [x10], #8
	subs	w8, w8, #1
	b.ne	LBB10_15
LBB10_16:
	ret
	.loh AdrpLdr	Lloh78, Lloh79
	.loh AdrpLdr	Lloh80, Lloh81
	.loh AdrpLdr	Lloh82, Lloh83
	.loh AdrpLdr	Lloh84, Lloh85
	.cfi_endproc
                                        ; -- End function
	.globl	_AllocateHT                     ; -- Begin function AllocateHT
	.p2align	2
_AllocateHT:                            ; @AllocateHT
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #112
	stp	x24, x23, [sp, #48]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #64]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #80]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #96]             ; 16-byte Folded Spill
	add	x29, sp, #96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	adrp	x8, _AllocateHT.registered_free_ht@PAGE
	ldrb	w9, [x8, _AllocateHT.registered_free_ht@PAGEOFF]
	tbnz	w9, #0, LBB11_2
; %bb.1:
	mov	w9, #1                          ; =0x1
	strb	w9, [x8, _AllocateHT.registered_free_ht@PAGEOFF]
Lloh86:
	adrp	x0, _FreeHT@PAGE
Lloh87:
	add	x0, x0, _FreeHT@PAGEOFF
	bl	_atexit
LBB11_2:
Lloh88:
	adrp	x8, _HT_Bits@PAGE
Lloh89:
	ldr	w22, [x8, _HT_Bits@PAGEOFF]
	mov	w21, #1                         ; =0x1
	lsl	x19, x21, x22
Lloh90:
	adrp	x8, _HT_Size@PAGE
	str	w19, [x8, _HT_Size@PAGEOFF]
	sub	w8, w19, #1
	adrp	x9, _HT_Mask@PAGE
	str	w8, [x9, _HT_Mask@PAGEOFF]
	mov	x0, x19
	mov	w1, #16                         ; =0x10
	bl	_calloc
Lloh91:
	adrp	x8, _TranspositionTable@PAGE
	str	x0, [x8, _TranspositionTable@PAGEOFF]
Lloh92:
	adrp	x8, _L_HT_Bits@PAGE
Lloh93:
	ldr	w8, [x8, _L_HT_Bits@PAGEOFF]
	lsl	w8, w21, w8
Lloh94:
	adrp	x9, _L_HT_Size@GOTPAGE
Lloh95:
	ldr	x9, [x9, _L_HT_Size@GOTPAGEOFF]
Lloh96:
	str	w8, [x9]
	sub	w8, w8, #1
Lloh97:
	adrp	x9, _L_HT_Mask@GOTPAGE
Lloh98:
	ldr	x9, [x9, _L_HT_Mask@GOTPAGEOFF]
Lloh99:
	str	w8, [x9]
Lloh100:
	adrp	x8, _PT_Bits@PAGE
Lloh101:
	ldr	w23, [x8, _PT_Bits@PAGEOFF]
	lsl	x20, x21, x23
Lloh102:
	adrp	x8, _PT_Size@PAGE
	str	w20, [x8, _PT_Size@PAGEOFF]
	sub	w8, w20, #1
	adrp	x9, _PT_Mask@PAGE
	str	w8, [x9, _PT_Mask@PAGEOFF]
	mov	x0, x20
	mov	w1, #32                         ; =0x20
	bl	_calloc
Lloh103:
	adrp	x8, _PawnTable@PAGE
	str	x0, [x8, _PawnTable@PAGEOFF]
Lloh104:
	adrp	x8, _ST_Bits@PAGE
Lloh105:
	ldr	w24, [x8, _ST_Bits@PAGEOFF]
	lsl	x21, x21, x24
Lloh106:
	adrp	x8, _ST_Size@PAGE
	str	w21, [x8, _ST_Size@PAGEOFF]
	sub	w8, w21, #1
	adrp	x9, _ST_Mask@PAGE
	str	w8, [x9, _ST_Mask@PAGEOFF]
	mov	x0, x21
	mov	w1, #8                          ; =0x8
	bl	_calloc
	sxtw	x8, w19
	ubfx	x8, x8, #6, #54
	adrp	x9, _ScoreTable@PAGE
	str	x0, [x9, _ScoreTable@PAGEOFF]
	sxtw	x9, w20
	ubfx	x9, x9, #5, #54
	sxtw	x10, w21
	ubfx	x10, x10, #7, #54
	stp	x23, x24, [sp, #32]
	stp	x10, x22, [sp, #16]
	stp	x8, x9, [sp]
Lloh107:
	adrp	x1, l_.str@PAGE
Lloh108:
	add	x1, x1, l_.str@PAGEOFF
	mov	w0, #0                          ; =0x0
	bl	_Print
	mov	x19, #0                         ; =0x0
Lloh109:
	adrp	x20, _TranspositionMutex@PAGE
Lloh110:
	add	x20, x20, _TranspositionMutex@PAGEOFF
Lloh111:
	adrp	x21, _PawnMutex@PAGE
Lloh112:
	add	x21, x21, _PawnMutex@PAGEOFF
Lloh113:
	adrp	x22, _ScoreMutex@PAGE
Lloh114:
	add	x22, x22, _ScoreMutex@PAGEOFF
LBB11_3:                                ; =>This Inner Loop Header: Depth=1
	add	x0, x20, x19
	mov	x1, #0                          ; =0x0
	bl	_pthread_mutex_init
	add	x0, x21, x19
	mov	x1, #0                          ; =0x0
	bl	_pthread_mutex_init
	add	x0, x22, x19
	mov	x1, #0                          ; =0x0
	bl	_pthread_mutex_init
	add	x19, x19, #64
	cmp	x19, #4, lsl #12                ; =16384
	b.ne	LBB11_3
; %bb.4:
	ldp	x29, x30, [sp, #96]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #80]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #64]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #112
	ret
	.loh AdrpAdd	Lloh86, Lloh87
	.loh AdrpAdd	Lloh113, Lloh114
	.loh AdrpAdd	Lloh111, Lloh112
	.loh AdrpAdd	Lloh109, Lloh110
	.loh AdrpAdd	Lloh107, Lloh108
	.loh AdrpAdrp	Lloh104, Lloh106
	.loh AdrpLdr	Lloh104, Lloh105
	.loh AdrpAdrp	Lloh103, Lloh104
	.loh AdrpAdrp	Lloh100, Lloh102
	.loh AdrpLdr	Lloh100, Lloh101
	.loh AdrpLdrGotStr	Lloh97, Lloh98, Lloh99
	.loh AdrpLdrGotStr	Lloh94, Lloh95, Lloh96
	.loh AdrpLdr	Lloh92, Lloh93
	.loh AdrpAdrp	Lloh91, Lloh92
	.loh AdrpAdrp	Lloh88, Lloh90
	.loh AdrpLdr	Lloh88, Lloh89
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function FreeHT
_FreeHT:                                ; @FreeHT
	.cfi_startproc
; %bb.0:
	stp	x20, x19, [sp, #-32]!           ; 16-byte Folded Spill
	stp	x29, x30, [sp, #16]             ; 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	adrp	x19, _TranspositionTable@PAGE
	ldr	x0, [x19, _TranspositionTable@PAGEOFF]
	cbz	x0, LBB12_2
; %bb.1:
	bl	_free
	str	xzr, [x19, _TranspositionTable@PAGEOFF]
LBB12_2:
	adrp	x19, _PawnTable@PAGE
	ldr	x0, [x19, _PawnTable@PAGEOFF]
	cbz	x0, LBB12_4
; %bb.3:
	bl	_free
	str	xzr, [x19, _PawnTable@PAGEOFF]
LBB12_4:
	adrp	x19, _ScoreTable@PAGE
	ldr	x0, [x19, _ScoreTable@PAGEOFF]
	cbz	x0, LBB12_6
; %bb.5:
	bl	_free
	str	xzr, [x19, _ScoreTable@PAGEOFF]
LBB12_6:
	ldp	x29, x30, [sp, #16]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp], #32             ; 16-byte Folded Reload
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_ShowHashStatistics             ; -- Begin function ShowHashStatistics
	.p2align	2
_ShowHashStatistics:                    ; @ShowHashStatistics
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #112
	stp	x22, x21, [sp, #64]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #80]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #96]             ; 16-byte Folded Spill
	add	x29, sp, #96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
Lloh115:
	adrp	x8, ___stack_chk_guard@GOTPAGE
Lloh116:
	ldr	x8, [x8, ___stack_chk_guard@GOTPAGEOFF]
Lloh117:
	ldr	x8, [x8]
	stur	x8, [x29, #-40]
Lloh118:
	adrp	x8, _HT_Size@PAGE
Lloh119:
	ldr	w19, [x8, _HT_Size@PAGEOFF]
	cbz	w19, LBB13_3
; %bb.1:
Lloh120:
	adrp	x8, _TranspositionTable@PAGE
Lloh121:
	ldr	x11, [x8, _TranspositionTable@PAGEOFF]
Lloh122:
	adrp	x8, _HTGeneration@PAGE
Lloh123:
	ldr	w8, [x8, _HTGeneration@PAGEOFF]
	cmp	w19, #17
	b.hs	LBB13_4
; %bb.2:
	mov	w20, #0                         ; =0x0
	mov	w9, #0                          ; =0x0
	mov	x10, x11
	b	LBB13_7
LBB13_3:
	mov	x19, #0                         ; =0x0
	mov	x20, #0                         ; =0x0
	b	LBB13_9
LBB13_4:
	ands	x9, x19, #0xf
	mov	w10, #16                        ; =0x10
	csel	x9, x10, x9, eq
	sub	x9, x19, x9
	add	x10, x11, x9, lsl #4
	dup.4s	v0, w8
	add	x11, x11, #140
	movi.2d	v1, #0000000000000000
	movi.4h	v2, #63
	mov	x12, x9
	movi.2d	v3, #0000000000000000
	movi.2d	v4, #0000000000000000
	movi.2d	v5, #0000000000000000
LBB13_5:                                ; =>This Inner Loop Header: Depth=1
	ldur	d6, [x11, #-80]
	ldur	d7, [x11, #-96]
	zip1.4h	v6, v7, v6
	ext.8b	v6, v7, v6, #4
	ldur	d7, [x11, #-112]
	ldur	d16, [x11, #-128]
	zip1.4h	v7, v16, v7
	mov.s	v7[1], v6[1]
	ldur	d6, [x11, #-16]
	ldur	d16, [x11, #-32]
	zip1.4h	v6, v16, v6
	ext.8b	v6, v16, v6, #4
	ldur	d16, [x11, #-48]
	ldur	d17, [x11, #-64]
	zip1.4h	v16, v17, v16
	mov.s	v16[1], v6[1]
	ldr	d6, [x11, #48]
	ldr	d17, [x11, #32]
	zip1.4h	v6, v17, v6
	ext.8b	v6, v17, v6, #4
	ldr	d17, [x11, #16]
	ldr	d18, [x11]
	zip1.4h	v17, v18, v17
	mov.s	v17[1], v6[1]
	ldr	d6, [x11, #112]
	ldr	d18, [x11, #96]
	zip1.4h	v6, v18, v6
	ext.8b	v6, v18, v6, #4
	ldr	d18, [x11, #80]
	ldr	d19, [x11, #64]
	zip1.4h	v18, v19, v18
	mov.s	v18[1], v6[1]
	and.8b	v6, v7, v2
	and.8b	v7, v16, v2
	and.8b	v16, v17, v2
	and.8b	v17, v18, v2
	ushll.4s	v6, v6, #0
	ushll.4s	v7, v7, #0
	ushll.4s	v16, v16, #0
	ushll.4s	v17, v17, #0
	cmeq.4s	v6, v0, v6
	cmeq.4s	v7, v0, v7
	cmeq.4s	v16, v0, v16
	cmeq.4s	v17, v0, v17
	sub.4s	v1, v1, v6
	sub.4s	v3, v3, v7
	sub.4s	v4, v4, v16
	sub.4s	v5, v5, v17
	add	x11, x11, #256
	subs	x12, x12, #16
	b.ne	LBB13_5
; %bb.6:
	add.4s	v0, v3, v1
	add.4s	v1, v5, v4
	add.4s	v0, v1, v0
	addv.4s	s0, v0
	fmov	w20, s0
LBB13_7:
	add	x10, x10, #12
	sub	w9, w19, w9
LBB13_8:                                ; =>This Inner Loop Header: Depth=1
	ldrh	w11, [x10], #16
	and	w11, w11, #0x3f
	cmp	w8, w11
	cinc	w20, w20, eq
	subs	w9, w9, #1
	b.ne	LBB13_8
LBB13_9:
	add	x1, sp, #40
	mov	x0, x19
	mov	w2, #16                         ; =0x10
	bl	_FormatCount
	mov	x21, x0
	add	x1, sp, #24
	mov	x0, x20
	mov	w2, #16                         ; =0x10
	bl	_FormatCount
	mov	x22, x0
	mov	x0, x20
	mov	x1, x19
	bl	_Percentage
                                        ; kill: def $w0 killed $w0 def $x0
	stp	x22, x0, [sp, #8]
	str	x21, [sp]
Lloh124:
	adrp	x1, l_.str.1@PAGE
Lloh125:
	add	x1, x1, l_.str.1@PAGEOFF
	mov	w0, #1                          ; =0x1
	bl	_Print
Lloh126:
	adrp	x20, _HTStoreFailed@PAGE
Lloh127:
	add	x20, x20, _HTStoreFailed@PAGEOFF
	ldar	w0, [x20]
	add	x1, sp, #40
	mov	w2, #16                         ; =0x10
	bl	_FormatCount
	mov	x19, x0
	ldar	w0, [x20]
Lloh128:
	adrp	x8, _HTStoreTried@PAGE
Lloh129:
	add	x8, x8, _HTStoreTried@PAGEOFF
	ldar	w1, [x8]
	bl	_Percentage
                                        ; kill: def $w0 killed $w0 def $x0
	stp	x19, x0, [sp]
Lloh130:
	adrp	x1, l_.str.2@PAGE
Lloh131:
	add	x1, x1, l_.str.2@PAGEOFF
	mov	w0, #1                          ; =0x1
	bl	_Print
	ldur	x8, [x29, #-40]
Lloh132:
	adrp	x9, ___stack_chk_guard@GOTPAGE
Lloh133:
	ldr	x9, [x9, ___stack_chk_guard@GOTPAGEOFF]
Lloh134:
	ldr	x9, [x9]
	cmp	x9, x8
	b.ne	LBB13_11
; %bb.10:
	ldp	x29, x30, [sp, #96]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #80]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #64]             ; 16-byte Folded Reload
	add	sp, sp, #112
	ret
LBB13_11:
	bl	___stack_chk_fail
	.loh AdrpLdr	Lloh118, Lloh119
	.loh AdrpLdrGotLdr	Lloh115, Lloh116, Lloh117
	.loh AdrpLdr	Lloh122, Lloh123
	.loh AdrpAdrp	Lloh120, Lloh122
	.loh AdrpLdr	Lloh120, Lloh121
	.loh AdrpLdrGotLdr	Lloh132, Lloh133, Lloh134
	.loh AdrpAdd	Lloh130, Lloh131
	.loh AdrpAdd	Lloh128, Lloh129
	.loh AdrpAdd	Lloh126, Lloh127
	.loh AdrpAdd	Lloh124, Lloh125
	.cfi_endproc
                                        ; -- End function
	.globl	_GuessHTSizes                   ; -- Begin function GuessHTSizes
	.p2align	2
_GuessHTSizes:                          ; @GuessHTSizes
	.cfi_startproc
; %bb.0:
	stp	x20, x19, [sp, #-32]!           ; 16-byte Folded Spill
	stp	x29, x30, [sp, #16]             ; 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	mov	x19, x0
	bl	_strlen
	sub	w8, w0, #1
	ldrb	w8, [x19, w8, sxtw]
	mov	x0, x19
	cmp	w8, #109
	b.eq	LBB14_3
; %bb.1:
	cmp	w8, #107
	b.ne	LBB14_4
; %bb.2:
	bl	_atoi
                                        ; kill: def $w0 killed $w0 def $x0
	sbfiz	x8, x0, #10, #32
	cmp	x8, #16, lsl #12                ; =65536
	b.lt	LBB14_5
	b	LBB14_6
LBB14_3:
	bl	_atoi
                                        ; kill: def $w0 killed $w0 def $x0
	sbfiz	x8, x0, #20, #32
	cmp	x8, #16, lsl #12                ; =65536
	b.lt	LBB14_5
	b	LBB14_6
LBB14_4:
	bl	_atoi
	lsl	w8, w0, #10
	sxtw	x8, w8
	cmp	x8, #16, lsl #12                ; =65536
	b.ge	LBB14_6
LBB14_5:
Lloh135:
	adrp	x1, l_.str.3@PAGE
Lloh136:
	add	x1, x1, l_.str.3@PAGEOFF
	mov	w0, #0                          ; =0x0
	bl	_Print
	mov	w8, #65536                      ; =0x10000
LBB14_6:
	lsl	x9, x8, #2
	mov	x10, #-3689348814741910324      ; =0xcccccccccccccccc
	movk	x10, #52429
	umulh	x9, x9, x10
	lsr	x9, x9, #2
	mov	w12, #1                         ; =0x1
	mov	w11, #2                         ; =0x2
LBB14_7:                                ; =>This Inner Loop Header: Depth=1
	mov	x10, x12
	cmp	w12, #32
	b.eq	LBB14_9
; %bb.8:                                ;   in Loop: Header=BB14_7 Depth=1
	add	w12, w10, #1
	lsl	w13, w11, w10
	cmp	x9, w13, sxtw #4
	b.ge	LBB14_7
LBB14_9:
	adrp	x9, _HT_Bits@PAGE
	str	w10, [x9, _HT_Bits@PAGEOFF]
	mov	w9, #1                          ; =0x1
	lsl	w10, w9, w10
	sub	x8, x8, w10, sxtw #4
	add	x10, x8, x8, lsl #1
	add	x11, x10, #3
	cmp	x10, #0
	csel	x10, x11, x10, lt
	asr	x10, x10, #2
	mov	w12, #2                         ; =0x2
LBB14_10:                               ; =>This Inner Loop Header: Depth=1
	mov	x11, x9
	cmp	w9, #32
	b.eq	LBB14_12
; %bb.11:                               ;   in Loop: Header=BB14_10 Depth=1
	add	w9, w11, #1
	lsl	w13, w12, w11
	cmp	x10, w13, sxtw #3
	b.ge	LBB14_10
LBB14_12:
	adrp	x9, _ST_Bits@PAGE
	str	w11, [x9, _ST_Bits@PAGEOFF]
	mov	w12, #1                         ; =0x1
	lsl	w9, w12, w11
	sub	x8, x8, w9, sxtw #3
	mov	w9, #2                          ; =0x2
LBB14_13:                               ; =>This Inner Loop Header: Depth=1
	mov	x10, x12
	cmp	w12, #32
	b.eq	LBB14_15
; %bb.14:                               ;   in Loop: Header=BB14_13 Depth=1
	add	w12, w10, #1
	lsl	w11, w9, w10
	sxtw	x11, w11
	cmp	x8, x11, lsl #5
	b.ge	LBB14_13
LBB14_15:
	adrp	x8, _PT_Bits@PAGE
	str	w10, [x8, _PT_Bits@PAGEOFF]
	ldp	x29, x30, [sp, #16]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp], #32             ; 16-byte Folded Reload
	ret
	.loh AdrpAdd	Lloh135, Lloh136
	.cfi_endproc
                                        ; -- End function
	.globl	_HashInit                       ; -- Begin function HashInit
	.p2align	2
_HashInit:                              ; @HashInit
	.cfi_startproc
; %bb.0:
	stp	x20, x19, [sp, #-32]!           ; 16-byte Folded Spill
	stp	x29, x30, [sp, #16]             ; 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	mov	x0, #0                          ; =0x0
	bl	_InitRandom
	mov	x20, #0                         ; =0x0
Lloh137:
	adrp	x19, _HashKeys@GOTPAGE
Lloh138:
	ldr	x19, [x19, _HashKeys@GOTPAGEOFF]
LBB15_1:                                ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	str	x0, [x19, x20]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_1
; %bb.2:
	mov	x20, #0                         ; =0x0
LBB15_3:                                ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #512]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_3
; %bb.4:
	mov	x20, #0                         ; =0x0
LBB15_5:                                ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #1024]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_5
; %bb.6:
	mov	x20, #0                         ; =0x0
LBB15_7:                                ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #1536]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_7
; %bb.8:
	mov	x20, #0                         ; =0x0
LBB15_9:                                ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #2048]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_9
; %bb.10:
	mov	x20, #0                         ; =0x0
LBB15_11:                               ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #2560]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_11
; %bb.12:
	mov	x20, #0                         ; =0x0
LBB15_13:                               ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #3072]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_13
; %bb.14:
	mov	x20, #0                         ; =0x0
LBB15_15:                               ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #3584]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_15
; %bb.16:
	mov	x20, #0                         ; =0x0
LBB15_17:                               ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #4096]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_17
; %bb.18:
	mov	x20, #0                         ; =0x0
LBB15_19:                               ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #4608]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_19
; %bb.20:
	mov	x20, #0                         ; =0x0
LBB15_21:                               ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #5120]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_21
; %bb.22:
	mov	x20, #0                         ; =0x0
LBB15_23:                               ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #5632]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_23
; %bb.24:
	mov	x20, #0                         ; =0x0
LBB15_25:                               ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #6144]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_25
; %bb.26:
	mov	x20, #0                         ; =0x0
LBB15_27:                               ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #6656]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_27
; %bb.28:
	mov	x20, #0                         ; =0x0
LBB15_29:                               ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #7168]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_29
; %bb.30:
	mov	x20, #0                         ; =0x0
LBB15_31:                               ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	add	x8, x19, x20
	str	x0, [x8, #7680]
	add	x20, x20, #8
	cmp	x20, #512
	b.ne	LBB15_31
; %bb.32:
	mov	x19, #0                         ; =0x0
Lloh139:
	adrp	x20, _HashKeysEP@GOTPAGE
Lloh140:
	ldr	x20, [x20, _HashKeysEP@GOTPAGEOFF]
LBB15_33:                               ; =>This Inner Loop Header: Depth=1
	bl	_Random64
	str	x0, [x20, x19]
	add	x19, x19, #8
	cmp	x19, #512
	b.ne	LBB15_33
; %bb.34:
	bl	_Random64
Lloh141:
	adrp	x19, _HashKeysCastle@GOTPAGE
Lloh142:
	ldr	x19, [x19, _HashKeysCastle@GOTPAGEOFF]
	str	x0, [x19]
	bl	_Random64
	str	x0, [x19, #8]
	bl	_Random64
	str	x0, [x19, #16]
	bl	_Random64
	str	x0, [x19, #24]
	bl	_Random64
	str	x0, [x19, #32]
	bl	_Random64
	str	x0, [x19, #40]
	bl	_Random64
	str	x0, [x19, #48]
	bl	_Random64
	str	x0, [x19, #56]
	bl	_Random64
	str	x0, [x19, #64]
	bl	_Random64
	str	x0, [x19, #72]
	bl	_Random64
	str	x0, [x19, #80]
	bl	_Random64
	str	x0, [x19, #88]
	bl	_Random64
	str	x0, [x19, #96]
	bl	_Random64
	str	x0, [x19, #104]
	bl	_Random64
	str	x0, [x19, #112]
	bl	_Random64
	str	x0, [x19, #120]
	bl	_Random64
Lloh143:
	adrp	x8, _STMKey@GOTPAGE
Lloh144:
	ldr	x8, [x8, _STMKey@GOTPAGEOFF]
Lloh145:
	str	x0, [x8]
	ldp	x29, x30, [sp, #16]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp], #32             ; 16-byte Folded Reload
	ret
	.loh AdrpLdrGot	Lloh137, Lloh138
	.loh AdrpLdrGot	Lloh139, Lloh140
	.loh AdrpLdrGotStr	Lloh143, Lloh144, Lloh145
	.loh AdrpLdrGot	Lloh141, Lloh142
	.cfi_endproc
                                        ; -- End function
	.section	__DATA,__data
	.globl	_HT_Bits                        ; @HT_Bits
	.p2align	2, 0x0
_HT_Bits:
	.long	17                              ; 0x11

	.globl	_L_HT_Bits                      ; @L_HT_Bits
	.p2align	2, 0x0
_L_HT_Bits:
	.long	16                              ; 0x10

	.comm	_L_HT_Mask,4,2                  ; @L_HT_Mask
.zerofill __DATA,__bss,_PawnMutex,16384,3 ; @PawnMutex
.zerofill __DATA,__bss,_PawnTable,8,3   ; @PawnTable
.zerofill __DATA,__bss,_PT_Mask,4,2     ; @PT_Mask
.zerofill __DATA,__bss,_ScoreMutex,16384,3 ; @ScoreMutex
.zerofill __DATA,__bss,_ScoreTable,8,3  ; @ScoreTable
.zerofill __DATA,__bss,_ST_Mask,4,2     ; @ST_Mask
.zerofill __DATA,__bss,_HTStoreTried,4,2 ; @HTStoreTried
.zerofill __DATA,__bss,_HTGeneration,4,2 ; @HTGeneration
.zerofill __DATA,__bss,_HTStoreFailed,4,2 ; @HTStoreFailed
.zerofill __DATA,__bss,_TranspositionTable,8,3 ; @TranspositionTable
.zerofill __DATA,__bss,_HT_Size,4,2     ; @HT_Size
.zerofill __DATA,__bss,_PT_Size,4,2     ; @PT_Size
.zerofill __DATA,__bss,_ST_Size,4,2     ; @ST_Size
.zerofill __DATA,__bss,_AllocateHT.registered_free_ht,1,0 ; @AllocateHT.registered_free_ht
.zerofill __DATA,__bss,_HT_Mask,4,2     ; @HT_Mask
	.comm	_L_HT_Size,4,2                  ; @L_HT_Size
	.p2align	2, 0x0                          ; @PT_Bits
_PT_Bits:
	.long	15                              ; 0xf

	.p2align	2, 0x0                          ; @ST_Bits
_ST_Bits:
	.long	15                              ; 0xf

	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"Hashtable sizes: %d k, %d k, %d k (%d, %d, %d bits)\n"

.zerofill __DATA,__bss,_TranspositionMutex,16384,3 ; @TranspositionMutex
l_.str.1:                               ; @.str.1
	.asciz	"Hashtable 1:  entries = %s, use = %s (%d %%)\n"

l_.str.2:                               ; @.str.2
	.asciz	"              store failed = %s (%d %%)\n"

l_.str.3:                               ; @.str.3
	.asciz	"I need at least 64k of hashtables.\n"

	.comm	_HashKeys,8192,3                ; @HashKeys
	.comm	_HashKeysEP,512,3               ; @HashKeysEP
	.comm	_HashKeysCastle,128,3           ; @HashKeysCastle
	.comm	_STMKey,8,3                     ; @STMKey
.subsections_via_symbols
