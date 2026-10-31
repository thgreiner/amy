	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 26, 0	sdk_version 26, 2
	.globl	_ReadWeightsQuantized           ; -- Begin function ReadWeightsQuantized
	.p2align	2
_ReadWeightsQuantized:                  ; @ReadWeightsQuantized
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
Lloh0:
	adrp	x0, l_.str@PAGE
Lloh1:
	add	x0, x0, l_.str@PAGEOFF
Lloh2:
	adrp	x1, l_.str.1@PAGE
Lloh3:
	add	x1, x1, l_.str.1@PAGEOFF
	bl	_fopen
	cbz	x0, LBB0_10
; %bb.1:
	mov	x19, x0
Lloh4:
	adrp	x0, _input_layer_weights_q@GOTPAGE
Lloh5:
	ldr	x0, [x0, _input_layer_weights_q@GOTPAGEOFF]
	mov	w1, #2                          ; =0x2
	mov	w2, #10485760                   ; =0xa00000
	mov	x3, x19
	bl	_fread
	cmp	x0, #2560, lsl #12              ; =10485760
	b.ne	LBB0_11
; %bb.2:
Lloh6:
	adrp	x0, _input_layer_bias_q@GOTPAGE
Lloh7:
	ldr	x0, [x0, _input_layer_bias_q@GOTPAGEOFF]
	mov	w1, #2                          ; =0x2
	mov	w2, #256                        ; =0x100
	mov	x3, x19
	bl	_fread
	cmp	x0, #256
	b.ne	LBB0_12
; %bb.3:
Lloh8:
	adrp	x0, _hidden_layer_1_weights_q@GOTPAGE
Lloh9:
	ldr	x0, [x0, _hidden_layer_1_weights_q@GOTPAGEOFF]
	mov	w1, #2                          ; =0x2
	mov	w2, #16384                      ; =0x4000
	mov	x3, x19
	bl	_fread
	cmp	x0, #4, lsl #12                 ; =16384
	b.ne	LBB0_13
; %bb.4:
Lloh10:
	adrp	x0, _hidden_layer_1_bias_q@GOTPAGE
Lloh11:
	ldr	x0, [x0, _hidden_layer_1_bias_q@GOTPAGEOFF]
	mov	w1, #4                          ; =0x4
	mov	w2, #32                         ; =0x20
	mov	x3, x19
	bl	_fread
	cmp	x0, #32
	b.ne	LBB0_14
; %bb.5:
Lloh12:
	adrp	x0, _hidden_layer_2_weights_q@GOTPAGE
Lloh13:
	ldr	x0, [x0, _hidden_layer_2_weights_q@GOTPAGEOFF]
	mov	w1, #2                          ; =0x2
	mov	w2, #1024                       ; =0x400
	mov	x3, x19
	bl	_fread
	cmp	x0, #1024
	b.ne	LBB0_15
; %bb.6:
Lloh14:
	adrp	x0, _hidden_layer_2_bias_q@GOTPAGE
Lloh15:
	ldr	x0, [x0, _hidden_layer_2_bias_q@GOTPAGEOFF]
	mov	w1, #4                          ; =0x4
	mov	w2, #32                         ; =0x20
	mov	x3, x19
	bl	_fread
	cmp	x0, #32
	b.ne	LBB0_16
; %bb.7:
Lloh16:
	adrp	x0, _output_layer_weights_q@GOTPAGE
Lloh17:
	ldr	x0, [x0, _output_layer_weights_q@GOTPAGEOFF]
	mov	w1, #4                          ; =0x4
	mov	w2, #32                         ; =0x20
	mov	x3, x19
	bl	_fread
	cmp	x0, #32
	b.ne	LBB0_17
; %bb.8:
Lloh18:
	adrp	x0, _output_layer_bias_q@GOTPAGE
Lloh19:
	ldr	x0, [x0, _output_layer_bias_q@GOTPAGEOFF]
	mov	w1, #4                          ; =0x4
	mov	w2, #1                          ; =0x1
	mov	x3, x19
	bl	_fread
	cmp	x0, #1
	b.ne	LBB0_18
; %bb.9:
	mov	x0, x19
	bl	_fclose
Lloh20:
	adrp	x1, l_.str.10@PAGE
Lloh21:
	add	x1, x1, l_.str.10@PAGEOFF
	mov	w0, #0                          ; =0x0
	ldp	x29, x30, [sp, #16]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp], #32             ; 16-byte Folded Reload
	b	_Print
LBB0_10:
Lloh22:
	adrp	x1, l_.str.2@PAGE
Lloh23:
	add	x1, x1, l_.str.2@PAGEOFF
	mov	w0, #0                          ; =0x0
	ldp	x29, x30, [sp, #16]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp], #32             ; 16-byte Folded Reload
	b	_Print
LBB0_11:
	bl	_ReadWeightsQuantized.cold.1
LBB0_12:
	bl	_ReadWeightsQuantized.cold.2
LBB0_13:
	bl	_ReadWeightsQuantized.cold.3
LBB0_14:
	bl	_ReadWeightsQuantized.cold.4
LBB0_15:
	bl	_ReadWeightsQuantized.cold.5
LBB0_16:
	bl	_ReadWeightsQuantized.cold.6
LBB0_17:
	bl	_ReadWeightsQuantized.cold.7
LBB0_18:
	bl	_ReadWeightsQuantized.cold.8
	.loh AdrpAdd	Lloh2, Lloh3
	.loh AdrpAdd	Lloh0, Lloh1
	.loh AdrpLdrGot	Lloh4, Lloh5
	.loh AdrpLdrGot	Lloh6, Lloh7
	.loh AdrpLdrGot	Lloh8, Lloh9
	.loh AdrpLdrGot	Lloh10, Lloh11
	.loh AdrpLdrGot	Lloh12, Lloh13
	.loh AdrpLdrGot	Lloh14, Lloh15
	.loh AdrpLdrGot	Lloh16, Lloh17
	.loh AdrpLdrGot	Lloh18, Lloh19
	.loh AdrpAdd	Lloh20, Lloh21
	.loh AdrpAdd	Lloh22, Lloh23
	.cfi_endproc
                                        ; -- End function
	.globl	_ReadWeightsCompressed          ; -- Begin function ReadWeightsCompressed
	.p2align	2
_ReadWeightsCompressed:                 ; @ReadWeightsCompressed
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
Lloh24:
	adrp	x0, l_.str.11@PAGE
Lloh25:
	add	x0, x0, l_.str.11@PAGEOFF
Lloh26:
	adrp	x1, l_.str.1@PAGE
Lloh27:
	add	x1, x1, l_.str.1@PAGEOFF
	bl	_fopen
	cbz	x0, LBB1_32
; %bb.1:
	mov	x20, x0
	mov	x1, #0                          ; =0x0
	mov	w2, #2                          ; =0x2
	bl	_fseek
	mov	x0, x20
	bl	_ftell
	mov	x21, x0
	mov	x0, x20
	mov	x1, #0                          ; =0x0
	mov	w2, #0                          ; =0x0
	bl	_fseek
	mov	x0, x21
	bl	_malloc
	cbz	x0, LBB1_42
; %bb.2:
	mov	x19, x0
	mov	x1, x21
	mov	w2, #1                          ; =0x1
	mov	x3, x20
	bl	_fread
	cmp	x0, #1
	b.ne	LBB1_33
; %bb.3:
	mov	x0, x20
	bl	_fclose
	mov	w0, #35724                      ; =0x8b8c
	movk	w0, #320, lsl #16
	bl	_malloc
	cbz	x0, LBB1_43
; %bb.4:
	mov	x20, x0
	cmp	x21, #1
	b.lt	LBB1_23
; %bb.5:
	add	x8, x19, x21
	mov	x10, x19
	mov	x9, x20
	b	LBB1_7
LBB1_6:                                 ;   in Loop: Header=BB1_7 Depth=1
	sxtb	w11, w12
	mov	w12, #1                         ; =0x1
	add	x10, x10, x12
	strh	w11, [x9, #14]
	add	x9, x9, #16
	add	x10, x10, #1
	cmp	x10, x8
	b.hs	LBB1_23
LBB1_7:                                 ; =>This Inner Loop Header: Depth=1
	ldrsb	w11, [x10]
	mov	x13, x10
	ldrb	w12, [x13, #1]!
	tbnz	w11, #31, LBB1_15
; %bb.8:                                ;   in Loop: Header=BB1_7 Depth=1
	sxtb	w12, w12
	mov	x10, x13
	and	w11, w11, #0xff
	strh	w12, [x9]
	ldrb	w12, [x13, #1]!
	tbnz	w11, #6, LBB1_16
LBB1_9:                                 ;   in Loop: Header=BB1_7 Depth=1
	sxtb	w12, w12
	mov	x10, x13
	strh	w12, [x9, #2]
	ldrb	w12, [x13, #1]!
	tbz	w11, #5, LBB1_17
LBB1_10:                                ;   in Loop: Header=BB1_7 Depth=1
	ldrb	w13, [x10, #2]!
	orr	w12, w12, w13, lsl #8
	strh	w12, [x9, #4]
	mov	x13, x10
	ldrb	w12, [x13, #1]!
	tbz	w11, #4, LBB1_18
LBB1_11:                                ;   in Loop: Header=BB1_7 Depth=1
	ldrb	w13, [x10, #2]!
	orr	w12, w12, w13, lsl #8
	strh	w12, [x9, #6]
	mov	x13, x10
	ldrb	w12, [x13, #1]!
	tbz	w11, #3, LBB1_19
LBB1_12:                                ;   in Loop: Header=BB1_7 Depth=1
	ldrb	w13, [x10, #2]!
	orr	w12, w12, w13, lsl #8
	strh	w12, [x9, #8]
	mov	x13, x10
	ldrb	w12, [x13, #1]!
	tbz	w11, #2, LBB1_20
LBB1_13:                                ;   in Loop: Header=BB1_7 Depth=1
	ldrb	w13, [x10, #2]!
	orr	w12, w12, w13, lsl #8
	strh	w12, [x9, #10]
	mov	x13, x10
	ldrb	w12, [x13, #1]!
	tbz	w11, #1, LBB1_21
LBB1_14:                                ;   in Loop: Header=BB1_7 Depth=1
	ldrb	w13, [x10, #2]!
	orr	w12, w12, w13, lsl #8
	strh	w12, [x9, #12]
	ldrb	w12, [x10, #1]
	tbz	w11, #0, LBB1_6
	b	LBB1_22
LBB1_15:                                ;   in Loop: Header=BB1_7 Depth=1
	ldrb	w13, [x10, #2]!
	orr	w12, w12, w13, lsl #8
	and	w11, w11, #0xff
	strh	w12, [x9]
	mov	x13, x10
	ldrb	w12, [x13, #1]!
	tbz	w11, #6, LBB1_9
LBB1_16:                                ;   in Loop: Header=BB1_7 Depth=1
	ldrb	w13, [x10, #2]!
	orr	w12, w12, w13, lsl #8
	strh	w12, [x9, #2]
	mov	x13, x10
	ldrb	w12, [x13, #1]!
	tbnz	w11, #5, LBB1_10
LBB1_17:                                ;   in Loop: Header=BB1_7 Depth=1
	sxtb	w12, w12
	mov	x10, x13
	strh	w12, [x9, #4]
	ldrb	w12, [x13, #1]!
	tbnz	w11, #4, LBB1_11
LBB1_18:                                ;   in Loop: Header=BB1_7 Depth=1
	sxtb	w12, w12
	mov	x10, x13
	strh	w12, [x9, #6]
	ldrb	w12, [x13, #1]!
	tbnz	w11, #3, LBB1_12
LBB1_19:                                ;   in Loop: Header=BB1_7 Depth=1
	sxtb	w12, w12
	mov	x10, x13
	strh	w12, [x9, #8]
	ldrb	w12, [x13, #1]!
	tbnz	w11, #2, LBB1_13
LBB1_20:                                ;   in Loop: Header=BB1_7 Depth=1
	sxtb	w12, w12
	mov	x10, x13
	strh	w12, [x9, #10]
	ldrb	w12, [x13, #1]!
	tbnz	w11, #1, LBB1_14
LBB1_21:                                ;   in Loop: Header=BB1_7 Depth=1
	sxtb	w12, w12
	mov	x10, x13
	strh	w12, [x9, #12]
	ldrb	w12, [x13, #1]
	tbz	w11, #0, LBB1_6
LBB1_22:                                ;   in Loop: Header=BB1_7 Depth=1
	ldrb	w11, [x10, #2]
	orr	w11, w12, w11, lsl #8
	mov	w12, #2                         ; =0x2
	add	x10, x10, x12
	strh	w11, [x9, #14]
	add	x9, x9, #16
	add	x10, x10, #1
	cmp	x10, x8
	b.lo	LBB1_7
LBB1_23:
Lloh28:
	adrp	x0, _input_layer_weights_q@GOTPAGE
Lloh29:
	ldr	x0, [x0, _input_layer_weights_q@GOTPAGEOFF]
	mov	x1, x20
	mov	w2, #20971520                   ; =0x1400000
	bl	_memcmp
	cbnz	w0, LBB1_34
; %bb.24:
Lloh30:
	adrp	x0, _input_layer_weights_q@GOTPAGE
Lloh31:
	ldr	x0, [x0, _input_layer_weights_q@GOTPAGEOFF]
	mov	w21, #20971520                  ; =0x1400000
	mov	x1, x20
	mov	w2, #20971520                   ; =0x1400000
	bl	_memcpy
Lloh32:
	adrp	x0, _input_layer_bias_q@GOTPAGE
Lloh33:
	ldr	x0, [x0, _input_layer_bias_q@GOTPAGEOFF]
	add	x1, x20, x21
	mov	w2, #512                        ; =0x200
	bl	_memcmp
	cbnz	w0, LBB1_35
; %bb.25:
Lloh34:
	adrp	x0, _input_layer_bias_q@GOTPAGE
Lloh35:
	ldr	x0, [x0, _input_layer_bias_q@GOTPAGEOFF]
	add	x1, x20, x21
	mov	w2, #512                        ; =0x200
	bl	_memcpy
	mov	w21, #512                       ; =0x200
	movk	w21, #320, lsl #16
Lloh36:
	adrp	x0, _hidden_layer_1_weights_q@GOTPAGE
Lloh37:
	ldr	x0, [x0, _hidden_layer_1_weights_q@GOTPAGEOFF]
	add	x1, x20, x21
	mov	w2, #32768                      ; =0x8000
	bl	_memcmp
	cbnz	w0, LBB1_36
; %bb.26:
Lloh38:
	adrp	x0, _hidden_layer_1_weights_q@GOTPAGE
Lloh39:
	ldr	x0, [x0, _hidden_layer_1_weights_q@GOTPAGEOFF]
	add	x1, x20, x21
	mov	w2, #32768                      ; =0x8000
	bl	_memcpy
	mov	w8, #33280                      ; =0x8200
	movk	w8, #320, lsl #16
	add	x22, x20, x8
Lloh40:
	adrp	x21, _hidden_layer_1_bias_q@GOTPAGE
Lloh41:
	ldr	x21, [x21, _hidden_layer_1_bias_q@GOTPAGEOFF]
	mov	x0, x21
	mov	x1, x22
	mov	w2, #128                        ; =0x80
	bl	_memcmp
	cbnz	w0, LBB1_37
; %bb.27:
	ldp	q0, q1, [x22, #64]
	stp	q0, q1, [x21, #64]
	ldp	q0, q1, [x22, #96]
	stp	q0, q1, [x21, #96]
	ldp	q0, q1, [x22]
	stp	q0, q1, [x21]
	ldp	q0, q1, [x22, #32]
	mov	w22, #33408                     ; =0x8280
	movk	w22, #320, lsl #16
	stp	q0, q1, [x21, #32]
Lloh42:
	adrp	x0, _hidden_layer_2_weights_q@GOTPAGE
Lloh43:
	ldr	x0, [x0, _hidden_layer_2_weights_q@GOTPAGEOFF]
	add	x1, x20, x22
	mov	w2, #2048                       ; =0x800
	bl	_memcmp
	cbnz	w0, LBB1_38
; %bb.28:
Lloh44:
	adrp	x0, _hidden_layer_2_weights_q@GOTPAGE
Lloh45:
	ldr	x0, [x0, _hidden_layer_2_weights_q@GOTPAGEOFF]
	add	x1, x20, x22
	mov	w2, #2048                       ; =0x800
	bl	_memcpy
	mov	w8, #35456                      ; =0x8a80
	movk	w8, #320, lsl #16
	add	x22, x20, x8
Lloh46:
	adrp	x21, _hidden_layer_2_bias_q@GOTPAGE
Lloh47:
	ldr	x21, [x21, _hidden_layer_2_bias_q@GOTPAGEOFF]
	mov	x0, x21
	mov	x1, x22
	mov	w2, #128                        ; =0x80
	bl	_memcmp
	cbnz	w0, LBB1_39
; %bb.29:
	ldp	q0, q1, [x22, #64]
	stp	q0, q1, [x21, #64]
	ldp	q0, q1, [x22, #96]
	stp	q0, q1, [x21, #96]
	ldp	q0, q1, [x22]
	stp	q0, q1, [x21]
	ldp	q0, q1, [x22, #32]
	mov	w8, #35584                      ; =0x8b00
	movk	w8, #320, lsl #16
	add	x22, x20, x8
	stp	q0, q1, [x21, #32]
Lloh48:
	adrp	x21, _output_layer_weights_q@GOTPAGE
Lloh49:
	ldr	x21, [x21, _output_layer_weights_q@GOTPAGEOFF]
	mov	x0, x21
	mov	x1, x22
	mov	w2, #128                        ; =0x80
	bl	_memcmp
	cbnz	w0, LBB1_40
; %bb.30:
	ldp	q0, q1, [x22, #64]
	stp	q0, q1, [x21, #64]
	ldp	q0, q1, [x22, #96]
	stp	q0, q1, [x21, #96]
	ldp	q0, q1, [x22]
	stp	q0, q1, [x21]
	ldp	q0, q1, [x22, #32]
	stp	q0, q1, [x21, #32]
	mov	w8, #35712                      ; =0x8b80
	movk	w8, #320, lsl #16
Lloh50:
	adrp	x9, _output_layer_bias_q@GOTPAGE
Lloh51:
	ldr	x9, [x9, _output_layer_bias_q@GOTPAGEOFF]
	ldr	w10, [x9]
	ldr	w11, [x20, x8]
	cmp	w10, w11
	b.ne	LBB1_41
; %bb.31:
	ldr	w8, [x20, x8]
	str	w8, [x9]
	mov	x0, x20
	bl	_free
	mov	x0, x19
	ldp	x29, x30, [sp, #32]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #16]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp], #48             ; 16-byte Folded Reload
	b	_free
LBB1_32:
Lloh52:
	adrp	x1, l_.str.2@PAGE
Lloh53:
	add	x1, x1, l_.str.2@PAGEOFF
	ldp	x29, x30, [sp, #32]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #16]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp], #48             ; 16-byte Folded Reload
	b	_Print
LBB1_33:
	bl	_ReadWeightsCompressed.cold.1
LBB1_34:
	bl	_ReadWeightsCompressed.cold.2
LBB1_35:
	bl	_ReadWeightsCompressed.cold.3
LBB1_36:
	bl	_ReadWeightsCompressed.cold.4
LBB1_37:
	bl	_ReadWeightsCompressed.cold.5
LBB1_38:
	bl	_ReadWeightsCompressed.cold.6
LBB1_39:
	bl	_ReadWeightsCompressed.cold.7
LBB1_40:
	bl	_ReadWeightsCompressed.cold.8
LBB1_41:
	bl	_ReadWeightsCompressed.cold.9
LBB1_42:
	bl	_ReadWeightsCompressed.cold.11
LBB1_43:
	bl	_ReadWeightsCompressed.cold.10
	.loh AdrpAdd	Lloh26, Lloh27
	.loh AdrpAdd	Lloh24, Lloh25
	.loh AdrpLdrGot	Lloh28, Lloh29
	.loh AdrpLdrGot	Lloh32, Lloh33
	.loh AdrpLdrGot	Lloh30, Lloh31
	.loh AdrpLdrGot	Lloh36, Lloh37
	.loh AdrpLdrGot	Lloh34, Lloh35
	.loh AdrpLdrGot	Lloh40, Lloh41
	.loh AdrpLdrGot	Lloh38, Lloh39
	.loh AdrpLdrGot	Lloh42, Lloh43
	.loh AdrpLdrGot	Lloh46, Lloh47
	.loh AdrpLdrGot	Lloh44, Lloh45
	.loh AdrpLdrGot	Lloh48, Lloh49
	.loh AdrpLdrGot	Lloh50, Lloh51
	.loh AdrpAdd	Lloh52, Lloh53
	.cfi_endproc
                                        ; -- End function
	.globl	_UpdateWeightsForPiece          ; -- Begin function UpdateWeightsForPiece
	.p2align	2
_UpdateWeightsForPiece:                 ; @UpdateWeightsForPiece
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #80
	stp	x24, x23, [sp, #16]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #32]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #48]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #64]             ; 16-byte Folded Spill
	add	x29, sp, #64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	mov	x19, x0
	ldrsb	w8, [x0, #2293]
	ldrsb	w9, [x0, #2294]
	cmp	w1, #0
	cneg	w10, w1, mi
Lloh54:
	adrp	x11, _update_accumulator_sub@PAGE
Lloh55:
	add	x11, x11, _update_accumulator_sub@PAGEOFF
Lloh56:
	adrp	x12, _update_accumulator_add@PAGE
Lloh57:
	add	x12, x12, _update_accumulator_add@PAGEOFF
	cmp	w4, #0
	csel	x0, x12, x11, ne
	str	x0, [sp, #8]                    ; 8-byte Folded Spill
	eor	w20, w9, #0x38
	lsl	w21, w10, #12
	eor	w22, w2, #0x38
	mov	w9, #-4096                      ; =0xfffff000
	mov	w10, #16384                     ; =0x4000
	cmp	w3, #0
	csel	w11, w10, w9, ne
	csel	w23, w9, w10, ne
	add	w9, w11, w2
	add	w9, w9, w21
	add	w1, w9, w8, lsl #6
	add	x0, x19, #1196
	ldr	x2, [sp, #8]                    ; 8-byte Folded Reload
	blr	x2
	add	w8, w23, w22
	add	w8, w21, w8
	add	w1, w8, w20, lsl #6
	add	x0, x19, #1708
	ldr	x2, [sp, #8]                    ; 8-byte Folded Reload
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	add	sp, sp, #80
	br	x2
	.loh AdrpAdd	Lloh56, Lloh57
	.loh AdrpAdd	Lloh54, Lloh55
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function update_accumulator_add
_update_accumulator_add:                ; @update_accumulator_add
	.cfi_startproc
; %bb.0:
                                        ; kill: def $w1 killed $w1 def $x1
	sxtw	x8, w1
	add	x9, x0, #512
Lloh58:
	adrp	x10, _input_layer_weights_q@GOTPAGE
Lloh59:
	ldr	x10, [x10, _input_layer_weights_q@GOTPAGEOFF]
	add	x8, x10, x8, lsl #9
	add	x10, x8, #512
	cmp	x10, x0
	ccmp	x8, x9, #2, hi
	b.lo	LBB3_2
; %bb.1:
	ldp	q0, q1, [x8]
	ldp	q2, q3, [x8, #32]
	ldp	q4, q5, [x0]
	ldp	q6, q7, [x0, #32]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x0]
	stp	q2, q3, [x0, #32]
	ldp	q0, q1, [x8, #64]
	ldp	q2, q3, [x8, #96]
	ldp	q4, q5, [x0, #64]
	ldp	q6, q7, [x0, #96]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x0, #64]
	stp	q2, q3, [x0, #96]
	ldp	q0, q1, [x8, #128]
	ldp	q2, q3, [x8, #160]
	ldp	q4, q5, [x0, #128]
	ldp	q6, q7, [x0, #160]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x0, #128]
	stp	q2, q3, [x0, #160]
	ldp	q0, q1, [x8, #192]
	ldp	q2, q3, [x8, #224]
	ldp	q4, q5, [x0, #192]
	ldp	q6, q7, [x0, #224]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x0, #192]
	stp	q2, q3, [x0, #224]
	ldp	q0, q1, [x8, #256]
	ldp	q2, q3, [x8, #288]
	ldp	q4, q5, [x0, #256]
	ldp	q6, q7, [x0, #288]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x0, #256]
	stp	q2, q3, [x0, #288]
	ldp	q0, q1, [x8, #320]
	ldp	q2, q3, [x8, #352]
	ldp	q4, q5, [x0, #320]
	ldp	q6, q7, [x0, #352]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x0, #320]
	stp	q2, q3, [x0, #352]
	ldp	q0, q1, [x8, #384]
	ldp	q2, q3, [x8, #416]
	ldp	q4, q5, [x0, #384]
	ldp	q6, q7, [x0, #416]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x0, #384]
	stp	q2, q3, [x0, #416]
	ldp	q0, q1, [x8, #448]
	ldp	q2, q3, [x8, #480]
	ldp	q4, q5, [x0, #448]
	ldp	q6, q7, [x0, #480]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x0, #448]
	stp	q2, q3, [x0, #480]
	ret
LBB3_2:
	mov	x9, #0                          ; =0x0
LBB3_3:                                 ; =>This Inner Loop Header: Depth=1
	ldrh	w10, [x8, x9]
	ldrh	w11, [x0, x9]
	add	w10, w11, w10
	strh	w10, [x0, x9]
	add	x9, x9, #2
	cmp	x9, #512
	b.ne	LBB3_3
; %bb.4:
	ret
	.loh AdrpLdrGot	Lloh58, Lloh59
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function update_accumulator_sub
_update_accumulator_sub:                ; @update_accumulator_sub
	.cfi_startproc
; %bb.0:
                                        ; kill: def $w1 killed $w1 def $x1
	sxtw	x8, w1
	add	x9, x0, #512
Lloh60:
	adrp	x10, _input_layer_weights_q@GOTPAGE
Lloh61:
	ldr	x10, [x10, _input_layer_weights_q@GOTPAGEOFF]
	add	x8, x10, x8, lsl #9
	add	x10, x8, #512
	cmp	x10, x0
	ccmp	x8, x9, #2, hi
	b.lo	LBB4_2
; %bb.1:
	ldp	q0, q1, [x8]
	ldp	q2, q3, [x8, #32]
	ldp	q4, q5, [x0]
	ldp	q6, q7, [x0, #32]
	sub.8h	v0, v4, v0
	sub.8h	v1, v5, v1
	sub.8h	v2, v6, v2
	sub.8h	v3, v7, v3
	stp	q0, q1, [x0]
	stp	q2, q3, [x0, #32]
	ldp	q0, q1, [x8, #64]
	ldp	q2, q3, [x8, #96]
	ldp	q4, q5, [x0, #64]
	ldp	q6, q7, [x0, #96]
	sub.8h	v0, v4, v0
	sub.8h	v1, v5, v1
	sub.8h	v2, v6, v2
	sub.8h	v3, v7, v3
	stp	q0, q1, [x0, #64]
	stp	q2, q3, [x0, #96]
	ldp	q0, q1, [x8, #128]
	ldp	q2, q3, [x8, #160]
	ldp	q4, q5, [x0, #128]
	ldp	q6, q7, [x0, #160]
	sub.8h	v0, v4, v0
	sub.8h	v1, v5, v1
	sub.8h	v2, v6, v2
	sub.8h	v3, v7, v3
	stp	q0, q1, [x0, #128]
	stp	q2, q3, [x0, #160]
	ldp	q0, q1, [x8, #192]
	ldp	q2, q3, [x8, #224]
	ldp	q4, q5, [x0, #192]
	ldp	q6, q7, [x0, #224]
	sub.8h	v0, v4, v0
	sub.8h	v1, v5, v1
	sub.8h	v2, v6, v2
	sub.8h	v3, v7, v3
	stp	q0, q1, [x0, #192]
	stp	q2, q3, [x0, #224]
	ldp	q0, q1, [x8, #256]
	ldp	q2, q3, [x8, #288]
	ldp	q4, q5, [x0, #256]
	ldp	q6, q7, [x0, #288]
	sub.8h	v0, v4, v0
	sub.8h	v1, v5, v1
	sub.8h	v2, v6, v2
	sub.8h	v3, v7, v3
	stp	q0, q1, [x0, #256]
	stp	q2, q3, [x0, #288]
	ldp	q0, q1, [x8, #320]
	ldp	q2, q3, [x8, #352]
	ldp	q4, q5, [x0, #320]
	ldp	q6, q7, [x0, #352]
	sub.8h	v0, v4, v0
	sub.8h	v1, v5, v1
	sub.8h	v2, v6, v2
	sub.8h	v3, v7, v3
	stp	q0, q1, [x0, #320]
	stp	q2, q3, [x0, #352]
	ldp	q0, q1, [x8, #384]
	ldp	q2, q3, [x8, #416]
	ldp	q4, q5, [x0, #384]
	ldp	q6, q7, [x0, #416]
	sub.8h	v0, v4, v0
	sub.8h	v1, v5, v1
	sub.8h	v2, v6, v2
	sub.8h	v3, v7, v3
	stp	q0, q1, [x0, #384]
	stp	q2, q3, [x0, #416]
	ldp	q0, q1, [x8, #448]
	ldp	q2, q3, [x8, #480]
	ldp	q4, q5, [x0, #448]
	ldp	q6, q7, [x0, #480]
	sub.8h	v0, v4, v0
	sub.8h	v1, v5, v1
	sub.8h	v2, v6, v2
	sub.8h	v3, v7, v3
	stp	q0, q1, [x0, #448]
	stp	q2, q3, [x0, #480]
	ret
LBB4_2:
	mov	x9, #0                          ; =0x0
LBB4_3:                                 ; =>This Inner Loop Header: Depth=1
	ldrh	w10, [x8, x9]
	ldrh	w11, [x0, x9]
	sub	w10, w11, w10
	strh	w10, [x0, x9]
	add	x9, x9, #2
	cmp	x9, #512
	b.ne	LBB4_3
; %bb.4:
	ret
	.loh AdrpLdrGot	Lloh60, Lloh61
	.cfi_endproc
                                        ; -- End function
	.globl	_UpdateWeightsForKing           ; -- Begin function UpdateWeightsForKing
	.p2align	2
_UpdateWeightsForKing:                  ; @UpdateWeightsForKing
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
	mov	x20, x0
	ldrsb	x22, [x0, #2292]
	cmp	x22, #0
	mov	w23, #56                        ; =0x38
	csel	w21, wzr, w23, eq
	add	x8, x0, x22
	ldrsb	w8, [x8, #2293]
	eor	w24, w21, w8
	cmp	w22, #0
	mov	w8, #1708                       ; =0x6ac
	mov	w9, #1196                       ; =0x4ac
	csel	x8, x9, x8, eq
	add	x19, x0, x8
Lloh62:
	adrp	x1, _input_layer_bias_q@GOTPAGE
Lloh63:
	ldr	x1, [x1, _input_layer_bias_q@GOTPAGEOFF]
	mov	x0, x19
	mov	w2, #512                        ; =0x200
	bl	_memcpy
	add	x9, x20, #1024
	eor	w8, w22, #0x1
	sxtw	x11, w8
	sbfiz	x10, x24, #6, #32
	smaddl	x8, w22, w23, x9
	ldr	x12, [x8, #8]
Lloh64:
	adrp	x8, _input_layer_weights_q@GOTPAGE
Lloh65:
	ldr	x8, [x8, _input_layer_weights_q@GOTPAGEOFF]
	cbz	x12, LBB5_7
; %bb.1:
	add	x13, x19, #512
	sxtw	x14, w10
	b	LBB5_4
LBB5_2:                                 ;   in Loop: Header=BB5_4 Depth=1
	ldp	q0, q1, [x15]
	ldp	q2, q3, [x15, #32]
	ldp	q4, q5, [x19]
	ldp	q6, q7, [x19, #32]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19]
	stp	q2, q3, [x19, #32]
	ldp	q0, q1, [x15, #64]
	ldp	q2, q3, [x15, #96]
	ldp	q4, q5, [x19, #64]
	ldp	q6, q7, [x19, #96]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	ldp	q0, q1, [x15, #128]
	ldp	q2, q3, [x15, #160]
	ldp	q4, q5, [x19, #128]
	ldp	q6, q7, [x19, #160]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #128]
	stp	q2, q3, [x19, #160]
	ldp	q0, q1, [x15, #192]
	ldp	q2, q3, [x15, #224]
	ldp	q4, q5, [x19, #192]
	ldp	q6, q7, [x19, #224]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #192]
	stp	q2, q3, [x19, #224]
	ldp	q0, q1, [x15, #256]
	ldp	q2, q3, [x15, #288]
	ldp	q4, q5, [x19, #256]
	ldp	q6, q7, [x19, #288]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #256]
	stp	q2, q3, [x19, #288]
	ldp	q0, q1, [x15, #320]
	ldp	q2, q3, [x15, #352]
	ldp	q4, q5, [x19, #320]
	ldp	q6, q7, [x19, #352]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #320]
	stp	q2, q3, [x19, #352]
	ldp	q0, q1, [x15, #384]
	ldp	q2, q3, [x15, #416]
	ldp	q4, q5, [x19, #384]
	ldp	q6, q7, [x19, #416]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #384]
	stp	q2, q3, [x19, #416]
	ldp	q0, q1, [x15, #448]
	ldp	q2, q3, [x15, #480]
	ldp	q4, q5, [x19, #448]
	ldp	q6, q7, [x19, #480]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #448]
	stp	q2, q3, [x19, #480]
LBB5_3:                                 ;   in Loop: Header=BB5_4 Depth=1
	sub	x15, x12, #1
	ands	x12, x15, x12
	b.eq	LBB5_7
LBB5_4:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_6 Depth 2
	rbit	x15, x12
	clz	x15, x15
	eor	w15, w21, w15
	orr	x15, x14, x15
	add	x15, x8, x15, lsl #9
	add	x16, x15, #512
	cmp	x19, x16
	ccmp	x15, x13, #2, lo
	b.hs	LBB5_2
; %bb.5:                                ;   in Loop: Header=BB5_4 Depth=1
	mov	x16, #0                         ; =0x0
LBB5_6:                                 ;   Parent Loop BB5_4 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldrh	w17, [x15, x16]
	ldrh	w0, [x19, x16]
	add	w17, w0, w17
	strh	w17, [x19, x16]
	add	x16, x16, #2
	cmp	x16, #512
	b.ne	LBB5_6
	b	LBB5_3
LBB5_7:
	mov	w12, #56                        ; =0x38
	smaddl	x12, w11, w12, x9
	ldr	x12, [x12, #8]
	cbz	x12, LBB5_14
; %bb.8:
	add	x13, x10, #5, lsl #12           ; =20480
	add	x14, x19, #512
	b	LBB5_11
LBB5_9:                                 ;   in Loop: Header=BB5_11 Depth=1
	ldp	q0, q1, [x15]
	ldp	q2, q3, [x15, #32]
	ldp	q4, q5, [x19]
	ldp	q6, q7, [x19, #32]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19]
	stp	q2, q3, [x19, #32]
	ldp	q0, q1, [x15, #64]
	ldp	q2, q3, [x15, #96]
	ldp	q4, q5, [x19, #64]
	ldp	q6, q7, [x19, #96]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	ldp	q0, q1, [x15, #128]
	ldp	q2, q3, [x15, #160]
	ldp	q4, q5, [x19, #128]
	ldp	q6, q7, [x19, #160]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #128]
	stp	q2, q3, [x19, #160]
	ldp	q0, q1, [x15, #192]
	ldp	q2, q3, [x15, #224]
	ldp	q4, q5, [x19, #192]
	ldp	q6, q7, [x19, #224]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #192]
	stp	q2, q3, [x19, #224]
	ldp	q0, q1, [x15, #256]
	ldp	q2, q3, [x15, #288]
	ldp	q4, q5, [x19, #256]
	ldp	q6, q7, [x19, #288]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #256]
	stp	q2, q3, [x19, #288]
	ldp	q0, q1, [x15, #320]
	ldp	q2, q3, [x15, #352]
	ldp	q4, q5, [x19, #320]
	ldp	q6, q7, [x19, #352]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #320]
	stp	q2, q3, [x19, #352]
	ldp	q0, q1, [x15, #384]
	ldp	q2, q3, [x15, #416]
	ldp	q4, q5, [x19, #384]
	ldp	q6, q7, [x19, #416]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #384]
	stp	q2, q3, [x19, #416]
	ldp	q0, q1, [x15, #448]
	ldp	q2, q3, [x15, #480]
	ldp	q4, q5, [x19, #448]
	ldp	q6, q7, [x19, #480]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #448]
	stp	q2, q3, [x19, #480]
LBB5_10:                                ;   in Loop: Header=BB5_11 Depth=1
	sub	x15, x12, #1
	ands	x12, x15, x12
	b.eq	LBB5_14
LBB5_11:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_13 Depth 2
	rbit	x15, x12
	clz	x15, x15
	eor	w15, w21, w15
	orr	x15, x13, x15
	add	x15, x8, x15, lsl #9
	add	x16, x15, #512
	cmp	x19, x16
	ccmp	x15, x14, #2, lo
	b.hs	LBB5_9
; %bb.12:                               ;   in Loop: Header=BB5_11 Depth=1
	mov	x16, #0                         ; =0x0
LBB5_13:                                ;   Parent Loop BB5_11 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldrh	w17, [x15, x16]
	ldrh	w0, [x19, x16]
	add	w17, w0, w17
	strh	w17, [x19, x16]
	add	x16, x16, #2
	cmp	x16, #512
	b.ne	LBB5_13
	b	LBB5_10
LBB5_14:
	mov	w12, #56                        ; =0x38
	smaddl	x12, w22, w12, x9
	ldr	x12, [x12, #16]
	cbz	x12, LBB5_21
; %bb.15:
	sxtw	x13, w10
	add	x13, x13, #1, lsl #12           ; =4096
	add	x14, x19, #512
	b	LBB5_18
LBB5_16:                                ;   in Loop: Header=BB5_18 Depth=1
	ldp	q0, q1, [x15]
	ldp	q2, q3, [x15, #32]
	ldp	q4, q5, [x19]
	ldp	q6, q7, [x19, #32]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19]
	stp	q2, q3, [x19, #32]
	ldp	q0, q1, [x15, #64]
	ldp	q2, q3, [x15, #96]
	ldp	q4, q5, [x19, #64]
	ldp	q6, q7, [x19, #96]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	ldp	q0, q1, [x15, #128]
	ldp	q2, q3, [x15, #160]
	ldp	q4, q5, [x19, #128]
	ldp	q6, q7, [x19, #160]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #128]
	stp	q2, q3, [x19, #160]
	ldp	q0, q1, [x15, #192]
	ldp	q2, q3, [x15, #224]
	ldp	q4, q5, [x19, #192]
	ldp	q6, q7, [x19, #224]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #192]
	stp	q2, q3, [x19, #224]
	ldp	q0, q1, [x15, #256]
	ldp	q2, q3, [x15, #288]
	ldp	q4, q5, [x19, #256]
	ldp	q6, q7, [x19, #288]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #256]
	stp	q2, q3, [x19, #288]
	ldp	q0, q1, [x15, #320]
	ldp	q2, q3, [x15, #352]
	ldp	q4, q5, [x19, #320]
	ldp	q6, q7, [x19, #352]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #320]
	stp	q2, q3, [x19, #352]
	ldp	q0, q1, [x15, #384]
	ldp	q2, q3, [x15, #416]
	ldp	q4, q5, [x19, #384]
	ldp	q6, q7, [x19, #416]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #384]
	stp	q2, q3, [x19, #416]
	ldp	q0, q1, [x15, #448]
	ldp	q2, q3, [x15, #480]
	ldp	q4, q5, [x19, #448]
	ldp	q6, q7, [x19, #480]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #448]
	stp	q2, q3, [x19, #480]
LBB5_17:                                ;   in Loop: Header=BB5_18 Depth=1
	sub	x15, x12, #1
	ands	x12, x15, x12
	b.eq	LBB5_21
LBB5_18:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_20 Depth 2
	rbit	x15, x12
	clz	x15, x15
	eor	w15, w21, w15
	orr	x15, x13, x15
	add	x15, x8, x15, lsl #9
	add	x16, x15, #512
	cmp	x19, x16
	ccmp	x15, x14, #2, lo
	b.hs	LBB5_16
; %bb.19:                               ;   in Loop: Header=BB5_18 Depth=1
	mov	x16, #0                         ; =0x0
LBB5_20:                                ;   Parent Loop BB5_18 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldrh	w17, [x15, x16]
	ldrh	w0, [x19, x16]
	add	w17, w0, w17
	strh	w17, [x19, x16]
	add	x16, x16, #2
	cmp	x16, #512
	b.ne	LBB5_20
	b	LBB5_17
LBB5_21:
	mov	w12, #56                        ; =0x38
	smaddl	x12, w11, w12, x9
	ldr	x12, [x12, #16]
	cbz	x12, LBB5_28
; %bb.22:
	sxtw	x13, w10
	add	x13, x13, #6, lsl #12           ; =24576
	add	x14, x19, #512
	b	LBB5_25
LBB5_23:                                ;   in Loop: Header=BB5_25 Depth=1
	ldp	q0, q1, [x15]
	ldp	q2, q3, [x15, #32]
	ldp	q4, q5, [x19]
	ldp	q6, q7, [x19, #32]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19]
	stp	q2, q3, [x19, #32]
	ldp	q0, q1, [x15, #64]
	ldp	q2, q3, [x15, #96]
	ldp	q4, q5, [x19, #64]
	ldp	q6, q7, [x19, #96]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	ldp	q0, q1, [x15, #128]
	ldp	q2, q3, [x15, #160]
	ldp	q4, q5, [x19, #128]
	ldp	q6, q7, [x19, #160]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #128]
	stp	q2, q3, [x19, #160]
	ldp	q0, q1, [x15, #192]
	ldp	q2, q3, [x15, #224]
	ldp	q4, q5, [x19, #192]
	ldp	q6, q7, [x19, #224]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #192]
	stp	q2, q3, [x19, #224]
	ldp	q0, q1, [x15, #256]
	ldp	q2, q3, [x15, #288]
	ldp	q4, q5, [x19, #256]
	ldp	q6, q7, [x19, #288]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #256]
	stp	q2, q3, [x19, #288]
	ldp	q0, q1, [x15, #320]
	ldp	q2, q3, [x15, #352]
	ldp	q4, q5, [x19, #320]
	ldp	q6, q7, [x19, #352]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #320]
	stp	q2, q3, [x19, #352]
	ldp	q0, q1, [x15, #384]
	ldp	q2, q3, [x15, #416]
	ldp	q4, q5, [x19, #384]
	ldp	q6, q7, [x19, #416]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #384]
	stp	q2, q3, [x19, #416]
	ldp	q0, q1, [x15, #448]
	ldp	q2, q3, [x15, #480]
	ldp	q4, q5, [x19, #448]
	ldp	q6, q7, [x19, #480]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #448]
	stp	q2, q3, [x19, #480]
LBB5_24:                                ;   in Loop: Header=BB5_25 Depth=1
	sub	x15, x12, #1
	ands	x12, x15, x12
	b.eq	LBB5_28
LBB5_25:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_27 Depth 2
	rbit	x15, x12
	clz	x15, x15
	eor	w15, w21, w15
	orr	x15, x13, x15
	add	x15, x8, x15, lsl #9
	add	x16, x15, #512
	cmp	x19, x16
	ccmp	x15, x14, #2, lo
	b.hs	LBB5_23
; %bb.26:                               ;   in Loop: Header=BB5_25 Depth=1
	mov	x16, #0                         ; =0x0
LBB5_27:                                ;   Parent Loop BB5_25 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldrh	w17, [x15, x16]
	ldrh	w0, [x19, x16]
	add	w17, w0, w17
	strh	w17, [x19, x16]
	add	x16, x16, #2
	cmp	x16, #512
	b.ne	LBB5_27
	b	LBB5_24
LBB5_28:
	mov	w12, #56                        ; =0x38
	smaddl	x12, w22, w12, x9
	ldr	x12, [x12, #24]
	cbz	x12, LBB5_35
; %bb.29:
	sxtw	x13, w10
	add	x13, x13, #2, lsl #12           ; =8192
	add	x14, x19, #512
	b	LBB5_32
LBB5_30:                                ;   in Loop: Header=BB5_32 Depth=1
	ldp	q0, q1, [x15]
	ldp	q2, q3, [x15, #32]
	ldp	q4, q5, [x19]
	ldp	q6, q7, [x19, #32]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19]
	stp	q2, q3, [x19, #32]
	ldp	q0, q1, [x15, #64]
	ldp	q2, q3, [x15, #96]
	ldp	q4, q5, [x19, #64]
	ldp	q6, q7, [x19, #96]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	ldp	q0, q1, [x15, #128]
	ldp	q2, q3, [x15, #160]
	ldp	q4, q5, [x19, #128]
	ldp	q6, q7, [x19, #160]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #128]
	stp	q2, q3, [x19, #160]
	ldp	q0, q1, [x15, #192]
	ldp	q2, q3, [x15, #224]
	ldp	q4, q5, [x19, #192]
	ldp	q6, q7, [x19, #224]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #192]
	stp	q2, q3, [x19, #224]
	ldp	q0, q1, [x15, #256]
	ldp	q2, q3, [x15, #288]
	ldp	q4, q5, [x19, #256]
	ldp	q6, q7, [x19, #288]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #256]
	stp	q2, q3, [x19, #288]
	ldp	q0, q1, [x15, #320]
	ldp	q2, q3, [x15, #352]
	ldp	q4, q5, [x19, #320]
	ldp	q6, q7, [x19, #352]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #320]
	stp	q2, q3, [x19, #352]
	ldp	q0, q1, [x15, #384]
	ldp	q2, q3, [x15, #416]
	ldp	q4, q5, [x19, #384]
	ldp	q6, q7, [x19, #416]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #384]
	stp	q2, q3, [x19, #416]
	ldp	q0, q1, [x15, #448]
	ldp	q2, q3, [x15, #480]
	ldp	q4, q5, [x19, #448]
	ldp	q6, q7, [x19, #480]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #448]
	stp	q2, q3, [x19, #480]
LBB5_31:                                ;   in Loop: Header=BB5_32 Depth=1
	sub	x15, x12, #1
	ands	x12, x15, x12
	b.eq	LBB5_35
LBB5_32:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_34 Depth 2
	rbit	x15, x12
	clz	x15, x15
	eor	w15, w21, w15
	orr	x15, x13, x15
	add	x15, x8, x15, lsl #9
	add	x16, x15, #512
	cmp	x19, x16
	ccmp	x15, x14, #2, lo
	b.hs	LBB5_30
; %bb.33:                               ;   in Loop: Header=BB5_32 Depth=1
	mov	x16, #0                         ; =0x0
LBB5_34:                                ;   Parent Loop BB5_32 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldrh	w17, [x15, x16]
	ldrh	w0, [x19, x16]
	add	w17, w0, w17
	strh	w17, [x19, x16]
	add	x16, x16, #2
	cmp	x16, #512
	b.ne	LBB5_34
	b	LBB5_31
LBB5_35:
	mov	w12, #56                        ; =0x38
	smaddl	x12, w11, w12, x9
	ldr	x12, [x12, #24]
	cbz	x12, LBB5_42
; %bb.36:
	sxtw	x13, w10
	add	x13, x13, #7, lsl #12           ; =28672
	add	x14, x19, #512
	b	LBB5_39
LBB5_37:                                ;   in Loop: Header=BB5_39 Depth=1
	ldp	q0, q1, [x15]
	ldp	q2, q3, [x15, #32]
	ldp	q4, q5, [x19]
	ldp	q6, q7, [x19, #32]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19]
	stp	q2, q3, [x19, #32]
	ldp	q0, q1, [x15, #64]
	ldp	q2, q3, [x15, #96]
	ldp	q4, q5, [x19, #64]
	ldp	q6, q7, [x19, #96]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	ldp	q0, q1, [x15, #128]
	ldp	q2, q3, [x15, #160]
	ldp	q4, q5, [x19, #128]
	ldp	q6, q7, [x19, #160]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #128]
	stp	q2, q3, [x19, #160]
	ldp	q0, q1, [x15, #192]
	ldp	q2, q3, [x15, #224]
	ldp	q4, q5, [x19, #192]
	ldp	q6, q7, [x19, #224]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #192]
	stp	q2, q3, [x19, #224]
	ldp	q0, q1, [x15, #256]
	ldp	q2, q3, [x15, #288]
	ldp	q4, q5, [x19, #256]
	ldp	q6, q7, [x19, #288]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #256]
	stp	q2, q3, [x19, #288]
	ldp	q0, q1, [x15, #320]
	ldp	q2, q3, [x15, #352]
	ldp	q4, q5, [x19, #320]
	ldp	q6, q7, [x19, #352]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #320]
	stp	q2, q3, [x19, #352]
	ldp	q0, q1, [x15, #384]
	ldp	q2, q3, [x15, #416]
	ldp	q4, q5, [x19, #384]
	ldp	q6, q7, [x19, #416]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #384]
	stp	q2, q3, [x19, #416]
	ldp	q0, q1, [x15, #448]
	ldp	q2, q3, [x15, #480]
	ldp	q4, q5, [x19, #448]
	ldp	q6, q7, [x19, #480]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #448]
	stp	q2, q3, [x19, #480]
LBB5_38:                                ;   in Loop: Header=BB5_39 Depth=1
	sub	x15, x12, #1
	ands	x12, x15, x12
	b.eq	LBB5_42
LBB5_39:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_41 Depth 2
	rbit	x15, x12
	clz	x15, x15
	eor	w15, w21, w15
	orr	x15, x13, x15
	add	x15, x8, x15, lsl #9
	add	x16, x15, #512
	cmp	x19, x16
	ccmp	x15, x14, #2, lo
	b.hs	LBB5_37
; %bb.40:                               ;   in Loop: Header=BB5_39 Depth=1
	mov	x16, #0                         ; =0x0
LBB5_41:                                ;   Parent Loop BB5_39 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldrh	w17, [x15, x16]
	ldrh	w0, [x19, x16]
	add	w17, w0, w17
	strh	w17, [x19, x16]
	add	x16, x16, #2
	cmp	x16, #512
	b.ne	LBB5_41
	b	LBB5_38
LBB5_42:
	mov	w12, #56                        ; =0x38
	smaddl	x12, w22, w12, x9
	ldr	x12, [x12, #32]
	cbz	x12, LBB5_49
; %bb.43:
	sxtw	x13, w10
	add	x13, x13, #3, lsl #12           ; =12288
	add	x14, x19, #512
	b	LBB5_46
LBB5_44:                                ;   in Loop: Header=BB5_46 Depth=1
	ldp	q0, q1, [x15]
	ldp	q2, q3, [x15, #32]
	ldp	q4, q5, [x19]
	ldp	q6, q7, [x19, #32]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19]
	stp	q2, q3, [x19, #32]
	ldp	q0, q1, [x15, #64]
	ldp	q2, q3, [x15, #96]
	ldp	q4, q5, [x19, #64]
	ldp	q6, q7, [x19, #96]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	ldp	q0, q1, [x15, #128]
	ldp	q2, q3, [x15, #160]
	ldp	q4, q5, [x19, #128]
	ldp	q6, q7, [x19, #160]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #128]
	stp	q2, q3, [x19, #160]
	ldp	q0, q1, [x15, #192]
	ldp	q2, q3, [x15, #224]
	ldp	q4, q5, [x19, #192]
	ldp	q6, q7, [x19, #224]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #192]
	stp	q2, q3, [x19, #224]
	ldp	q0, q1, [x15, #256]
	ldp	q2, q3, [x15, #288]
	ldp	q4, q5, [x19, #256]
	ldp	q6, q7, [x19, #288]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #256]
	stp	q2, q3, [x19, #288]
	ldp	q0, q1, [x15, #320]
	ldp	q2, q3, [x15, #352]
	ldp	q4, q5, [x19, #320]
	ldp	q6, q7, [x19, #352]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #320]
	stp	q2, q3, [x19, #352]
	ldp	q0, q1, [x15, #384]
	ldp	q2, q3, [x15, #416]
	ldp	q4, q5, [x19, #384]
	ldp	q6, q7, [x19, #416]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #384]
	stp	q2, q3, [x19, #416]
	ldp	q0, q1, [x15, #448]
	ldp	q2, q3, [x15, #480]
	ldp	q4, q5, [x19, #448]
	ldp	q6, q7, [x19, #480]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #448]
	stp	q2, q3, [x19, #480]
LBB5_45:                                ;   in Loop: Header=BB5_46 Depth=1
	sub	x15, x12, #1
	ands	x12, x15, x12
	b.eq	LBB5_49
LBB5_46:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_48 Depth 2
	rbit	x15, x12
	clz	x15, x15
	eor	w15, w21, w15
	orr	x15, x13, x15
	add	x15, x8, x15, lsl #9
	add	x16, x15, #512
	cmp	x19, x16
	ccmp	x15, x14, #2, lo
	b.hs	LBB5_44
; %bb.47:                               ;   in Loop: Header=BB5_46 Depth=1
	mov	x16, #0                         ; =0x0
LBB5_48:                                ;   Parent Loop BB5_46 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldrh	w17, [x15, x16]
	ldrh	w0, [x19, x16]
	add	w17, w0, w17
	strh	w17, [x19, x16]
	add	x16, x16, #2
	cmp	x16, #512
	b.ne	LBB5_48
	b	LBB5_45
LBB5_49:
	mov	w12, #56                        ; =0x38
	smaddl	x12, w11, w12, x9
	ldr	x12, [x12, #32]
	cbz	x12, LBB5_56
; %bb.50:
	sxtw	x13, w10
	add	x13, x13, #8, lsl #12           ; =32768
	add	x14, x19, #512
	b	LBB5_53
LBB5_51:                                ;   in Loop: Header=BB5_53 Depth=1
	ldp	q0, q1, [x15]
	ldp	q2, q3, [x15, #32]
	ldp	q4, q5, [x19]
	ldp	q6, q7, [x19, #32]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19]
	stp	q2, q3, [x19, #32]
	ldp	q0, q1, [x15, #64]
	ldp	q2, q3, [x15, #96]
	ldp	q4, q5, [x19, #64]
	ldp	q6, q7, [x19, #96]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	ldp	q0, q1, [x15, #128]
	ldp	q2, q3, [x15, #160]
	ldp	q4, q5, [x19, #128]
	ldp	q6, q7, [x19, #160]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #128]
	stp	q2, q3, [x19, #160]
	ldp	q0, q1, [x15, #192]
	ldp	q2, q3, [x15, #224]
	ldp	q4, q5, [x19, #192]
	ldp	q6, q7, [x19, #224]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #192]
	stp	q2, q3, [x19, #224]
	ldp	q0, q1, [x15, #256]
	ldp	q2, q3, [x15, #288]
	ldp	q4, q5, [x19, #256]
	ldp	q6, q7, [x19, #288]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #256]
	stp	q2, q3, [x19, #288]
	ldp	q0, q1, [x15, #320]
	ldp	q2, q3, [x15, #352]
	ldp	q4, q5, [x19, #320]
	ldp	q6, q7, [x19, #352]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #320]
	stp	q2, q3, [x19, #352]
	ldp	q0, q1, [x15, #384]
	ldp	q2, q3, [x15, #416]
	ldp	q4, q5, [x19, #384]
	ldp	q6, q7, [x19, #416]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #384]
	stp	q2, q3, [x19, #416]
	ldp	q0, q1, [x15, #448]
	ldp	q2, q3, [x15, #480]
	ldp	q4, q5, [x19, #448]
	ldp	q6, q7, [x19, #480]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #448]
	stp	q2, q3, [x19, #480]
LBB5_52:                                ;   in Loop: Header=BB5_53 Depth=1
	sub	x15, x12, #1
	ands	x12, x15, x12
	b.eq	LBB5_56
LBB5_53:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_55 Depth 2
	rbit	x15, x12
	clz	x15, x15
	eor	w15, w21, w15
	orr	x15, x13, x15
	add	x15, x8, x15, lsl #9
	add	x16, x15, #512
	cmp	x19, x16
	ccmp	x15, x14, #2, lo
	b.hs	LBB5_51
; %bb.54:                               ;   in Loop: Header=BB5_53 Depth=1
	mov	x16, #0                         ; =0x0
LBB5_55:                                ;   Parent Loop BB5_53 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldrh	w17, [x15, x16]
	ldrh	w0, [x19, x16]
	add	w17, w0, w17
	strh	w17, [x19, x16]
	add	x16, x16, #2
	cmp	x16, #512
	b.ne	LBB5_55
	b	LBB5_52
LBB5_56:
	mov	w12, #56                        ; =0x38
	smaddl	x12, w22, w12, x9
	ldr	x12, [x12, #40]
	cbz	x12, LBB5_63
; %bb.57:
	sxtw	x13, w10
	add	x13, x13, #4, lsl #12           ; =16384
	add	x14, x19, #512
	b	LBB5_60
LBB5_58:                                ;   in Loop: Header=BB5_60 Depth=1
	ldp	q0, q1, [x15]
	ldp	q2, q3, [x15, #32]
	ldp	q4, q5, [x19]
	ldp	q6, q7, [x19, #32]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19]
	stp	q2, q3, [x19, #32]
	ldp	q0, q1, [x15, #64]
	ldp	q2, q3, [x15, #96]
	ldp	q4, q5, [x19, #64]
	ldp	q6, q7, [x19, #96]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	ldp	q0, q1, [x15, #128]
	ldp	q2, q3, [x15, #160]
	ldp	q4, q5, [x19, #128]
	ldp	q6, q7, [x19, #160]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #128]
	stp	q2, q3, [x19, #160]
	ldp	q0, q1, [x15, #192]
	ldp	q2, q3, [x15, #224]
	ldp	q4, q5, [x19, #192]
	ldp	q6, q7, [x19, #224]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #192]
	stp	q2, q3, [x19, #224]
	ldp	q0, q1, [x15, #256]
	ldp	q2, q3, [x15, #288]
	ldp	q4, q5, [x19, #256]
	ldp	q6, q7, [x19, #288]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #256]
	stp	q2, q3, [x19, #288]
	ldp	q0, q1, [x15, #320]
	ldp	q2, q3, [x15, #352]
	ldp	q4, q5, [x19, #320]
	ldp	q6, q7, [x19, #352]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #320]
	stp	q2, q3, [x19, #352]
	ldp	q0, q1, [x15, #384]
	ldp	q2, q3, [x15, #416]
	ldp	q4, q5, [x19, #384]
	ldp	q6, q7, [x19, #416]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #384]
	stp	q2, q3, [x19, #416]
	ldp	q0, q1, [x15, #448]
	ldp	q2, q3, [x15, #480]
	ldp	q4, q5, [x19, #448]
	ldp	q6, q7, [x19, #480]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #448]
	stp	q2, q3, [x19, #480]
LBB5_59:                                ;   in Loop: Header=BB5_60 Depth=1
	sub	x15, x12, #1
	ands	x12, x15, x12
	b.eq	LBB5_63
LBB5_60:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_62 Depth 2
	rbit	x15, x12
	clz	x15, x15
	eor	w15, w21, w15
	orr	x15, x13, x15
	add	x15, x8, x15, lsl #9
	add	x16, x15, #512
	cmp	x19, x16
	ccmp	x15, x14, #2, lo
	b.hs	LBB5_58
; %bb.61:                               ;   in Loop: Header=BB5_60 Depth=1
	mov	x16, #0                         ; =0x0
LBB5_62:                                ;   Parent Loop BB5_60 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldrh	w17, [x15, x16]
	ldrh	w0, [x19, x16]
	add	w17, w0, w17
	strh	w17, [x19, x16]
	add	x16, x16, #2
	cmp	x16, #512
	b.ne	LBB5_62
	b	LBB5_59
LBB5_63:
	mov	w12, #56                        ; =0x38
	smaddl	x9, w11, w12, x9
	ldr	x9, [x9, #40]
	cbz	x9, LBB5_70
; %bb.64:
                                        ; kill: def $w10 killed $w10 killed $x10 def $x10
	sxtw	x10, w10
	add	x10, x10, #9, lsl #12           ; =36864
	add	x11, x19, #512
	b	LBB5_67
LBB5_65:                                ;   in Loop: Header=BB5_67 Depth=1
	ldp	q0, q1, [x12]
	ldp	q2, q3, [x12, #32]
	ldp	q4, q5, [x19]
	ldp	q6, q7, [x19, #32]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19]
	stp	q2, q3, [x19, #32]
	ldp	q0, q1, [x12, #64]
	ldp	q2, q3, [x12, #96]
	ldp	q4, q5, [x19, #64]
	ldp	q6, q7, [x19, #96]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	ldp	q0, q1, [x12, #128]
	ldp	q2, q3, [x12, #160]
	ldp	q4, q5, [x19, #128]
	ldp	q6, q7, [x19, #160]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #128]
	stp	q2, q3, [x19, #160]
	ldp	q0, q1, [x12, #192]
	ldp	q2, q3, [x12, #224]
	ldp	q4, q5, [x19, #192]
	ldp	q6, q7, [x19, #224]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #192]
	stp	q2, q3, [x19, #224]
	ldp	q0, q1, [x12, #256]
	ldp	q2, q3, [x12, #288]
	ldp	q4, q5, [x19, #256]
	ldp	q6, q7, [x19, #288]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #256]
	stp	q2, q3, [x19, #288]
	ldp	q0, q1, [x12, #320]
	ldp	q2, q3, [x12, #352]
	ldp	q4, q5, [x19, #320]
	ldp	q6, q7, [x19, #352]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #320]
	stp	q2, q3, [x19, #352]
	ldp	q0, q1, [x12, #384]
	ldp	q2, q3, [x12, #416]
	ldp	q4, q5, [x19, #384]
	ldp	q6, q7, [x19, #416]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #384]
	stp	q2, q3, [x19, #416]
	ldp	q0, q1, [x12, #448]
	ldp	q2, q3, [x12, #480]
	ldp	q4, q5, [x19, #448]
	ldp	q6, q7, [x19, #480]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x19, #448]
	stp	q2, q3, [x19, #480]
LBB5_66:                                ;   in Loop: Header=BB5_67 Depth=1
	sub	x12, x9, #1
	ands	x9, x12, x9
	b.eq	LBB5_70
LBB5_67:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_69 Depth 2
	rbit	x12, x9
	clz	x12, x12
	eor	w12, w21, w12
	orr	x12, x10, x12
	add	x12, x8, x12, lsl #9
	add	x13, x12, #512
	cmp	x19, x13
	ccmp	x12, x11, #2, lo
	b.hs	LBB5_65
; %bb.68:                               ;   in Loop: Header=BB5_67 Depth=1
	mov	x13, #0                         ; =0x0
LBB5_69:                                ;   Parent Loop BB5_67 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldrh	w14, [x12, x13]
	ldrh	w15, [x19, x13]
	add	w14, w15, w14
	strh	w14, [x19, x13]
	add	x13, x13, #2
	cmp	x13, #512
	b.ne	LBB5_69
	b	LBB5_66
LBB5_70:
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
	.loh AdrpLdrGot	Lloh64, Lloh65
	.loh AdrpLdrGot	Lloh62, Lloh63
	.cfi_endproc
                                        ; -- End function
	.globl	_EvaluatePositionNeuralNetwork  ; -- Begin function EvaluatePositionNeuralNetwork
	.p2align	2
_EvaluatePositionNeuralNetwork:         ; @EvaluatePositionNeuralNetwork
	.cfi_startproc
; %bb.0:
	stp	d15, d14, [sp, #-112]!          ; 16-byte Folded Spill
	stp	d13, d12, [sp, #16]             ; 16-byte Folded Spill
	stp	d11, d10, [sp, #32]             ; 16-byte Folded Spill
	stp	d9, d8, [sp, #48]               ; 16-byte Folded Spill
	stp	x28, x27, [sp, #64]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #80]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #96]             ; 16-byte Folded Spill
	add	x29, sp, #96
	sub	sp, sp, #2176
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w27, -40
	.cfi_offset w28, -48
	.cfi_offset b8, -56
	.cfi_offset b9, -64
	.cfi_offset b10, -72
	.cfi_offset b11, -80
	.cfi_offset b12, -88
	.cfi_offset b13, -96
	.cfi_offset b14, -104
	.cfi_offset b15, -112
	mov	x19, x0
Lloh66:
	adrp	x8, ___stack_chk_guard@GOTPAGE
Lloh67:
	ldr	x8, [x8, ___stack_chk_guard@GOTPAGEOFF]
Lloh68:
	ldr	x8, [x8]
	stur	x8, [x29, #-104]
Lloh69:
	adrp	x8, _STry@GOTPAGE
Lloh70:
	ldr	x8, [x8, _STry@GOTPAGEOFF]
	ldr	x9, [x8]
	add	x9, x9, #1
	str	x9, [x8]
	ldr	x0, [x0, #1144]
	add	x1, sp, #1900
	bl	_ProbeST
	cmp	w0, #3
	b.ne	LBB6_2
; %bb.1:
Lloh71:
	adrp	x8, _SHit@GOTPAGE
Lloh72:
	ldr	x8, [x8, _SHit@GOTPAGEOFF]
	ldr	x9, [x8]
	add	x9, x9, #1
	str	x9, [x8]
	b	LBB6_9
LBB6_2:
	mov	x8, #0                          ; =0x0
	movi.2d	v0, #0000000000000000
	ldrb	w20, [x19, #2292]
	cmp	w20, #0
	mov	w9, #1196                       ; =0x4ac
	mov	w10, #1708                      ; =0x6ac
	csel	x11, x9, x10, eq
	add	x11, x19, x11
	ldp	q1, q2, [x11]
	smax.8h	v1, v1, v0
	smax.8h	v2, v2, v0
	ldp	q3, q4, [x11, #32]
	smax.8h	v3, v3, v0
	smax.8h	v4, v4, v0
	ldp	q5, q6, [x11, #64]
	smax.8h	v5, v5, v0
	smax.8h	v6, v6, v0
	ldp	q7, q16, [x11, #96]
	smax.8h	v7, v7, v0
	smax.8h	v16, v16, v0
	ldp	q17, q18, [x11, #128]
	smax.8h	v17, v17, v0
	smax.8h	v18, v18, v0
	ldp	q19, q20, [x11, #160]
	smax.8h	v19, v19, v0
	smax.8h	v20, v20, v0
	ldp	q21, q22, [x11, #192]
	smax.8h	v21, v21, v0
	smax.8h	v22, v22, v0
	ldp	q23, q24, [x11, #224]
	smax.8h	v23, v23, v0
	smax.8h	v24, v24, v0
	ldp	q25, q26, [x11, #256]
	smax.8h	v25, v25, v0
	smax.8h	v26, v26, v0
	ldp	q27, q28, [x11, #288]
	smax.8h	v27, v27, v0
	smax.8h	v28, v28, v0
	ldp	q29, q30, [x11, #320]
	smax.8h	v29, v29, v0
	smax.8h	v30, v30, v0
	ldp	q31, q8, [x11, #352]
	smax.8h	v31, v31, v0
	smax.8h	v8, v8, v0
	ldp	q9, q10, [x11, #384]
	smax.8h	v9, v9, v0
	smax.8h	v10, v10, v0
	ldp	q11, q12, [x11, #416]
	smax.8h	v11, v11, v0
	smax.8h	v12, v12, v0
	ldp	q13, q14, [x11, #448]
	smax.8h	v13, v13, v0
	smax.8h	v14, v14, v0
	ldr	q15, [x11, #480]
	smax.8h	v15, v15, v0
	str	q15, [sp, #896]                 ; 16-byte Folded Spill
	ldr	q15, [x11, #496]
	smax.8h	v15, v15, v0
	str	q15, [sp, #864]                 ; 16-byte Folded Spill
	csel	x9, x10, x9, eq
	add	x9, x19, x9
	ldr	q15, [x9]
	smax.8h	v15, v15, v0
	str	q15, [sp, #832]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #16]
	smax.8h	v15, v15, v0
	str	q15, [sp, #800]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #32]
	smax.8h	v15, v15, v0
	str	q15, [sp, #768]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #48]
	smax.8h	v15, v15, v0
	str	q15, [sp, #736]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #64]
	smax.8h	v15, v15, v0
	str	q15, [sp, #704]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #80]
	smax.8h	v15, v15, v0
	str	q15, [sp, #672]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #96]
	smax.8h	v15, v15, v0
	str	q15, [sp, #640]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #112]
	smax.8h	v15, v15, v0
	str	q15, [sp, #608]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #128]
	smax.8h	v15, v15, v0
	str	q15, [sp, #576]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #144]
	smax.8h	v15, v15, v0
	str	q15, [sp, #544]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #160]
	smax.8h	v15, v15, v0
	str	q15, [sp, #512]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #176]
	smax.8h	v15, v15, v0
	str	q15, [sp, #480]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #192]
	smax.8h	v15, v15, v0
	str	q15, [sp, #448]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #208]
	smax.8h	v15, v15, v0
	str	q15, [sp, #416]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #224]
	smax.8h	v15, v15, v0
	str	q15, [sp, #384]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #240]
	smax.8h	v15, v15, v0
	str	q15, [sp, #352]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #256]
	smax.8h	v15, v15, v0
	str	q15, [sp, #320]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #272]
	smax.8h	v15, v15, v0
	str	q15, [sp, #288]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #288]
	smax.8h	v15, v15, v0
	str	q15, [sp, #256]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #304]
	smax.8h	v15, v15, v0
	str	q15, [sp, #224]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #320]
	smax.8h	v15, v15, v0
	str	q15, [sp, #192]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #336]
	smax.8h	v15, v15, v0
	str	q15, [sp, #160]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #352]
	smax.8h	v15, v15, v0
	str	q15, [sp]                       ; 16-byte Folded Spill
	ldr	q15, [x9, #368]
	smax.8h	v15, v15, v0
	str	q15, [sp, #16]                  ; 16-byte Folded Spill
	ldr	q15, [x9, #384]
	smax.8h	v15, v15, v0
	str	q15, [sp, #32]                  ; 16-byte Folded Spill
	ldr	q15, [x9, #400]
	smax.8h	v15, v15, v0
	str	q15, [sp, #48]                  ; 16-byte Folded Spill
	ldr	q15, [x9, #416]
	smax.8h	v15, v15, v0
	str	q15, [sp, #64]                  ; 16-byte Folded Spill
	ldr	q15, [x9, #432]
	smax.8h	v15, v15, v0
	str	q15, [sp, #80]                  ; 16-byte Folded Spill
	ldr	q15, [x9, #448]
	smax.8h	v15, v15, v0
	str	q15, [sp, #96]                  ; 16-byte Folded Spill
	ldr	q15, [x9, #464]
	smax.8h	v15, v15, v0
	str	q15, [sp, #112]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #480]
	smax.8h	v15, v15, v0
	str	q15, [sp, #128]                 ; 16-byte Folded Spill
	ldr	q15, [x9, #496]
	smax.8h	v0, v15, v0
	str	q0, [sp, #144]                  ; 16-byte Folded Spill
	sub	x9, x29, #240
Lloh73:
	adrp	x10, _hidden_layer_1_bias_q@GOTPAGE
Lloh74:
	ldr	x10, [x10, _hidden_layer_1_bias_q@GOTPAGEOFF]
	ldp	q0, q15, [x10, #64]
	stp	q0, q15, [x9, #64]
	ldp	q0, q15, [x10, #96]
	stp	q0, q15, [x9, #96]
	ldp	q0, q15, [x10]
	stp	q0, q15, [x9]
	ldp	q0, q15, [x10, #32]
	stp	q0, q15, [x9, #32]
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #1872]                 ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #1856]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v2, #0
	str	q0, [sp, #1840]                 ; 16-byte Folded Spill
	ushll.4s	v0, v2, #0
	str	q0, [sp, #1824]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v3, #0
	str	q0, [sp, #1808]                 ; 16-byte Folded Spill
	ushll.4s	v0, v3, #0
	str	q0, [sp, #1792]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v4, #0
	str	q0, [sp, #1776]                 ; 16-byte Folded Spill
	ushll.4s	v0, v4, #0
	str	q0, [sp, #1760]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v5, #0
	str	q0, [sp, #1744]                 ; 16-byte Folded Spill
	ushll.4s	v0, v5, #0
	str	q0, [sp, #1728]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v6, #0
	str	q0, [sp, #1712]                 ; 16-byte Folded Spill
	ushll.4s	v0, v6, #0
	str	q0, [sp, #1696]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v7, #0
	str	q0, [sp, #1680]                 ; 16-byte Folded Spill
	ushll.4s	v0, v7, #0
	str	q0, [sp, #1664]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v16, #0
	str	q0, [sp, #1648]                 ; 16-byte Folded Spill
	ushll.4s	v0, v16, #0
	str	q0, [sp, #1632]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v17, #0
	str	q0, [sp, #1616]                 ; 16-byte Folded Spill
	ushll.4s	v0, v17, #0
	str	q0, [sp, #1600]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v18, #0
	str	q0, [sp, #1584]                 ; 16-byte Folded Spill
	ushll.4s	v0, v18, #0
	str	q0, [sp, #1568]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v19, #0
	str	q0, [sp, #1552]                 ; 16-byte Folded Spill
	ushll.4s	v0, v19, #0
	str	q0, [sp, #1536]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v20, #0
	str	q0, [sp, #1520]                 ; 16-byte Folded Spill
	ushll.4s	v0, v20, #0
	str	q0, [sp, #1504]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v21, #0
	str	q0, [sp, #1488]                 ; 16-byte Folded Spill
	ushll.4s	v0, v21, #0
	str	q0, [sp, #1472]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v22, #0
	str	q0, [sp, #1456]                 ; 16-byte Folded Spill
	ushll.4s	v0, v22, #0
	str	q0, [sp, #1440]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v23, #0
	str	q0, [sp, #1424]                 ; 16-byte Folded Spill
	ushll.4s	v0, v23, #0
	str	q0, [sp, #1408]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v24, #0
	str	q0, [sp, #1392]                 ; 16-byte Folded Spill
	ushll.4s	v0, v24, #0
	str	q0, [sp, #1376]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v25, #0
	str	q0, [sp, #1360]                 ; 16-byte Folded Spill
	ushll.4s	v0, v25, #0
	str	q0, [sp, #1344]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v26, #0
	str	q0, [sp, #1328]                 ; 16-byte Folded Spill
	ushll.4s	v0, v26, #0
	str	q0, [sp, #1312]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v27, #0
	str	q0, [sp, #1296]                 ; 16-byte Folded Spill
	ushll.4s	v0, v27, #0
	str	q0, [sp, #1280]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v28, #0
	str	q0, [sp, #1264]                 ; 16-byte Folded Spill
	ushll.4s	v0, v28, #0
	str	q0, [sp, #1248]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v29, #0
	str	q0, [sp, #1232]                 ; 16-byte Folded Spill
	ushll.4s	v0, v29, #0
	str	q0, [sp, #1216]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v30, #0
	str	q0, [sp, #1200]                 ; 16-byte Folded Spill
	ushll.4s	v0, v30, #0
	str	q0, [sp, #1184]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v31, #0
	str	q0, [sp, #1168]                 ; 16-byte Folded Spill
	ushll.4s	v0, v31, #0
	str	q0, [sp, #1152]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v8, #0
	str	q0, [sp, #1136]                 ; 16-byte Folded Spill
	ushll.4s	v0, v8, #0
	str	q0, [sp, #1120]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v9, #0
	str	q0, [sp, #1104]                 ; 16-byte Folded Spill
	ushll.4s	v0, v9, #0
	str	q0, [sp, #1088]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v10, #0
	str	q0, [sp, #1072]                 ; 16-byte Folded Spill
	ushll.4s	v0, v10, #0
	str	q0, [sp, #1056]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v11, #0
	str	q0, [sp, #1040]                 ; 16-byte Folded Spill
	ushll.4s	v0, v11, #0
	str	q0, [sp, #1024]                 ; 16-byte Folded Spill
	ushll2.4s	v0, v12, #0
	str	q0, [sp, #1008]                 ; 16-byte Folded Spill
	ushll.4s	v0, v12, #0
	str	q0, [sp, #992]                  ; 16-byte Folded Spill
	ushll2.4s	v0, v13, #0
	str	q0, [sp, #976]                  ; 16-byte Folded Spill
	ushll.4s	v0, v13, #0
	str	q0, [sp, #960]                  ; 16-byte Folded Spill
	ushll2.4s	v0, v14, #0
	str	q0, [sp, #944]                  ; 16-byte Folded Spill
	ushll.4s	v0, v14, #0
	str	q0, [sp, #928]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #896]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #912]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #896]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #864]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #880]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #864]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #832]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #848]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #832]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #800]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #816]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #800]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #768]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #784]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #768]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #736]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #752]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #736]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #704]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #720]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #704]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #672]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #688]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #672]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #640]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #656]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #640]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #608]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #624]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #608]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #576]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #592]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #576]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #544]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #560]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #544]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #512]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #528]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #512]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #480]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #496]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #480]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #448]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #464]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #448]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #416]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #432]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #416]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #384]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #400]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #384]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #352]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #368]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #352]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #320]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #336]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #320]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #288]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #304]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #288]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #256]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #272]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #256]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #224]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #240]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #224]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #192]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #208]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #192]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #160]                  ; 16-byte Folded Reload
	ushll2.4s	v0, v1, #0
	str	q0, [sp, #176]                  ; 16-byte Folded Spill
	ushll.4s	v0, v1, #0
	str	q0, [sp, #160]                  ; 16-byte Folded Spill
	ldr	q0, [sp]                        ; 16-byte Folded Reload
	ushll2.4s	v20, v0, #0
	ushll.4s	v21, v0, #0
	ldr	q0, [sp, #16]                   ; 16-byte Folded Reload
	ushll2.4s	v22, v0, #0
	ushll.4s	v23, v0, #0
	ldr	q0, [sp, #32]                   ; 16-byte Folded Reload
	ushll2.4s	v24, v0, #0
	ushll.4s	v25, v0, #0
	ldr	q0, [sp, #48]                   ; 16-byte Folded Reload
	ushll2.4s	v26, v0, #0
	ushll.4s	v27, v0, #0
	ldr	q0, [sp, #64]                   ; 16-byte Folded Reload
	ushll2.4s	v28, v0, #0
	ushll.4s	v29, v0, #0
	ldr	q0, [sp, #80]                   ; 16-byte Folded Reload
	ushll2.4s	v30, v0, #0
	ushll.4s	v31, v0, #0
	ldr	q0, [sp, #96]                   ; 16-byte Folded Reload
	ushll2.4s	v8, v0, #0
	ushll.4s	v9, v0, #0
	ldr	q0, [sp, #112]                  ; 16-byte Folded Reload
	ushll2.4s	v10, v0, #0
	ushll.4s	v11, v0, #0
	ldr	q0, [sp, #128]                  ; 16-byte Folded Reload
	ushll2.4s	v12, v0, #0
	ushll.4s	v13, v0, #0
Lloh75:
	adrp	x10, _hidden_layer_1_weights_q@GOTPAGE
Lloh76:
	ldr	x10, [x10, _hidden_layer_1_weights_q@GOTPAGEOFF]
	ldr	q0, [sp, #144]                  ; 16-byte Folded Reload
	ushll2.4s	v14, v0, #0
	ushll.4s	v15, v0, #0
LBB6_3:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x10]
	sshll.4s	v7, v0, #0
	sshll2.4s	v2, v0, #0
	sshll.4s	v4, v1, #0
	sshll2.4s	v1, v1, #0
	ldp	q0, q3, [x10, #32]
	sshll2.4s	v5, v0, #0
	add	x11, x9, x8
	sshll.4s	v6, v0, #0
	sshll.4s	v16, v3, #0
	ldr	s0, [x11]
	sshll2.4s	v17, v3, #0
	ldr	q3, [sp, #1872]                 ; 16-byte Folded Reload
	mul.4s	v2, v2, v3
	ldr	q3, [sp, #1840]                 ; 16-byte Folded Reload
	mul.4s	v3, v1, v3
	ldr	q1, [sp, #1824]                 ; 16-byte Folded Reload
	mul.4s	v4, v4, v1
	ldr	q1, [sp, #1792]                 ; 16-byte Folded Reload
	mul.4s	v1, v6, v1
	ldr	q6, [sp, #1808]                 ; 16-byte Folded Reload
	mul.4s	v5, v5, v6
	ldr	q6, [sp, #1776]                 ; 16-byte Folded Reload
	mul.4s	v6, v17, v6
	ldp	q17, q18, [x10, #64]
	ldr	q19, [sp, #1856]                ; 16-byte Folded Reload
	mla.4s	v0, v7, v19
	sshll.4s	v7, v17, #0
	sshll2.4s	v17, v17, #0
	ldr	q19, [sp, #1728]                ; 16-byte Folded Reload
	mla.4s	v0, v7, v19
	sshll.4s	v7, v18, #0
	ldr	q19, [sp, #1744]                ; 16-byte Folded Reload
	mla.4s	v2, v17, v19
	ldr	q17, [sp, #1696]                ; 16-byte Folded Reload
	mla.4s	v4, v7, v17
	ldp	q17, q19, [x10, #96]
	sshll2.4s	v7, v18, #0
	ldr	q18, [sp, #1712]                ; 16-byte Folded Reload
	mla.4s	v3, v7, v18
	sshll2.4s	v18, v17, #0
	ldr	q7, [sp, #1760]                 ; 16-byte Folded Reload
	mul.4s	v7, v16, v7
	sshll.4s	v16, v17, #0
	ldr	q17, [sp, #1680]                ; 16-byte Folded Reload
	mla.4s	v5, v18, v17
	sshll.4s	v17, v19, #0
	sshll2.4s	v18, v19, #0
	ldr	q19, [sp, #1664]                ; 16-byte Folded Reload
	mla.4s	v1, v16, v19
	ldr	q16, [sp, #1632]                ; 16-byte Folded Reload
	mla.4s	v7, v17, v16
	ldr	q16, [sp, #1648]                ; 16-byte Folded Reload
	mla.4s	v6, v18, v16
	ldp	q16, q17, [x10, #128]
	sshll2.4s	v18, v16, #0
	sshll.4s	v16, v16, #0
	ldr	q19, [sp, #1616]                ; 16-byte Folded Reload
	mla.4s	v2, v18, v19
	sshll2.4s	v18, v17, #0
	ldr	q19, [sp, #1600]                ; 16-byte Folded Reload
	mla.4s	v0, v16, v19
	ldr	q16, [sp, #1584]                ; 16-byte Folded Reload
	mla.4s	v3, v18, v16
	ldp	q16, q18, [x10, #160]
	sshll.4s	v17, v17, #0
	ldr	q19, [sp, #1568]                ; 16-byte Folded Reload
	mla.4s	v4, v17, v19
	sshll.4s	v17, v16, #0
	sshll2.4s	v16, v16, #0
	ldr	q19, [sp, #1536]                ; 16-byte Folded Reload
	mla.4s	v1, v17, v19
	sshll2.4s	v17, v18, #0
	sshll.4s	v18, v18, #0
	ldr	q19, [sp, #1552]                ; 16-byte Folded Reload
	mla.4s	v5, v16, v19
	ldr	q16, [sp, #1520]                ; 16-byte Folded Reload
	mla.4s	v6, v17, v16
	ldr	q16, [sp, #1504]                ; 16-byte Folded Reload
	mla.4s	v7, v18, v16
	ldp	q16, q17, [x10, #192]
	sshll.4s	v18, v16, #0
	sshll2.4s	v16, v16, #0
	ldr	q19, [sp, #1472]                ; 16-byte Folded Reload
	mla.4s	v0, v18, v19
	sshll.4s	v18, v17, #0
	ldr	q19, [sp, #1488]                ; 16-byte Folded Reload
	mla.4s	v2, v16, v19
	ldr	q19, [sp, #1440]                ; 16-byte Folded Reload
	mla.4s	v4, v18, v19
	ldp	q16, q18, [x10, #224]
	sshll2.4s	v17, v17, #0
	ldr	q19, [sp, #1456]                ; 16-byte Folded Reload
	mla.4s	v3, v17, v19
	sshll2.4s	v17, v16, #0
	sshll.4s	v16, v16, #0
	ldr	q19, [sp, #1424]                ; 16-byte Folded Reload
	mla.4s	v5, v17, v19
	sshll.4s	v17, v18, #0
	sshll2.4s	v18, v18, #0
	ldr	q19, [sp, #1408]                ; 16-byte Folded Reload
	mla.4s	v1, v16, v19
	ldr	q16, [sp, #1376]                ; 16-byte Folded Reload
	mla.4s	v7, v17, v16
	ldr	q17, [sp, #1392]                ; 16-byte Folded Reload
	mla.4s	v6, v18, v17
	ldp	q16, q17, [x10, #256]
	sshll2.4s	v18, v16, #0
	sshll.4s	v16, v16, #0
	ldr	q19, [sp, #1360]                ; 16-byte Folded Reload
	mla.4s	v2, v18, v19
	sshll2.4s	v18, v17, #0
	ldr	q19, [sp, #1344]                ; 16-byte Folded Reload
	mla.4s	v0, v16, v19
	ldr	q16, [sp, #1328]                ; 16-byte Folded Reload
	mla.4s	v3, v18, v16
	ldp	q16, q18, [x10, #288]
	sshll.4s	v17, v17, #0
	ldr	q19, [sp, #1312]                ; 16-byte Folded Reload
	mla.4s	v4, v17, v19
	sshll.4s	v17, v16, #0
	sshll2.4s	v16, v16, #0
	ldr	q19, [sp, #1280]                ; 16-byte Folded Reload
	mla.4s	v1, v17, v19
	sshll2.4s	v17, v18, #0
	sshll.4s	v18, v18, #0
	ldr	q19, [sp, #1296]                ; 16-byte Folded Reload
	mla.4s	v5, v16, v19
	ldr	q16, [sp, #1264]                ; 16-byte Folded Reload
	mla.4s	v6, v17, v16
	ldr	q16, [sp, #1248]                ; 16-byte Folded Reload
	mla.4s	v7, v18, v16
	ldp	q16, q17, [x10, #320]
	sshll.4s	v18, v16, #0
	sshll2.4s	v16, v16, #0
	ldr	q19, [sp, #1216]                ; 16-byte Folded Reload
	mla.4s	v0, v18, v19
	sshll.4s	v18, v17, #0
	ldr	q19, [sp, #1232]                ; 16-byte Folded Reload
	mla.4s	v2, v16, v19
	ldr	q16, [sp, #1184]                ; 16-byte Folded Reload
	mla.4s	v4, v18, v16
	ldp	q16, q18, [x10, #352]
	sshll2.4s	v17, v17, #0
	ldr	q19, [sp, #1200]                ; 16-byte Folded Reload
	mla.4s	v3, v17, v19
	sshll2.4s	v17, v16, #0
	sshll.4s	v16, v16, #0
	ldr	q19, [sp, #1168]                ; 16-byte Folded Reload
	mla.4s	v5, v17, v19
	sshll.4s	v17, v18, #0
	sshll2.4s	v18, v18, #0
	ldr	q19, [sp, #1152]                ; 16-byte Folded Reload
	mla.4s	v1, v16, v19
	ldr	q16, [sp, #1120]                ; 16-byte Folded Reload
	mla.4s	v7, v17, v16
	ldr	q16, [sp, #1136]                ; 16-byte Folded Reload
	mla.4s	v6, v18, v16
	ldp	q16, q17, [x10, #384]
	sshll2.4s	v18, v16, #0
	sshll.4s	v16, v16, #0
	ldr	q19, [sp, #1104]                ; 16-byte Folded Reload
	mla.4s	v2, v18, v19
	sshll2.4s	v18, v17, #0
	ldr	q19, [sp, #1088]                ; 16-byte Folded Reload
	mla.4s	v0, v16, v19
	ldr	q19, [sp, #1072]                ; 16-byte Folded Reload
	mla.4s	v3, v18, v19
	ldp	q16, q18, [x10, #416]
	sshll.4s	v17, v17, #0
	ldr	q19, [sp, #1056]                ; 16-byte Folded Reload
	mla.4s	v4, v17, v19
	sshll.4s	v17, v16, #0
	sshll2.4s	v16, v16, #0
	ldr	q19, [sp, #1024]                ; 16-byte Folded Reload
	mla.4s	v1, v17, v19
	sshll2.4s	v17, v18, #0
	sshll.4s	v18, v18, #0
	ldr	q19, [sp, #1040]                ; 16-byte Folded Reload
	mla.4s	v5, v16, v19
	ldr	q16, [sp, #1008]                ; 16-byte Folded Reload
	mla.4s	v6, v17, v16
	ldr	q17, [sp, #992]                 ; 16-byte Folded Reload
	mla.4s	v7, v18, v17
	ldp	q16, q17, [x10, #448]
	sshll.4s	v18, v16, #0
	sshll2.4s	v16, v16, #0
	ldr	q19, [sp, #960]                 ; 16-byte Folded Reload
	mla.4s	v0, v18, v19
	sshll.4s	v18, v17, #0
	ldr	q19, [sp, #976]                 ; 16-byte Folded Reload
	mla.4s	v2, v16, v19
	ldp	q16, q19, [sp, #928]            ; 32-byte Folded Reload
	mla.4s	v4, v18, v16
	ldp	q16, q18, [x10, #480]
	sshll2.4s	v17, v17, #0
	mla.4s	v3, v17, v19
	sshll2.4s	v17, v16, #0
	sshll.4s	v16, v16, #0
	ldr	q19, [sp, #912]                 ; 16-byte Folded Reload
	mla.4s	v5, v17, v19
	sshll.4s	v17, v18, #0
	sshll2.4s	v18, v18, #0
	ldr	q19, [sp, #896]                 ; 16-byte Folded Reload
	mla.4s	v1, v16, v19
	ldp	q19, q16, [sp, #864]            ; 32-byte Folded Reload
	mla.4s	v7, v17, v19
	mla.4s	v6, v18, v16
	ldp	q16, q17, [x10, #512]
	sshll2.4s	v18, v16, #0
	sshll.4s	v16, v16, #0
	ldr	q19, [sp, #848]                 ; 16-byte Folded Reload
	mla.4s	v2, v18, v19
	sshll2.4s	v18, v17, #0
	ldr	q19, [sp, #832]                 ; 16-byte Folded Reload
	mla.4s	v0, v16, v19
	ldp	q19, q16, [sp, #800]            ; 32-byte Folded Reload
	mla.4s	v3, v18, v16
	ldp	q16, q18, [x10, #544]
	sshll.4s	v17, v17, #0
	mla.4s	v4, v17, v19
	sshll.4s	v17, v16, #0
	sshll2.4s	v16, v16, #0
	ldr	q19, [sp, #768]                 ; 16-byte Folded Reload
	mla.4s	v1, v17, v19
	sshll2.4s	v17, v18, #0
	sshll.4s	v18, v18, #0
	ldr	q19, [sp, #784]                 ; 16-byte Folded Reload
	mla.4s	v5, v16, v19
	ldp	q16, q19, [sp, #736]            ; 32-byte Folded Reload
	mla.4s	v6, v17, v19
	mla.4s	v7, v18, v16
	ldp	q16, q17, [x10, #576]
	sshll.4s	v18, v16, #0
	sshll2.4s	v16, v16, #0
	ldr	q19, [sp, #704]                 ; 16-byte Folded Reload
	mla.4s	v0, v18, v19
	sshll.4s	v18, v17, #0
	ldr	q19, [sp, #720]                 ; 16-byte Folded Reload
	mla.4s	v2, v16, v19
	ldr	q19, [sp, #672]                 ; 16-byte Folded Reload
	mla.4s	v4, v18, v19
	ldp	q16, q18, [x10, #608]
	sshll2.4s	v17, v17, #0
	ldr	q19, [sp, #688]                 ; 16-byte Folded Reload
	mla.4s	v3, v17, v19
	sshll2.4s	v17, v16, #0
	sshll.4s	v16, v16, #0
	ldr	q19, [sp, #656]                 ; 16-byte Folded Reload
	mla.4s	v5, v17, v19
	sshll.4s	v17, v18, #0
	sshll2.4s	v18, v18, #0
	ldr	q19, [sp, #640]                 ; 16-byte Folded Reload
	mla.4s	v1, v16, v19
	ldp	q19, q16, [sp, #592]            ; 32-byte Folded Reload
	mla.4s	v7, v17, v16
	ldr	q17, [sp, #624]                 ; 16-byte Folded Reload
	mla.4s	v6, v18, v17
	ldp	q16, q17, [x10, #640]
	sshll2.4s	v18, v16, #0
	sshll.4s	v16, v16, #0
	mla.4s	v2, v18, v19
	sshll2.4s	v18, v17, #0
	ldr	q19, [sp, #576]                 ; 16-byte Folded Reload
	mla.4s	v0, v16, v19
	ldp	q19, q16, [sp, #544]            ; 32-byte Folded Reload
	mla.4s	v3, v18, v16
	ldp	q16, q18, [x10, #672]
	sshll.4s	v17, v17, #0
	mla.4s	v4, v17, v19
	sshll.4s	v17, v16, #0
	sshll2.4s	v16, v16, #0
	ldr	q19, [sp, #512]                 ; 16-byte Folded Reload
	mla.4s	v1, v17, v19
	sshll2.4s	v17, v18, #0
	sshll.4s	v18, v18, #0
	ldr	q19, [sp, #528]                 ; 16-byte Folded Reload
	mla.4s	v5, v16, v19
	ldp	q16, q19, [sp, #480]            ; 32-byte Folded Reload
	mla.4s	v6, v17, v19
	mla.4s	v7, v18, v16
	ldp	q16, q17, [x10, #704]
	sshll.4s	v18, v16, #0
	sshll2.4s	v16, v16, #0
	ldr	q19, [sp, #448]                 ; 16-byte Folded Reload
	mla.4s	v0, v18, v19
	sshll.4s	v18, v17, #0
	ldr	q19, [sp, #464]                 ; 16-byte Folded Reload
	mla.4s	v2, v16, v19
	ldp	q16, q19, [sp, #416]            ; 32-byte Folded Reload
	mla.4s	v4, v18, v16
	ldp	q16, q18, [x10, #736]
	sshll2.4s	v17, v17, #0
	mla.4s	v3, v17, v19
	sshll2.4s	v17, v16, #0
	sshll.4s	v16, v16, #0
	ldr	q19, [sp, #400]                 ; 16-byte Folded Reload
	mla.4s	v5, v17, v19
	sshll.4s	v17, v18, #0
	sshll2.4s	v18, v18, #0
	ldr	q19, [sp, #384]                 ; 16-byte Folded Reload
	mla.4s	v1, v16, v19
	ldp	q19, q16, [sp, #352]            ; 32-byte Folded Reload
	mla.4s	v7, v17, v19
	mla.4s	v6, v18, v16
	ldp	q16, q17, [x10, #768]
	sshll2.4s	v18, v16, #0
	sshll.4s	v16, v16, #0
	ldr	q19, [sp, #336]                 ; 16-byte Folded Reload
	mla.4s	v2, v18, v19
	sshll2.4s	v18, v17, #0
	ldr	q19, [sp, #320]                 ; 16-byte Folded Reload
	mla.4s	v0, v16, v19
	ldr	q19, [sp, #304]                 ; 16-byte Folded Reload
	mla.4s	v3, v18, v19
	ldp	q16, q18, [x10, #800]
	sshll.4s	v17, v17, #0
	ldr	q19, [sp, #288]                 ; 16-byte Folded Reload
	mla.4s	v4, v17, v19
	sshll.4s	v17, v16, #0
	sshll2.4s	v16, v16, #0
	ldr	q19, [sp, #256]                 ; 16-byte Folded Reload
	mla.4s	v1, v17, v19
	sshll2.4s	v17, v18, #0
	sshll.4s	v18, v18, #0
	ldr	q19, [sp, #272]                 ; 16-byte Folded Reload
	mla.4s	v5, v16, v19
	ldr	q16, [sp, #240]                 ; 16-byte Folded Reload
	mla.4s	v6, v17, v16
	ldr	q17, [sp, #224]                 ; 16-byte Folded Reload
	mla.4s	v7, v18, v17
	ldp	q16, q17, [x10, #832]
	sshll.4s	v18, v16, #0
	sshll2.4s	v16, v16, #0
	ldr	q19, [sp, #192]                 ; 16-byte Folded Reload
	mla.4s	v0, v18, v19
	sshll.4s	v18, v17, #0
	ldr	q19, [sp, #208]                 ; 16-byte Folded Reload
	mla.4s	v2, v16, v19
	ldp	q16, q19, [sp, #160]            ; 32-byte Folded Reload
	mla.4s	v4, v18, v16
	ldp	q16, q18, [x10, #864]
	sshll2.4s	v17, v17, #0
	mla.4s	v3, v17, v19
	sshll2.4s	v17, v16, #0
	sshll.4s	v16, v16, #0
	mla.4s	v5, v17, v20
	sshll.4s	v17, v18, #0
	sshll2.4s	v18, v18, #0
	mla.4s	v1, v16, v21
	mla.4s	v7, v17, v23
	mla.4s	v6, v18, v22
	ldp	q16, q17, [x10, #896]
	sshll2.4s	v18, v16, #0
	sshll.4s	v16, v16, #0
	mla.4s	v2, v18, v24
	sshll2.4s	v18, v17, #0
	mla.4s	v0, v16, v25
	mla.4s	v3, v18, v26
	ldp	q16, q18, [x10, #928]
	sshll.4s	v17, v17, #0
	mla.4s	v4, v17, v27
	sshll.4s	v17, v16, #0
	sshll2.4s	v16, v16, #0
	mla.4s	v1, v17, v29
	sshll2.4s	v17, v18, #0
	sshll.4s	v18, v18, #0
	mla.4s	v5, v16, v28
	mla.4s	v6, v17, v30
	mla.4s	v7, v18, v31
	ldp	q16, q17, [x10, #960]
	sshll.4s	v18, v16, #0
	sshll2.4s	v16, v16, #0
	mla.4s	v0, v18, v9
	sshll.4s	v18, v17, #0
	mla.4s	v2, v16, v8
	mla.4s	v4, v18, v11
	ldp	q16, q18, [x10, #992]
	sshll2.4s	v17, v17, #0
	mla.4s	v3, v17, v10
	sshll2.4s	v17, v16, #0
	mla.4s	v5, v17, v12
	sshll.4s	v16, v16, #0
	mla.4s	v1, v16, v13
	sshll.4s	v16, v18, #0
	mla.4s	v7, v16, v15
	sshll2.4s	v16, v18, #0
	mla.4s	v6, v16, v14
	add.4s	v2, v3, v2
	add.4s	v0, v4, v0
	add.4s	v3, v6, v5
	add.4s	v2, v3, v2
	add.4s	v1, v7, v1
	add.4s	v0, v1, v0
	add.4s	v0, v0, v2
	addv.4s	s0, v0
	str	s0, [x9, x8]
	add	x8, x8, #4
	add	x10, x10, #1024
	cmp	x8, #128
	b.ne	LBB6_3
; %bb.4:
	mov	x8, #0                          ; =0x0
	ldp	q5, q3, [x9]
	cmle.4s	v0, v5, #0
	xtn.4h	v0, v0
	cmle.4s	v1, v3, #0
	xtn.4h	v1, v1
	ldp	q16, q6, [x9, #32]
	cmle.4s	v2, v16, #0
	xtn.4h	v2, v2
	cmle.4s	v4, v6, #0
	ldp	q18, q17, [x9, #64]
	xtn.4h	v4, v4
	cmle.4s	v7, v18, #0
	xtn.4h	v7, v7
	cmle.4s	v19, v17, #0
	xtn.4h	v19, v19
	ldp	q21, q20, [x9, #96]
	cmle.4s	v22, v21, #0
	xtn.4h	v22, v22
	cmle.4s	v23, v20, #0
	xtn.4h	v23, v23
	movi.4s	v24, #31, msl #8
	add.4s	v3, v3, v24
	add.4s	v5, v5, v24
	add.4s	v6, v6, v24
	add.4s	v16, v16, v24
	add.4s	v17, v17, v24
	add.4s	v18, v18, v24
	add.4s	v20, v20, v24
Lloh77:
	adrp	x9, _hidden_layer_2_bias_q@GOTPAGE
Lloh78:
	ldr	x9, [x9, _hidden_layer_2_bias_q@GOTPAGEOFF]
	add.4s	v21, v21, v24
	shrn.4h	v24, v5, #14
	shrn.4h	v25, v3, #14
	ldp	q3, q5, [x9, #64]
	shrn.4h	v16, v16, #14
	shrn.4h	v26, v6, #14
	shrn.4h	v18, v18, #14
	str	q3, [sp, #1968]
	str	q5, [sp, #1984]
	shrn.4h	v6, v17, #14
	shrn.4h	v5, v21, #14
	ldp	q3, q17, [x9, #96]
	shrn.4h	v20, v20, #14
	str	q3, [sp, #2000]
	str	q17, [sp, #2016]
	ldp	q3, q17, [x9]
	str	q3, [sp, #1904]
	str	q17, [sp, #1920]
	bic.8b	v3, v20, v23
	ldp	q17, q20, [x9, #32]
	str	q17, [sp, #1936]
	str	q20, [sp, #1952]
	sshll.4s	v3, v3, #0
	bic.8b	v5, v5, v22
	sshll.4s	v5, v5, #0
	bic.8b	v6, v6, v19
	sshll.4s	v6, v6, #0
	bic.8b	v7, v18, v7
	sshll.4s	v7, v7, #0
	bic.8b	v4, v26, v4
	sshll.4s	v4, v4, #0
	bic.8b	v2, v16, v2
	sshll.4s	v2, v2, #0
	bic.8b	v1, v25, v1
	sshll.4s	v1, v1, #0
	add	x9, sp, #1904
	bic.8b	v0, v24, v0
Lloh79:
	adrp	x10, _hidden_layer_2_weights_q@GOTPAGE
Lloh80:
	ldr	x10, [x10, _hidden_layer_2_weights_q@GOTPAGEOFF]
	sshll.4s	v0, v0, #0
LBB6_5:                                 ; =>This Inner Loop Header: Depth=1
	ldr	w11, [x9, x8, lsl #2]
	add	x12, x10, x8, lsl #6
	ldp	q17, q16, [x12, #32]
	ldp	q19, q18, [x12]
	sshll.4s	v20, v19, #0
	sshll.4s	v21, v17, #0
	sshll.4s	v22, v16, #0
	sshll.4s	v23, v18, #0
	sshll2.4s	v17, v17, #0
	sshll2.4s	v19, v19, #0
	sshll2.4s	v16, v16, #0
	sshll2.4s	v18, v18, #0
	mul.4s	v16, v16, v3
	mul.4s	v17, v17, v6
	mul.4s	v22, v22, v5
	mul.4s	v21, v21, v7
	mla.4s	v21, v20, v0
	mla.4s	v22, v23, v2
	add.4s	v20, v21, v22
	mla.4s	v17, v19, v1
	mla.4s	v16, v18, v4
	add.4s	v16, v17, v16
	add.4s	v16, v20, v16
	addv.4s	s16, v16
	fmov	w12, s16
	add	w11, w12, w11
	str	w11, [x9, x8, lsl #2]
	add	x8, x8, #1
	cmp	x8, #32
	b.ne	LBB6_5
; %bb.6:
	ldr	q0, [sp, #1920]
	ldr	q1, [sp, #1904]
	ldr	q2, [sp, #1952]
	ldr	q3, [sp, #1936]
	ldr	q4, [sp, #1984]
	ldr	q5, [sp, #1968]
	ldr	q6, [sp, #2016]
	ldr	q7, [sp, #2000]
	cmle.4s	v16, v1, #0
	xtn.4h	v16, v16
	cmle.4s	v17, v0, #0
	xtn.4h	v17, v17
	cmle.4s	v18, v3, #0
	xtn.4h	v18, v18
	cmle.4s	v19, v2, #0
	xtn.4h	v19, v19
	cmle.4s	v20, v5, #0
	xtn.4h	v20, v20
	cmle.4s	v21, v4, #0
	xtn.4h	v21, v21
	cmle.4s	v22, v7, #0
	xtn.4h	v22, v22
	cmle.4s	v23, v6, #0
	xtn.4h	v23, v23
	movi.4s	v24, #31, msl #8
	add.4s	v0, v0, v24
	add.4s	v1, v1, v24
	add.4s	v2, v2, v24
	add.4s	v3, v3, v24
	add.4s	v4, v4, v24
	add.4s	v5, v5, v24
	add.4s	v6, v6, v24
	add.4s	v7, v7, v24
	shrn.4h	v1, v1, #14
	shrn.4h	v0, v0, #14
	shrn.4h	v3, v3, #14
	shrn.4h	v2, v2, #14
	shrn.4h	v5, v5, #14
	shrn.4h	v4, v4, #14
	shrn.4h	v7, v7, #14
	shrn.4h	v6, v6, #14
Lloh81:
	adrp	x8, _output_layer_bias_q@GOTPAGE
Lloh82:
	ldr	x8, [x8, _output_layer_bias_q@GOTPAGEOFF]
Lloh83:
	ldr	w8, [x8]
	bic.8b	v2, v2, v19
	sshll.4s	v2, v2, #0
	bic.8b	v6, v6, v23
	sshll.4s	v6, v6, #0
	bic.8b	v0, v0, v17
	sshll.4s	v0, v0, #0
	bic.8b	v4, v4, v21
	sshll.4s	v4, v4, #0
	bic.8b	v3, v3, v18
	sshll.4s	v3, v3, #0
	bic.8b	v7, v7, v22
	sshll.4s	v7, v7, #0
	bic.8b	v1, v1, v16
	sshll.4s	v1, v1, #0
	bic.8b	v5, v5, v20
	sshll.4s	v5, v5, #0
Lloh84:
	adrp	x9, _output_layer_weights_q@GOTPAGE
Lloh85:
	ldr	x9, [x9, _output_layer_weights_q@GOTPAGEOFF]
	ldp	q17, q16, [x9, #32]
	ldp	q19, q18, [x9, #96]
	ldp	q21, q20, [x9]
	ldp	q23, q22, [x9, #64]
	mul.4s	v5, v23, v5
	mul.4s	v7, v19, v7
	mul.4s	v4, v22, v4
	mul.4s	v6, v18, v6
	mla.4s	v6, v16, v2
	mla.4s	v4, v20, v0
	add.4s	v0, v4, v6
	mla.4s	v7, v17, v3
	mla.4s	v5, v21, v1
	add.4s	v1, v5, v7
	add.4s	v0, v1, v0
	addv.4s	s0, v0
	fmov	w9, s0
	mov	w10, #8191                      ; =0x1fff
	add	w8, w9, w8
	add	w9, w8, w10
	mov	w11, #24574                     ; =0x5ffe
	add	w8, w8, w11
	cmp	w9, #0
	csel	w8, w8, w9, lt
	asr	w8, w8, #14
	mov	w9, #4000                       ; =0xfa0
	mul	w8, w8, w9
	adds	w9, w8, w10
	add	w8, w8, w11
	csel	w8, w8, w9, lt
	asr	w8, w8, #14
	str	w8, [sp, #1900]
	mov	x0, x19
	bl	_MaterialBalance
	cmp	w20, #0
	cneg	w8, w0, ne
	ldr	w1, [sp, #1900]
	subs	w8, w8, w1
	cneg	w8, w8, mi
Lloh86:
	adrp	x9, _MaxPos@GOTPAGE
Lloh87:
	ldr	x9, [x9, _MaxPos@GOTPAGEOFF]
	ldr	w10, [x9]
	cmp	w8, w10
	b.le	LBB6_8
; %bb.7:
	str	w8, [x9]
LBB6_8:
	ldr	x0, [x19, #1144]
	bl	_StoreST
LBB6_9:
	ldr	w0, [sp, #1900]
	ldur	x8, [x29, #-104]
Lloh88:
	adrp	x9, ___stack_chk_guard@GOTPAGE
Lloh89:
	ldr	x9, [x9, ___stack_chk_guard@GOTPAGEOFF]
Lloh90:
	ldr	x9, [x9]
	cmp	x9, x8
	b.ne	LBB6_11
; %bb.10:
	add	sp, sp, #2176
	ldp	x29, x30, [sp, #96]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #80]             ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #64]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp, #48]               ; 16-byte Folded Reload
	ldp	d11, d10, [sp, #32]             ; 16-byte Folded Reload
	ldp	d13, d12, [sp, #16]             ; 16-byte Folded Reload
	ldp	d15, d14, [sp], #112            ; 16-byte Folded Reload
	ret
LBB6_11:
	bl	___stack_chk_fail
	.loh AdrpLdrGot	Lloh69, Lloh70
	.loh AdrpLdrGotLdr	Lloh66, Lloh67, Lloh68
	.loh AdrpLdrGot	Lloh71, Lloh72
	.loh AdrpLdrGot	Lloh75, Lloh76
	.loh AdrpLdrGot	Lloh73, Lloh74
	.loh AdrpLdrGot	Lloh79, Lloh80
	.loh AdrpLdrGot	Lloh77, Lloh78
	.loh AdrpLdrGot	Lloh86, Lloh87
	.loh AdrpLdrGot	Lloh84, Lloh85
	.loh AdrpLdrGotLdr	Lloh81, Lloh82, Lloh83
	.loh AdrpLdrGotLdr	Lloh88, Lloh89, Lloh90
	.cfi_endproc
                                        ; -- End function
	.globl	_InitAccumulator                ; -- Begin function InitAccumulator
	.p2align	2
_InitAccumulator:                       ; @InitAccumulator
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
	mov	x19, x0
	add	x20, x0, #1196
Lloh91:
	adrp	x22, _input_layer_bias_q@GOTPAGE
Lloh92:
	ldr	x22, [x22, _input_layer_bias_q@GOTPAGEOFF]
	mov	x0, x20
	mov	x1, x22
	mov	w2, #512                        ; =0x200
	bl	_memcpy
	add	x21, x19, #1708
	mov	x0, x21
	mov	x1, x22
	mov	w2, #512                        ; =0x200
	bl	_memcpy
	mov	w8, #0                          ; =0x0
	add	x9, x19, #1024
	add	x10, x19, #1080
	add	x11, x19, #2220
	mov	w12, #20480                     ; =0x5000
	mov	w13, #1                         ; =0x1
Lloh93:
	adrp	x14, _input_layer_weights_q@GOTPAGE
Lloh94:
	ldr	x14, [x14, _input_layer_weights_q@GOTPAGEOFF]
	b	LBB7_2
LBB7_1:                                 ;   in Loop: Header=BB7_2 Depth=1
	add	x13, x13, #1
	add	w8, w8, #1, lsl #12             ; =4096
	add	w12, w12, #1, lsl #12           ; =4096
	cmp	x13, #6
	b.eq	LBB7_24
LBB7_2:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB7_6 Depth 2
                                        ;       Child Loop BB7_9 Depth 3
                                        ;       Child Loop BB7_12 Depth 3
                                        ;     Child Loop BB7_17 Depth 2
                                        ;       Child Loop BB7_20 Depth 3
                                        ;       Child Loop BB7_23 Depth 3
	ldr	x15, [x9, x13, lsl #3]
	cbz	x15, LBB7_13
; %bb.3:                                ;   in Loop: Header=BB7_2 Depth=1
	lsl	w17, w13, #12
	sub	w16, w17, #1, lsl #12           ; =4096
	add	w17, w17, #4, lsl #12           ; =16384
	b	LBB7_6
LBB7_4:                                 ;   in Loop: Header=BB7_6 Depth=2
	ldp	q0, q1, [x0]
	ldp	q2, q3, [x0, #32]
	ldp	q4, q5, [x20, #512]
	ldp	q6, q7, [x20, #544]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #512]
	stp	q2, q3, [x20, #544]
	ldp	q0, q1, [x0, #64]
	ldp	q2, q3, [x0, #96]
	ldp	q4, q5, [x20, #576]
	ldp	q6, q7, [x20, #608]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #576]
	stp	q2, q3, [x20, #608]
	ldp	q0, q1, [x0, #128]
	ldp	q2, q3, [x0, #160]
	ldp	q4, q5, [x20, #640]
	ldp	q6, q7, [x20, #672]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #640]
	stp	q2, q3, [x20, #672]
	ldp	q0, q1, [x0, #192]
	ldp	q2, q3, [x0, #224]
	ldp	q4, q5, [x20, #704]
	ldp	q6, q7, [x20, #736]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #704]
	stp	q2, q3, [x20, #736]
	ldp	q0, q1, [x0, #256]
	ldp	q2, q3, [x0, #288]
	ldp	q4, q5, [x20, #768]
	ldp	q6, q7, [x20, #800]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #768]
	stp	q2, q3, [x20, #800]
	ldp	q0, q1, [x0, #320]
	ldp	q2, q3, [x0, #352]
	ldp	q4, q5, [x20, #832]
	ldp	q6, q7, [x20, #864]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #832]
	stp	q2, q3, [x20, #864]
	ldp	q0, q1, [x0, #384]
	ldp	q2, q3, [x0, #416]
	ldp	q4, q5, [x20, #896]
	ldp	q6, q7, [x20, #928]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #896]
	stp	q2, q3, [x20, #928]
	ldp	q0, q1, [x0, #448]
	ldp	q2, q3, [x0, #480]
	ldp	q4, q5, [x20, #960]
	ldp	q6, q7, [x20, #992]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #960]
	stp	q2, q3, [x20, #992]
LBB7_5:                                 ;   in Loop: Header=BB7_6 Depth=2
	sub	x0, x15, #1
	ands	x15, x0, x15
	b.eq	LBB7_13
LBB7_6:                                 ;   Parent Loop BB7_2 Depth=1
                                        ; =>  This Loop Header: Depth=2
                                        ;       Child Loop BB7_9 Depth 3
                                        ;       Child Loop BB7_12 Depth 3
	rbit	x0, x15
	clz	x0, x0
	ldrsb	x2, [x19, #2293]
	ldrsb	x1, [x19, #2294]
	lsl	x3, x2, #6
	orr	w2, w16, w0
	add	x2, x3, w2, sxtw
	add	x2, x14, x2, lsl #9
	add	x4, x2, #512
	cmp	x20, x4
	ccmp	x2, x21, #2, lo
	b.lo	LBB7_8
; %bb.7:                                ;   in Loop: Header=BB7_6 Depth=2
	ldp	q0, q1, [x2]
	ldp	q2, q3, [x2, #32]
	ldp	q4, q5, [x20]
	ldp	q6, q7, [x20, #32]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20]
	stp	q2, q3, [x20, #32]
	ldp	q0, q1, [x2, #64]
	ldp	q2, q3, [x2, #96]
	ldp	q4, q5, [x20, #64]
	ldp	q6, q7, [x20, #96]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #64]
	stp	q2, q3, [x20, #96]
	ldp	q0, q1, [x2, #128]
	ldp	q2, q3, [x2, #160]
	ldp	q4, q5, [x20, #128]
	ldp	q6, q7, [x20, #160]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #128]
	stp	q2, q3, [x20, #160]
	ldp	q0, q1, [x2, #192]
	ldp	q2, q3, [x2, #224]
	ldp	q4, q5, [x20, #192]
	ldp	q6, q7, [x20, #224]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #192]
	stp	q2, q3, [x20, #224]
	ldp	q0, q1, [x2, #256]
	ldp	q2, q3, [x2, #288]
	ldp	q4, q5, [x20, #256]
	ldp	q6, q7, [x20, #288]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #256]
	stp	q2, q3, [x20, #288]
	ldp	q0, q1, [x2, #320]
	ldp	q2, q3, [x2, #352]
	ldp	q4, q5, [x20, #320]
	ldp	q6, q7, [x20, #352]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #320]
	stp	q2, q3, [x20, #352]
	ldp	q0, q1, [x2, #384]
	ldp	q2, q3, [x2, #416]
	ldp	q4, q5, [x20, #384]
	ldp	q6, q7, [x20, #416]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #384]
	stp	q2, q3, [x20, #416]
	ldp	q0, q1, [x2, #448]
	ldp	q2, q3, [x2, #480]
	ldp	q4, q5, [x20, #448]
	ldp	q6, q7, [x20, #480]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #448]
	stp	q2, q3, [x20, #480]
	b	LBB7_10
LBB7_8:                                 ;   in Loop: Header=BB7_6 Depth=2
	mov	x2, #0                          ; =0x0
	add	w3, w8, w3
	add	w3, w3, w0
	sxtw	x3, w3
	add	x3, x14, x3, lsl #9
LBB7_9:                                 ;   Parent Loop BB7_2 Depth=1
                                        ;     Parent Loop BB7_6 Depth=2
                                        ; =>    This Inner Loop Header: Depth=3
	ldrh	w4, [x3, x2]
	ldrh	w5, [x20, x2]
	add	w4, w5, w4
	strh	w4, [x20, x2]
	add	x2, x2, #2
	cmp	x2, #512
	b.ne	LBB7_9
LBB7_10:                                ;   in Loop: Header=BB7_6 Depth=2
	eor	x1, x1, #0x38
	orr	w0, w17, w0
	sxtw	x0, w0
	eor	x0, x0, #0x38
	add	x0, x0, x1, lsl #6
	add	x0, x14, x0, lsl #9
	add	x1, x0, #512
	cmp	x21, x1
	ccmp	x0, x11, #2, lo
	b.hs	LBB7_4
; %bb.11:                               ;   in Loop: Header=BB7_6 Depth=2
	mov	x1, #0                          ; =0x0
LBB7_12:                                ;   Parent Loop BB7_2 Depth=1
                                        ;     Parent Loop BB7_6 Depth=2
                                        ; =>    This Inner Loop Header: Depth=3
	ldrh	w2, [x0, x1]
	ldrh	w3, [x21, x1]
	add	w2, w3, w2
	strh	w2, [x21, x1]
	add	x1, x1, #2
	cmp	x1, #512
	b.ne	LBB7_12
	b	LBB7_5
LBB7_13:                                ;   in Loop: Header=BB7_2 Depth=1
	ldr	x15, [x10, x13, lsl #3]
	cbz	x15, LBB7_1
; %bb.14:                               ;   in Loop: Header=BB7_2 Depth=1
	lsl	w17, w13, #12
	add	w16, w17, #4, lsl #12           ; =16384
	sub	w17, w17, #1, lsl #12           ; =4096
	b	LBB7_17
LBB7_15:                                ;   in Loop: Header=BB7_17 Depth=2
	ldp	q0, q1, [x0]
	ldp	q2, q3, [x0, #32]
	ldp	q4, q5, [x20, #512]
	ldp	q6, q7, [x20, #544]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #512]
	stp	q2, q3, [x20, #544]
	ldp	q0, q1, [x0, #64]
	ldp	q2, q3, [x0, #96]
	ldp	q4, q5, [x20, #576]
	ldp	q6, q7, [x20, #608]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #576]
	stp	q2, q3, [x20, #608]
	ldp	q0, q1, [x0, #128]
	ldp	q2, q3, [x0, #160]
	ldp	q4, q5, [x20, #640]
	ldp	q6, q7, [x20, #672]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #640]
	stp	q2, q3, [x20, #672]
	ldp	q0, q1, [x0, #192]
	ldp	q2, q3, [x0, #224]
	ldp	q4, q5, [x20, #704]
	ldp	q6, q7, [x20, #736]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #704]
	stp	q2, q3, [x20, #736]
	ldp	q0, q1, [x0, #256]
	ldp	q2, q3, [x0, #288]
	ldp	q4, q5, [x20, #768]
	ldp	q6, q7, [x20, #800]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #768]
	stp	q2, q3, [x20, #800]
	ldp	q0, q1, [x0, #320]
	ldp	q2, q3, [x0, #352]
	ldp	q4, q5, [x20, #832]
	ldp	q6, q7, [x20, #864]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #832]
	stp	q2, q3, [x20, #864]
	ldp	q0, q1, [x0, #384]
	ldp	q2, q3, [x0, #416]
	ldp	q4, q5, [x20, #896]
	ldp	q6, q7, [x20, #928]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #896]
	stp	q2, q3, [x20, #928]
	ldp	q0, q1, [x0, #448]
	ldp	q2, q3, [x0, #480]
	ldp	q4, q5, [x20, #960]
	ldp	q6, q7, [x20, #992]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #960]
	stp	q2, q3, [x20, #992]
LBB7_16:                                ;   in Loop: Header=BB7_17 Depth=2
	sub	x0, x15, #1
	ands	x15, x0, x15
	b.eq	LBB7_1
LBB7_17:                                ;   Parent Loop BB7_2 Depth=1
                                        ; =>  This Loop Header: Depth=2
                                        ;       Child Loop BB7_20 Depth 3
                                        ;       Child Loop BB7_23 Depth 3
	rbit	x0, x15
	clz	x0, x0
	ldrsb	x2, [x19, #2293]
	ldrsb	x1, [x19, #2294]
	lsl	x3, x2, #6
	orr	w2, w16, w0
	add	x2, x3, w2, sxtw
	add	x2, x14, x2, lsl #9
	add	x4, x2, #512
	cmp	x20, x4
	ccmp	x2, x21, #2, lo
	b.lo	LBB7_19
; %bb.18:                               ;   in Loop: Header=BB7_17 Depth=2
	ldp	q0, q1, [x2]
	ldp	q2, q3, [x2, #32]
	ldp	q4, q5, [x20]
	ldp	q6, q7, [x20, #32]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20]
	stp	q2, q3, [x20, #32]
	ldp	q0, q1, [x2, #64]
	ldp	q2, q3, [x2, #96]
	ldp	q4, q5, [x20, #64]
	ldp	q6, q7, [x20, #96]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #64]
	stp	q2, q3, [x20, #96]
	ldp	q0, q1, [x2, #128]
	ldp	q2, q3, [x2, #160]
	ldp	q4, q5, [x20, #128]
	ldp	q6, q7, [x20, #160]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #128]
	stp	q2, q3, [x20, #160]
	ldp	q0, q1, [x2, #192]
	ldp	q2, q3, [x2, #224]
	ldp	q4, q5, [x20, #192]
	ldp	q6, q7, [x20, #224]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #192]
	stp	q2, q3, [x20, #224]
	ldp	q0, q1, [x2, #256]
	ldp	q2, q3, [x2, #288]
	ldp	q4, q5, [x20, #256]
	ldp	q6, q7, [x20, #288]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #256]
	stp	q2, q3, [x20, #288]
	ldp	q0, q1, [x2, #320]
	ldp	q2, q3, [x2, #352]
	ldp	q4, q5, [x20, #320]
	ldp	q6, q7, [x20, #352]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #320]
	stp	q2, q3, [x20, #352]
	ldp	q0, q1, [x2, #384]
	ldp	q2, q3, [x2, #416]
	ldp	q4, q5, [x20, #384]
	ldp	q6, q7, [x20, #416]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #384]
	stp	q2, q3, [x20, #416]
	ldp	q0, q1, [x2, #448]
	ldp	q2, q3, [x2, #480]
	ldp	q4, q5, [x20, #448]
	ldp	q6, q7, [x20, #480]
	add.8h	v0, v4, v0
	add.8h	v1, v5, v1
	add.8h	v2, v6, v2
	add.8h	v3, v7, v3
	stp	q0, q1, [x20, #448]
	stp	q2, q3, [x20, #480]
	b	LBB7_21
LBB7_19:                                ;   in Loop: Header=BB7_17 Depth=2
	mov	x2, #0                          ; =0x0
	add	w3, w12, w3
	add	w3, w3, w0
	add	x3, x14, x3, lsl #9
LBB7_20:                                ;   Parent Loop BB7_2 Depth=1
                                        ;     Parent Loop BB7_17 Depth=2
                                        ; =>    This Inner Loop Header: Depth=3
	ldrh	w4, [x3, x2]
	ldrh	w5, [x20, x2]
	add	w4, w5, w4
	strh	w4, [x20, x2]
	add	x2, x2, #2
	cmp	x2, #512
	b.ne	LBB7_20
LBB7_21:                                ;   in Loop: Header=BB7_17 Depth=2
	eor	x1, x1, #0x38
	orr	w0, w17, w0
	sxtw	x0, w0
	eor	x0, x0, #0x38
	add	x0, x0, x1, lsl #6
	add	x0, x14, x0, lsl #9
	add	x1, x0, #512
	cmp	x21, x1
	ccmp	x0, x11, #2, lo
	b.hs	LBB7_15
; %bb.22:                               ;   in Loop: Header=BB7_17 Depth=2
	mov	x1, #0                          ; =0x0
LBB7_23:                                ;   Parent Loop BB7_2 Depth=1
                                        ;     Parent Loop BB7_17 Depth=2
                                        ; =>    This Inner Loop Header: Depth=3
	ldrh	w2, [x0, x1]
	ldrh	w3, [x21, x1]
	add	w2, w3, w2
	strh	w2, [x21, x1]
	add	x1, x1, #2
	cmp	x1, #512
	b.ne	LBB7_23
	b	LBB7_16
LBB7_24:
	ldp	x29, x30, [sp, #32]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #16]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp], #48             ; 16-byte Folded Reload
	ret
	.loh AdrpLdrGot	Lloh93, Lloh94
	.loh AdrpLdrGot	Lloh91, Lloh92
	.cfi_endproc
                                        ; -- End function
	.globl	_ValidateWeights                ; -- Begin function ValidateWeights
	.p2align	2
_ValidateWeights:                       ; @ValidateWeights
	.cfi_startproc
; %bb.0:
	stp	d15, d14, [sp, #-112]!          ; 16-byte Folded Spill
	stp	d13, d12, [sp, #16]             ; 16-byte Folded Spill
	stp	d11, d10, [sp, #32]             ; 16-byte Folded Spill
	stp	d9, d8, [sp, #48]               ; 16-byte Folded Spill
	stp	x28, x27, [sp, #64]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #80]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #96]             ; 16-byte Folded Spill
	add	x29, sp, #96
	sub	sp, sp, #1648
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w27, -40
	.cfi_offset w28, -48
	.cfi_offset b8, -56
	.cfi_offset b9, -64
	.cfi_offset b10, -72
	.cfi_offset b11, -80
	.cfi_offset b12, -88
	.cfi_offset b13, -96
	.cfi_offset b14, -104
	.cfi_offset b15, -112
	mov	x19, x0
Lloh95:
	adrp	x8, ___stack_chk_guard@GOTPAGE
Lloh96:
	ldr	x8, [x8, ___stack_chk_guard@GOTPAGEOFF]
Lloh97:
	ldr	x8, [x8]
	stur	x8, [x29, #-104]
Lloh98:
	adrp	x20, _input_layer_bias_q@GOTPAGE
Lloh99:
	ldr	x20, [x20, _input_layer_bias_q@GOTPAGEOFF]
	add	x0, sp, #1120
	mov	x1, x20
	mov	w2, #512                        ; =0x200
	bl	_memcpy
	add	x0, sp, #608
	mov	x1, x20
	mov	w2, #512                        ; =0x200
	bl	_memcpy
	ldrsb	w10, [x19, #2293]
	ldrsb	w8, [x19, #2294]
	eor	w11, w8, #0x38
	add	x8, x19, #1024
	add	x9, x19, #1080
	lsl	w12, w10, #6
	sub	w10, w12, #1, lsl #12           ; =4096
	lsl	w13, w11, #6
	add	w11, w13, #4, lsl #12           ; =16384
	add	w12, w12, #4, lsl #12           ; =16384
	sub	w13, w13, #1, lsl #12           ; =4096
	mov	w14, #1                         ; =0x1
Lloh100:
	adrp	x15, _input_layer_weights_q@GOTPAGE
Lloh101:
	ldr	x15, [x15, _input_layer_weights_q@GOTPAGEOFF]
	b	LBB8_2
LBB8_1:                                 ;   in Loop: Header=BB8_2 Depth=1
	add	x14, x14, #1
	cmp	x14, #6
	b.eq	LBB8_10
LBB8_2:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB8_4 Depth 2
                                        ;     Child Loop BB8_8 Depth 2
	ldr	x16, [x8, x14, lsl #3]
	cbz	x16, LBB8_6
; %bb.3:                                ;   in Loop: Header=BB8_2 Depth=1
	lsl	w0, w14, #12
	add	w17, w10, w0
	add	w0, w11, w0
	ldr	q15, [sp, #1120]
	ldr	q14, [sp, #1136]
	ldr	q13, [sp, #1152]
	ldr	q12, [sp, #1168]
	ldr	q11, [sp, #1184]
	ldr	q10, [sp, #1200]
	ldr	q9, [sp, #1216]
	ldr	q8, [sp, #1232]
	ldr	q31, [sp, #1248]
	ldr	q30, [sp, #1264]
	ldr	q29, [sp, #1280]
	ldr	q28, [sp, #1296]
	ldr	q27, [sp, #1312]
	ldr	q26, [sp, #1328]
	ldr	q25, [sp, #1344]
	ldr	q24, [sp, #1360]
	ldr	q23, [sp, #1376]
	ldr	q22, [sp, #1392]
	ldr	q21, [sp, #1408]
	ldr	q20, [sp, #1424]
	ldr	q19, [sp, #1440]
	ldr	q18, [sp, #1456]
	ldr	q17, [sp, #1472]
	ldr	q16, [sp, #1488]
	ldr	q7, [sp, #1504]
	ldr	q6, [sp, #1520]
	ldr	q5, [sp, #1536]
	ldr	q4, [sp, #1552]
	ldr	q3, [sp, #1568]
	ldr	q0, [sp, #1584]
	str	q0, [sp, #528]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #1600]
	ldr	q0, [sp, #1616]
	stp	q1, q0, [sp, #560]              ; 32-byte Folded Spill
	sxtw	x17, w17
	sxtw	x0, w0
	ldp	q1, q0, [sp, #608]
	stp	q1, q0, [sp, #48]               ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #640]
	stp	q1, q0, [sp, #80]               ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #672]
	stp	q1, q0, [sp, #112]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #704]
	stp	q1, q0, [sp, #144]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #736]
	stp	q1, q0, [sp, #176]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #768]
	stp	q1, q0, [sp, #208]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #800]
	stp	q1, q0, [sp, #240]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #832]
	stp	q1, q0, [sp, #272]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #864]
	stp	q1, q0, [sp, #304]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #896]
	stp	q1, q0, [sp, #336]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #928]
	stp	q1, q0, [sp, #368]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #960]
	stp	q1, q0, [sp, #400]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #992]
	stp	q1, q0, [sp, #432]              ; 32-byte Folded Spill
	ldr	q1, [sp, #1024]
	ldr	q0, [sp, #1040]
	stp	q1, q0, [sp, #464]              ; 32-byte Folded Spill
	ldr	q1, [sp, #1056]
	ldr	q0, [sp, #1072]
	stp	q1, q0, [sp, #496]              ; 32-byte Folded Spill
	ldr	q0, [sp, #1088]
	str	q0, [sp, #544]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #1104]
LBB8_4:                                 ;   Parent Loop BB8_2 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	str	q1, [sp, #592]                  ; 16-byte Folded Spill
	rbit	x1, x16
	clz	x1, x1
	orr	x2, x17, x1
	add	x2, x15, x2, lsl #9
	ldp	q0, q1, [x2]
	add.8h	v15, v15, v0
	str	q15, [sp, #32]                  ; 16-byte Folded Spill
	add.8h	v15, v14, v1
	ldp	q0, q1, [x2, #32]
	add.8h	v14, v13, v0
	add.8h	v12, v12, v1
	ldp	q0, q1, [x2, #64]
	add.8h	v11, v11, v0
	add.8h	v10, v10, v1
	ldp	q0, q1, [x2, #96]
	add.8h	v9, v9, v0
	add.8h	v8, v8, v1
	ldp	q0, q1, [x2, #128]
	add.8h	v31, v31, v0
	add.8h	v30, v30, v1
	ldp	q0, q1, [x2, #160]
	add.8h	v29, v29, v0
	add.8h	v28, v28, v1
	ldp	q0, q1, [x2, #192]
	add.8h	v27, v27, v0
	add.8h	v26, v26, v1
	ldp	q0, q1, [x2, #224]
	add.8h	v25, v25, v0
	add.8h	v24, v24, v1
	ldp	q0, q1, [x2, #256]
	add.8h	v23, v23, v0
	add.8h	v22, v22, v1
	ldp	q0, q1, [x2, #288]
	add.8h	v21, v21, v0
	add.8h	v20, v20, v1
	ldp	q0, q1, [x2, #320]
	add.8h	v19, v19, v0
	add.8h	v18, v18, v1
	ldp	q0, q1, [x2, #352]
	add.8h	v17, v17, v0
	add.8h	v16, v16, v1
	ldp	q0, q1, [x2, #384]
	add.8h	v7, v7, v0
	add.8h	v6, v6, v1
	ldp	q0, q1, [x2, #416]
	add.8h	v5, v5, v0
	add.8h	v4, v4, v1
	ldp	q0, q1, [x2, #448]
	add.8h	v3, v3, v0
	ldr	q0, [sp, #528]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	str	q0, [sp, #528]                  ; 16-byte Folded Spill
	ldp	q0, q1, [x2, #480]
	mov.16b	v13, v12
	mov.16b	v12, v11
	mov.16b	v11, v10
	mov.16b	v10, v9
	mov.16b	v9, v8
	mov.16b	v8, v31
	mov.16b	v31, v30
	mov.16b	v30, v29
	mov.16b	v29, v28
	mov.16b	v28, v27
	mov.16b	v27, v26
	mov.16b	v26, v25
	mov.16b	v25, v24
	mov.16b	v24, v23
	mov.16b	v23, v22
	mov.16b	v22, v21
	mov.16b	v21, v20
	mov.16b	v20, v19
	mov.16b	v19, v18
	mov.16b	v18, v17
	mov.16b	v17, v16
	mov.16b	v16, v7
	mov.16b	v7, v6
	mov.16b	v6, v5
	ldr	q2, [sp, #560]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	str	q2, [sp, #560]                  ; 16-byte Folded Spill
	mov.16b	v5, v6
	mov.16b	v6, v7
	mov.16b	v7, v16
	mov.16b	v16, v17
	mov.16b	v17, v18
	mov.16b	v18, v19
	mov.16b	v19, v20
	mov.16b	v20, v21
	mov.16b	v21, v22
	mov.16b	v22, v23
	mov.16b	v23, v24
	mov.16b	v24, v25
	mov.16b	v25, v26
	mov.16b	v26, v27
	mov.16b	v27, v28
	mov.16b	v28, v29
	mov.16b	v29, v30
	mov.16b	v30, v31
	mov.16b	v31, v8
	mov.16b	v8, v9
	mov.16b	v9, v10
	mov.16b	v10, v11
	mov.16b	v11, v12
	mov.16b	v12, v13
	mov.16b	v13, v14
	mov.16b	v14, v15
	ldp	q15, q2, [sp, #32]              ; 32-byte Folded Reload
	ldr	q0, [sp, #576]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	str	q0, [sp, #576]                  ; 16-byte Folded Spill
	orr	x1, x0, x1
	eor	x1, x1, #0x38
	add	x1, x15, x1, lsl #9
	ldp	q0, q1, [x1]
	add.8h	v2, v2, v0
	ldr	q0, [sp, #64]                   ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #48]               ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #32]
	ldr	q2, [sp, #80]                   ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #96]                   ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #80]               ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #64]
	ldr	q2, [sp, #112]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #128]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #112]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #96]
	ldr	q2, [sp, #144]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #160]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #144]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #128]
	ldr	q2, [sp, #176]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #192]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #176]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #160]
	ldr	q2, [sp, #208]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #224]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #208]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #192]
	ldr	q2, [sp, #240]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #256]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #240]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #224]
	ldr	q2, [sp, #272]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #288]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #272]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #256]
	ldr	q2, [sp, #304]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #320]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #304]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #288]
	ldr	q2, [sp, #336]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #352]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #336]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #320]
	ldr	q2, [sp, #368]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #384]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #368]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #352]
	ldr	q2, [sp, #400]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #416]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #400]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #384]
	ldr	q2, [sp, #432]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #448]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #432]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #416]
	ldr	q2, [sp, #464]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #480]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #464]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #448]
	ldr	q2, [sp, #496]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #512]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #496]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #480]
	ldr	q2, [sp, #544]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	str	q2, [sp, #544]                  ; 16-byte Folded Spill
	ldr	q0, [sp, #592]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	str	q0, [sp, #592]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #592]                  ; 16-byte Folded Reload
	sub	x1, x16, #1
	ands	x16, x1, x16
	b.ne	LBB8_4
; %bb.5:                                ;   in Loop: Header=BB8_2 Depth=1
	str	q15, [sp, #1120]
	str	q14, [sp, #1136]
	str	q13, [sp, #1152]
	str	q12, [sp, #1168]
	str	q11, [sp, #1184]
	str	q10, [sp, #1200]
	str	q9, [sp, #1216]
	str	q8, [sp, #1232]
	str	q31, [sp, #1248]
	str	q30, [sp, #1264]
	str	q29, [sp, #1280]
	str	q28, [sp, #1296]
	str	q27, [sp, #1312]
	str	q26, [sp, #1328]
	str	q25, [sp, #1344]
	str	q24, [sp, #1360]
	str	q23, [sp, #1376]
	str	q22, [sp, #1392]
	str	q21, [sp, #1408]
	str	q20, [sp, #1424]
	str	q19, [sp, #1440]
	str	q18, [sp, #1456]
	str	q17, [sp, #1472]
	str	q16, [sp, #1488]
	str	q7, [sp, #1504]
	str	q6, [sp, #1520]
	str	q5, [sp, #1536]
	str	q4, [sp, #1552]
	str	q3, [sp, #1568]
	ldr	q0, [sp, #528]                  ; 16-byte Folded Reload
	str	q0, [sp, #1584]
	ldp	q2, q0, [sp, #560]              ; 32-byte Folded Reload
	str	q2, [sp, #1600]
	str	q0, [sp, #1616]
	ldp	q2, q0, [sp, #48]               ; 32-byte Folded Reload
	stp	q2, q0, [sp, #608]
	ldp	q2, q0, [sp, #80]               ; 32-byte Folded Reload
	stp	q2, q0, [sp, #640]
	ldp	q2, q0, [sp, #112]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #672]
	ldp	q2, q0, [sp, #144]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #704]
	ldp	q2, q0, [sp, #176]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #736]
	ldp	q2, q0, [sp, #208]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #768]
	ldp	q2, q0, [sp, #240]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #800]
	ldp	q2, q0, [sp, #272]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #832]
	ldp	q2, q0, [sp, #304]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #864]
	ldp	q2, q0, [sp, #336]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #896]
	ldp	q2, q0, [sp, #368]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #928]
	ldp	q2, q0, [sp, #400]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #960]
	ldp	q2, q0, [sp, #432]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #992]
	ldp	q2, q0, [sp, #464]              ; 32-byte Folded Reload
	str	q2, [sp, #1024]
	str	q0, [sp, #1040]
	ldp	q2, q0, [sp, #496]              ; 32-byte Folded Reload
	str	q2, [sp, #1056]
	str	q0, [sp, #1072]
	ldr	q0, [sp, #544]                  ; 16-byte Folded Reload
	str	q0, [sp, #1088]
	str	q1, [sp, #1104]
LBB8_6:                                 ;   in Loop: Header=BB8_2 Depth=1
	ldr	x16, [x9, x14, lsl #3]
	cbz	x16, LBB8_1
; %bb.7:                                ;   in Loop: Header=BB8_2 Depth=1
	lsl	w0, w14, #12
	add	w17, w12, w0
	add	w0, w13, w0
	ldr	q15, [sp, #1120]
	ldr	q14, [sp, #1136]
	ldr	q13, [sp, #1152]
	ldr	q12, [sp, #1168]
	ldr	q11, [sp, #1184]
	ldr	q10, [sp, #1200]
	ldr	q9, [sp, #1216]
	ldr	q8, [sp, #1232]
	ldr	q31, [sp, #1248]
	ldr	q30, [sp, #1264]
	ldr	q29, [sp, #1280]
	ldr	q28, [sp, #1296]
	ldr	q27, [sp, #1312]
	ldr	q26, [sp, #1328]
	ldr	q25, [sp, #1344]
	ldr	q24, [sp, #1360]
	ldr	q23, [sp, #1376]
	ldr	q22, [sp, #1392]
	ldr	q21, [sp, #1408]
	ldr	q20, [sp, #1424]
	ldr	q19, [sp, #1440]
	ldr	q18, [sp, #1456]
	ldr	q17, [sp, #1472]
	ldr	q16, [sp, #1488]
	ldr	q7, [sp, #1504]
	ldr	q6, [sp, #1520]
	ldr	q5, [sp, #1536]
	ldr	q4, [sp, #1552]
	ldr	q3, [sp, #1568]
	ldr	q0, [sp, #1584]
	str	q0, [sp, #528]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #1600]
	ldr	q0, [sp, #1616]
	stp	q1, q0, [sp, #560]              ; 32-byte Folded Spill
	sxtw	x17, w17
	sxtw	x0, w0
	ldp	q1, q0, [sp, #608]
	stp	q1, q0, [sp, #48]               ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #640]
	stp	q1, q0, [sp, #80]               ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #672]
	stp	q1, q0, [sp, #112]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #704]
	stp	q1, q0, [sp, #144]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #736]
	stp	q1, q0, [sp, #176]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #768]
	stp	q1, q0, [sp, #208]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #800]
	stp	q1, q0, [sp, #240]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #832]
	stp	q1, q0, [sp, #272]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #864]
	stp	q1, q0, [sp, #304]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #896]
	stp	q1, q0, [sp, #336]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #928]
	stp	q1, q0, [sp, #368]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #960]
	stp	q1, q0, [sp, #400]              ; 32-byte Folded Spill
	ldp	q1, q0, [sp, #992]
	stp	q1, q0, [sp, #432]              ; 32-byte Folded Spill
	ldr	q1, [sp, #1024]
	ldr	q0, [sp, #1040]
	stp	q1, q0, [sp, #464]              ; 32-byte Folded Spill
	ldr	q1, [sp, #1056]
	ldr	q0, [sp, #1072]
	stp	q1, q0, [sp, #496]              ; 32-byte Folded Spill
	ldr	q0, [sp, #1088]
	str	q0, [sp, #544]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #1104]
LBB8_8:                                 ;   Parent Loop BB8_2 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	str	q1, [sp, #592]                  ; 16-byte Folded Spill
	rbit	x1, x16
	clz	x1, x1
	orr	x2, x17, x1
	add	x2, x15, x2, lsl #9
	ldp	q0, q1, [x2]
	add.8h	v15, v15, v0
	str	q15, [sp, #32]                  ; 16-byte Folded Spill
	add.8h	v15, v14, v1
	ldp	q0, q1, [x2, #32]
	add.8h	v14, v13, v0
	add.8h	v12, v12, v1
	ldp	q0, q1, [x2, #64]
	add.8h	v11, v11, v0
	add.8h	v10, v10, v1
	ldp	q0, q1, [x2, #96]
	add.8h	v9, v9, v0
	add.8h	v8, v8, v1
	ldp	q0, q1, [x2, #128]
	add.8h	v31, v31, v0
	add.8h	v30, v30, v1
	ldp	q0, q1, [x2, #160]
	add.8h	v29, v29, v0
	add.8h	v28, v28, v1
	ldp	q0, q1, [x2, #192]
	add.8h	v27, v27, v0
	add.8h	v26, v26, v1
	ldp	q0, q1, [x2, #224]
	add.8h	v25, v25, v0
	add.8h	v24, v24, v1
	ldp	q0, q1, [x2, #256]
	add.8h	v23, v23, v0
	add.8h	v22, v22, v1
	ldp	q0, q1, [x2, #288]
	add.8h	v21, v21, v0
	add.8h	v20, v20, v1
	ldp	q0, q1, [x2, #320]
	add.8h	v19, v19, v0
	add.8h	v18, v18, v1
	ldp	q0, q1, [x2, #352]
	add.8h	v17, v17, v0
	add.8h	v16, v16, v1
	ldp	q0, q1, [x2, #384]
	add.8h	v7, v7, v0
	add.8h	v6, v6, v1
	ldp	q0, q1, [x2, #416]
	add.8h	v5, v5, v0
	add.8h	v4, v4, v1
	ldp	q0, q1, [x2, #448]
	add.8h	v3, v3, v0
	ldr	q0, [sp, #528]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	str	q0, [sp, #528]                  ; 16-byte Folded Spill
	ldp	q0, q1, [x2, #480]
	mov.16b	v13, v12
	mov.16b	v12, v11
	mov.16b	v11, v10
	mov.16b	v10, v9
	mov.16b	v9, v8
	mov.16b	v8, v31
	mov.16b	v31, v30
	mov.16b	v30, v29
	mov.16b	v29, v28
	mov.16b	v28, v27
	mov.16b	v27, v26
	mov.16b	v26, v25
	mov.16b	v25, v24
	mov.16b	v24, v23
	mov.16b	v23, v22
	mov.16b	v22, v21
	mov.16b	v21, v20
	mov.16b	v20, v19
	mov.16b	v19, v18
	mov.16b	v18, v17
	mov.16b	v17, v16
	mov.16b	v16, v7
	mov.16b	v7, v6
	mov.16b	v6, v5
	ldr	q2, [sp, #560]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	str	q2, [sp, #560]                  ; 16-byte Folded Spill
	mov.16b	v5, v6
	mov.16b	v6, v7
	mov.16b	v7, v16
	mov.16b	v16, v17
	mov.16b	v17, v18
	mov.16b	v18, v19
	mov.16b	v19, v20
	mov.16b	v20, v21
	mov.16b	v21, v22
	mov.16b	v22, v23
	mov.16b	v23, v24
	mov.16b	v24, v25
	mov.16b	v25, v26
	mov.16b	v26, v27
	mov.16b	v27, v28
	mov.16b	v28, v29
	mov.16b	v29, v30
	mov.16b	v30, v31
	mov.16b	v31, v8
	mov.16b	v8, v9
	mov.16b	v9, v10
	mov.16b	v10, v11
	mov.16b	v11, v12
	mov.16b	v12, v13
	mov.16b	v13, v14
	mov.16b	v14, v15
	ldp	q15, q2, [sp, #32]              ; 32-byte Folded Reload
	ldr	q0, [sp, #576]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	str	q0, [sp, #576]                  ; 16-byte Folded Spill
	orr	x1, x0, x1
	eor	x1, x1, #0x38
	add	x1, x15, x1, lsl #9
	ldp	q0, q1, [x1]
	add.8h	v2, v2, v0
	ldr	q0, [sp, #64]                   ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #48]               ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #32]
	ldr	q2, [sp, #80]                   ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #96]                   ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #80]               ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #64]
	ldr	q2, [sp, #112]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #128]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #112]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #96]
	ldr	q2, [sp, #144]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #160]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #144]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #128]
	ldr	q2, [sp, #176]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #192]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #176]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #160]
	ldr	q2, [sp, #208]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #224]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #208]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #192]
	ldr	q2, [sp, #240]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #256]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #240]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #224]
	ldr	q2, [sp, #272]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #288]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #272]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #256]
	ldr	q2, [sp, #304]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #320]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #304]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #288]
	ldr	q2, [sp, #336]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #352]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #336]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #320]
	ldr	q2, [sp, #368]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #384]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #368]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #352]
	ldr	q2, [sp, #400]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #416]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #400]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #384]
	ldr	q2, [sp, #432]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #448]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #432]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #416]
	ldr	q2, [sp, #464]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #480]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #464]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #448]
	ldr	q2, [sp, #496]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	ldr	q0, [sp, #512]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	stp	q2, q0, [sp, #496]              ; 32-byte Folded Spill
	ldp	q0, q1, [x1, #480]
	ldr	q2, [sp, #544]                  ; 16-byte Folded Reload
	add.8h	v2, v2, v0
	str	q2, [sp, #544]                  ; 16-byte Folded Spill
	ldr	q0, [sp, #592]                  ; 16-byte Folded Reload
	add.8h	v0, v0, v1
	str	q0, [sp, #592]                  ; 16-byte Folded Spill
	ldr	q1, [sp, #592]                  ; 16-byte Folded Reload
	sub	x1, x16, #1
	ands	x16, x1, x16
	b.ne	LBB8_8
; %bb.9:                                ;   in Loop: Header=BB8_2 Depth=1
	str	q15, [sp, #1120]
	str	q14, [sp, #1136]
	str	q13, [sp, #1152]
	str	q12, [sp, #1168]
	str	q11, [sp, #1184]
	str	q10, [sp, #1200]
	str	q9, [sp, #1216]
	str	q8, [sp, #1232]
	str	q31, [sp, #1248]
	str	q30, [sp, #1264]
	str	q29, [sp, #1280]
	str	q28, [sp, #1296]
	str	q27, [sp, #1312]
	str	q26, [sp, #1328]
	str	q25, [sp, #1344]
	str	q24, [sp, #1360]
	str	q23, [sp, #1376]
	str	q22, [sp, #1392]
	str	q21, [sp, #1408]
	str	q20, [sp, #1424]
	str	q19, [sp, #1440]
	str	q18, [sp, #1456]
	str	q17, [sp, #1472]
	str	q16, [sp, #1488]
	str	q7, [sp, #1504]
	str	q6, [sp, #1520]
	str	q5, [sp, #1536]
	str	q4, [sp, #1552]
	str	q3, [sp, #1568]
	ldr	q0, [sp, #528]                  ; 16-byte Folded Reload
	str	q0, [sp, #1584]
	ldp	q2, q0, [sp, #560]              ; 32-byte Folded Reload
	str	q2, [sp, #1600]
	str	q0, [sp, #1616]
	ldp	q2, q0, [sp, #48]               ; 32-byte Folded Reload
	stp	q2, q0, [sp, #608]
	ldp	q2, q0, [sp, #80]               ; 32-byte Folded Reload
	stp	q2, q0, [sp, #640]
	ldp	q2, q0, [sp, #112]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #672]
	ldp	q2, q0, [sp, #144]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #704]
	ldp	q2, q0, [sp, #176]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #736]
	ldp	q2, q0, [sp, #208]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #768]
	ldp	q2, q0, [sp, #240]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #800]
	ldp	q2, q0, [sp, #272]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #832]
	ldp	q2, q0, [sp, #304]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #864]
	ldp	q2, q0, [sp, #336]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #896]
	ldp	q2, q0, [sp, #368]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #928]
	ldp	q2, q0, [sp, #400]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #960]
	ldp	q2, q0, [sp, #432]              ; 32-byte Folded Reload
	stp	q2, q0, [sp, #992]
	ldp	q2, q0, [sp, #464]              ; 32-byte Folded Reload
	str	q2, [sp, #1024]
	str	q0, [sp, #1040]
	ldp	q2, q0, [sp, #496]              ; 32-byte Folded Reload
	str	q2, [sp, #1056]
	str	q0, [sp, #1072]
	ldr	q0, [sp, #544]                  ; 16-byte Folded Reload
	str	q0, [sp, #1088]
	str	q1, [sp, #1104]
	b	LBB8_1
LBB8_10:
	mov	x8, #0                          ; =0x0
	add	x12, x19, #1196
	add	x9, sp, #1120
	add	x10, sp, #608
	mov	x11, x12
LBB8_11:                                ; =>This Inner Loop Header: Depth=1
	ldrh	w13, [x11], #2
	ldrh	w14, [x9, x8, lsl #1]
	cmp	w13, w14
	b.ne	LBB8_17
; %bb.12:                               ;   in Loop: Header=BB8_11 Depth=1
	ldrh	w12, [x12, #512]
	ldrh	w13, [x10, x8, lsl #1]
	cmp	w12, w13
	b.ne	LBB8_16
; %bb.13:                               ;   in Loop: Header=BB8_11 Depth=1
	add	x8, x8, #1
	mov	x12, x11
	cmp	x8, #256
	b.ne	LBB8_11
; %bb.14:
	ldur	x8, [x29, #-104]
Lloh102:
	adrp	x9, ___stack_chk_guard@GOTPAGE
Lloh103:
	ldr	x9, [x9, ___stack_chk_guard@GOTPAGEOFF]
Lloh104:
	ldr	x9, [x9]
	cmp	x9, x8
	b.ne	LBB8_18
; %bb.15:
	add	sp, sp, #1648
	ldp	x29, x30, [sp, #96]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #80]             ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #64]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp, #48]               ; 16-byte Folded Reload
	ldp	d11, d10, [sp, #32]             ; 16-byte Folded Reload
	ldp	d13, d12, [sp, #16]             ; 16-byte Folded Reload
	ldp	d15, d14, [sp], #112            ; 16-byte Folded Reload
	ret
LBB8_16:
	sxth	w9, w12
	sxth	w10, w13
Lloh105:
	adrp	x11, ___stderrp@GOTPAGE
Lloh106:
	ldr	x11, [x11, ___stderrp@GOTPAGEOFF]
Lloh107:
	ldr	x0, [x11]
	stp	x10, x9, [sp, #8]
	str	x8, [sp]
Lloh108:
	adrp	x1, l_.str.21@PAGE
Lloh109:
	add	x1, x1, l_.str.21@PAGEOFF
	bl	_fprintf
	mov	x0, x19
	bl	_ShowPosition
	bl	_abort
LBB8_17:
	sxth	w9, w13
	sxth	w10, w14
Lloh110:
	adrp	x11, ___stderrp@GOTPAGE
Lloh111:
	ldr	x11, [x11, ___stderrp@GOTPAGEOFF]
Lloh112:
	ldr	x0, [x11]
	stp	x10, x9, [sp, #8]
	str	x8, [sp]
Lloh113:
	adrp	x1, l_.str.20@PAGE
Lloh114:
	add	x1, x1, l_.str.20@PAGEOFF
	bl	_fprintf
	mov	x0, x19
	bl	_ShowPosition
	bl	_abort
LBB8_18:
	bl	___stack_chk_fail
	.loh AdrpLdrGot	Lloh100, Lloh101
	.loh AdrpLdrGot	Lloh98, Lloh99
	.loh AdrpLdrGotLdr	Lloh95, Lloh96, Lloh97
	.loh AdrpLdrGotLdr	Lloh102, Lloh103, Lloh104
	.loh AdrpAdd	Lloh108, Lloh109
	.loh AdrpLdrGotLdr	Lloh105, Lloh106, Lloh107
	.loh AdrpAdd	Lloh113, Lloh114
	.loh AdrpLdrGotLdr	Lloh110, Lloh111, Lloh112
	.cfi_endproc
                                        ; -- End function
	.globl	_ReadWeights                    ; -- Begin function ReadWeights
	.p2align	2
_ReadWeights:                           ; @ReadWeights
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	bl	_ReadWeightsQuantized
	ldp	x29, x30, [sp], #16             ; 16-byte Folded Reload
	b	_ReadWeightsCompressed
	.cfi_endproc
                                        ; -- End function
	.globl	_RandomizeWeights               ; -- Begin function RandomizeWeights
	.p2align	2
_RandomizeWeights:                      ; @RandomizeWeights
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
	mov	x19, #0                         ; =0x0
Lloh115:
	adrp	x20, _input_layer_weights_q@GOTPAGE
Lloh116:
	ldr	x20, [x20, _input_layer_weights_q@GOTPAGEOFF]
LBB10_1:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB10_2 Depth 2
	mov	x21, #0                         ; =0x0
LBB10_2:                                ;   Parent Loop BB10_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	bl	_Random
	fcvtzs	w8, d0, #10
	strh	w8, [x20, x21]
	add	x21, x21, #2
	cmp	x21, #512
	b.ne	LBB10_2
; %bb.3:                                ;   in Loop: Header=BB10_1 Depth=1
	add	x19, x19, #1
	add	x20, x20, #512
	cmp	x19, #10, lsl #12               ; =40960
	b.ne	LBB10_1
; %bb.4:
	mov	x19, #0                         ; =0x0
Lloh117:
	adrp	x20, _input_layer_bias_q@GOTPAGE
Lloh118:
	ldr	x20, [x20, _input_layer_bias_q@GOTPAGEOFF]
LBB10_5:                                ; =>This Inner Loop Header: Depth=1
	bl	_Random
	fcvtzs	w8, d0, #10
	strh	w8, [x20, x19]
	add	x19, x19, #2
	cmp	x19, #512
	b.ne	LBB10_5
; %bb.6:
	ldp	x29, x30, [sp, #32]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #16]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp], #48             ; 16-byte Folded Reload
	ret
	.loh AdrpLdrGot	Lloh115, Lloh116
	.loh AdrpLdrGot	Lloh117, Lloh118
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsQuantized.cold.1
_ReadWeightsQuantized.cold.1:           ; @ReadWeightsQuantized.cold.1
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh119:
	adrp	x0, l___func__.ReadWeightsQuantized@PAGE
Lloh120:
	add	x0, x0, l___func__.ReadWeightsQuantized@PAGEOFF
Lloh121:
	adrp	x1, l_.str.3@PAGE
Lloh122:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh123:
	adrp	x3, l_.str.4@PAGE
Lloh124:
	add	x3, x3, l_.str.4@PAGEOFF
	mov	w2, #77                         ; =0x4d
	bl	___assert_rtn
	.loh AdrpAdd	Lloh123, Lloh124
	.loh AdrpAdd	Lloh121, Lloh122
	.loh AdrpAdd	Lloh119, Lloh120
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsQuantized.cold.2
_ReadWeightsQuantized.cold.2:           ; @ReadWeightsQuantized.cold.2
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh125:
	adrp	x0, l___func__.ReadWeightsQuantized@PAGE
Lloh126:
	add	x0, x0, l___func__.ReadWeightsQuantized@PAGEOFF
Lloh127:
	adrp	x1, l_.str.3@PAGE
Lloh128:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh129:
	adrp	x3, l_.str.5@PAGE
Lloh130:
	add	x3, x3, l_.str.5@PAGEOFF
	mov	w2, #81                         ; =0x51
	bl	___assert_rtn
	.loh AdrpAdd	Lloh129, Lloh130
	.loh AdrpAdd	Lloh127, Lloh128
	.loh AdrpAdd	Lloh125, Lloh126
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsQuantized.cold.3
_ReadWeightsQuantized.cold.3:           ; @ReadWeightsQuantized.cold.3
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh131:
	adrp	x0, l___func__.ReadWeightsQuantized@PAGE
Lloh132:
	add	x0, x0, l___func__.ReadWeightsQuantized@PAGEOFF
Lloh133:
	adrp	x1, l_.str.3@PAGE
Lloh134:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh135:
	adrp	x3, l_.str.6@PAGE
Lloh136:
	add	x3, x3, l_.str.6@PAGEOFF
	mov	w2, #85                         ; =0x55
	bl	___assert_rtn
	.loh AdrpAdd	Lloh135, Lloh136
	.loh AdrpAdd	Lloh133, Lloh134
	.loh AdrpAdd	Lloh131, Lloh132
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsQuantized.cold.4
_ReadWeightsQuantized.cold.4:           ; @ReadWeightsQuantized.cold.4
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh137:
	adrp	x0, l___func__.ReadWeightsQuantized@PAGE
Lloh138:
	add	x0, x0, l___func__.ReadWeightsQuantized@PAGEOFF
Lloh139:
	adrp	x1, l_.str.3@PAGE
Lloh140:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh141:
	adrp	x3, l_.str.7@PAGE
Lloh142:
	add	x3, x3, l_.str.7@PAGEOFF
	mov	w2, #88                         ; =0x58
	bl	___assert_rtn
	.loh AdrpAdd	Lloh141, Lloh142
	.loh AdrpAdd	Lloh139, Lloh140
	.loh AdrpAdd	Lloh137, Lloh138
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsQuantized.cold.5
_ReadWeightsQuantized.cold.5:           ; @ReadWeightsQuantized.cold.5
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh143:
	adrp	x0, l___func__.ReadWeightsQuantized@PAGE
Lloh144:
	add	x0, x0, l___func__.ReadWeightsQuantized@PAGEOFF
Lloh145:
	adrp	x1, l_.str.3@PAGE
Lloh146:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh147:
	adrp	x3, l_.str.8@PAGE
Lloh148:
	add	x3, x3, l_.str.8@PAGEOFF
	mov	w2, #91                         ; =0x5b
	bl	___assert_rtn
	.loh AdrpAdd	Lloh147, Lloh148
	.loh AdrpAdd	Lloh145, Lloh146
	.loh AdrpAdd	Lloh143, Lloh144
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsQuantized.cold.6
_ReadWeightsQuantized.cold.6:           ; @ReadWeightsQuantized.cold.6
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh149:
	adrp	x0, l___func__.ReadWeightsQuantized@PAGE
Lloh150:
	add	x0, x0, l___func__.ReadWeightsQuantized@PAGEOFF
Lloh151:
	adrp	x1, l_.str.3@PAGE
Lloh152:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh153:
	adrp	x3, l_.str.7@PAGE
Lloh154:
	add	x3, x3, l_.str.7@PAGEOFF
	mov	w2, #94                         ; =0x5e
	bl	___assert_rtn
	.loh AdrpAdd	Lloh153, Lloh154
	.loh AdrpAdd	Lloh151, Lloh152
	.loh AdrpAdd	Lloh149, Lloh150
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsQuantized.cold.7
_ReadWeightsQuantized.cold.7:           ; @ReadWeightsQuantized.cold.7
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh155:
	adrp	x0, l___func__.ReadWeightsQuantized@PAGE
Lloh156:
	add	x0, x0, l___func__.ReadWeightsQuantized@PAGEOFF
Lloh157:
	adrp	x1, l_.str.3@PAGE
Lloh158:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh159:
	adrp	x3, l_.str.7@PAGE
Lloh160:
	add	x3, x3, l_.str.7@PAGEOFF
	mov	w2, #97                         ; =0x61
	bl	___assert_rtn
	.loh AdrpAdd	Lloh159, Lloh160
	.loh AdrpAdd	Lloh157, Lloh158
	.loh AdrpAdd	Lloh155, Lloh156
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsQuantized.cold.8
_ReadWeightsQuantized.cold.8:           ; @ReadWeightsQuantized.cold.8
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh161:
	adrp	x0, l___func__.ReadWeightsQuantized@PAGE
Lloh162:
	add	x0, x0, l___func__.ReadWeightsQuantized@PAGEOFF
Lloh163:
	adrp	x1, l_.str.3@PAGE
Lloh164:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh165:
	adrp	x3, l_.str.9@PAGE
Lloh166:
	add	x3, x3, l_.str.9@PAGEOFF
	mov	w2, #100                        ; =0x64
	bl	___assert_rtn
	.loh AdrpAdd	Lloh165, Lloh166
	.loh AdrpAdd	Lloh163, Lloh164
	.loh AdrpAdd	Lloh161, Lloh162
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsCompressed.cold.1
_ReadWeightsCompressed.cold.1:          ; @ReadWeightsCompressed.cold.1
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh167:
	adrp	x0, l___func__.ReadWeightsCompressed@PAGE
Lloh168:
	add	x0, x0, l___func__.ReadWeightsCompressed@PAGEOFF
Lloh169:
	adrp	x1, l_.str.3@PAGE
Lloh170:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh171:
	adrp	x3, l_.str.9@PAGE
Lloh172:
	add	x3, x3, l_.str.9@PAGEOFF
	mov	w2, #121                        ; =0x79
	bl	___assert_rtn
	.loh AdrpAdd	Lloh171, Lloh172
	.loh AdrpAdd	Lloh169, Lloh170
	.loh AdrpAdd	Lloh167, Lloh168
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsCompressed.cold.2
_ReadWeightsCompressed.cold.2:          ; @ReadWeightsCompressed.cold.2
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh173:
	adrp	x0, l___func__.ReadWeightsCompressed@PAGE
Lloh174:
	add	x0, x0, l___func__.ReadWeightsCompressed@PAGEOFF
Lloh175:
	adrp	x1, l_.str.3@PAGE
Lloh176:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh177:
	adrp	x3, l_.str.12@PAGE
Lloh178:
	add	x3, x3, l_.str.12@PAGEOFF
	mov	w2, #163                        ; =0xa3
	bl	___assert_rtn
	.loh AdrpAdd	Lloh177, Lloh178
	.loh AdrpAdd	Lloh175, Lloh176
	.loh AdrpAdd	Lloh173, Lloh174
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsCompressed.cold.3
_ReadWeightsCompressed.cold.3:          ; @ReadWeightsCompressed.cold.3
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh179:
	adrp	x0, l___func__.ReadWeightsCompressed@PAGE
Lloh180:
	add	x0, x0, l___func__.ReadWeightsCompressed@PAGEOFF
Lloh181:
	adrp	x1, l_.str.3@PAGE
Lloh182:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh183:
	adrp	x3, l_.str.13@PAGE
Lloh184:
	add	x3, x3, l_.str.13@PAGEOFF
	mov	w2, #167                        ; =0xa7
	bl	___assert_rtn
	.loh AdrpAdd	Lloh183, Lloh184
	.loh AdrpAdd	Lloh181, Lloh182
	.loh AdrpAdd	Lloh179, Lloh180
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsCompressed.cold.4
_ReadWeightsCompressed.cold.4:          ; @ReadWeightsCompressed.cold.4
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh185:
	adrp	x0, l___func__.ReadWeightsCompressed@PAGE
Lloh186:
	add	x0, x0, l___func__.ReadWeightsCompressed@PAGEOFF
Lloh187:
	adrp	x1, l_.str.3@PAGE
Lloh188:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh189:
	adrp	x3, l_.str.14@PAGE
Lloh190:
	add	x3, x3, l_.str.14@PAGEOFF
	mov	w2, #172                        ; =0xac
	bl	___assert_rtn
	.loh AdrpAdd	Lloh189, Lloh190
	.loh AdrpAdd	Lloh187, Lloh188
	.loh AdrpAdd	Lloh185, Lloh186
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsCompressed.cold.5
_ReadWeightsCompressed.cold.5:          ; @ReadWeightsCompressed.cold.5
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh191:
	adrp	x0, l___func__.ReadWeightsCompressed@PAGE
Lloh192:
	add	x0, x0, l___func__.ReadWeightsCompressed@PAGEOFF
Lloh193:
	adrp	x1, l_.str.3@PAGE
Lloh194:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh195:
	adrp	x3, l_.str.15@PAGE
Lloh196:
	add	x3, x3, l_.str.15@PAGEOFF
	mov	w2, #177                        ; =0xb1
	bl	___assert_rtn
	.loh AdrpAdd	Lloh195, Lloh196
	.loh AdrpAdd	Lloh193, Lloh194
	.loh AdrpAdd	Lloh191, Lloh192
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsCompressed.cold.6
_ReadWeightsCompressed.cold.6:          ; @ReadWeightsCompressed.cold.6
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh197:
	adrp	x0, l___func__.ReadWeightsCompressed@PAGE
Lloh198:
	add	x0, x0, l___func__.ReadWeightsCompressed@PAGEOFF
Lloh199:
	adrp	x1, l_.str.3@PAGE
Lloh200:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh201:
	adrp	x3, l_.str.16@PAGE
Lloh202:
	add	x3, x3, l_.str.16@PAGEOFF
	mov	w2, #182                        ; =0xb6
	bl	___assert_rtn
	.loh AdrpAdd	Lloh201, Lloh202
	.loh AdrpAdd	Lloh199, Lloh200
	.loh AdrpAdd	Lloh197, Lloh198
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsCompressed.cold.7
_ReadWeightsCompressed.cold.7:          ; @ReadWeightsCompressed.cold.7
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh203:
	adrp	x0, l___func__.ReadWeightsCompressed@PAGE
Lloh204:
	add	x0, x0, l___func__.ReadWeightsCompressed@PAGEOFF
Lloh205:
	adrp	x1, l_.str.3@PAGE
Lloh206:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh207:
	adrp	x3, l_.str.17@PAGE
Lloh208:
	add	x3, x3, l_.str.17@PAGEOFF
	mov	w2, #187                        ; =0xbb
	bl	___assert_rtn
	.loh AdrpAdd	Lloh207, Lloh208
	.loh AdrpAdd	Lloh205, Lloh206
	.loh AdrpAdd	Lloh203, Lloh204
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsCompressed.cold.8
_ReadWeightsCompressed.cold.8:          ; @ReadWeightsCompressed.cold.8
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh209:
	adrp	x0, l___func__.ReadWeightsCompressed@PAGE
Lloh210:
	add	x0, x0, l___func__.ReadWeightsCompressed@PAGEOFF
Lloh211:
	adrp	x1, l_.str.3@PAGE
Lloh212:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh213:
	adrp	x3, l_.str.18@PAGE
Lloh214:
	add	x3, x3, l_.str.18@PAGEOFF
	mov	w2, #192                        ; =0xc0
	bl	___assert_rtn
	.loh AdrpAdd	Lloh213, Lloh214
	.loh AdrpAdd	Lloh211, Lloh212
	.loh AdrpAdd	Lloh209, Lloh210
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsCompressed.cold.9
_ReadWeightsCompressed.cold.9:          ; @ReadWeightsCompressed.cold.9
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
Lloh215:
	adrp	x0, l___func__.ReadWeightsCompressed@PAGE
Lloh216:
	add	x0, x0, l___func__.ReadWeightsCompressed@PAGEOFF
Lloh217:
	adrp	x1, l_.str.3@PAGE
Lloh218:
	add	x1, x1, l_.str.3@PAGEOFF
Lloh219:
	adrp	x3, l_.str.19@PAGE
Lloh220:
	add	x3, x3, l_.str.19@PAGEOFF
	mov	w2, #196                        ; =0xc4
	bl	___assert_rtn
	.loh AdrpAdd	Lloh219, Lloh220
	.loh AdrpAdd	Lloh217, Lloh218
	.loh AdrpAdd	Lloh215, Lloh216
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsCompressed.cold.10
_ReadWeightsCompressed.cold.10:         ; @ReadWeightsCompressed.cold.10
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	mov	x0, #0                          ; =0x0
	bl	_perror
	mov	w0, #1                          ; =0x1
	bl	_exit
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function ReadWeightsCompressed.cold.11
_ReadWeightsCompressed.cold.11:         ; @ReadWeightsCompressed.cold.11
	.cfi_startproc
; %bb.0:
	stp	x29, x30, [sp, #-16]!           ; 16-byte Folded Spill
	mov	x29, sp
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	mov	x0, #0                          ; =0x0
	bl	_perror
	mov	w0, #1                          ; =0x1
	bl	_exit
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"network.quant"

l_.str.1:                               ; @.str.1
	.asciz	"rb"

l_.str.2:                               ; @.str.2
	.asciz	"No weights found.\n"

	.comm	_input_layer_weights_q,20971520,1 ; @input_layer_weights_q
l___func__.ReadWeightsQuantized:        ; @__func__.ReadWeightsQuantized
	.asciz	"ReadWeightsQuantized"

l_.str.3:                               ; @.str.3
	.asciz	"neural_net.c"

l_.str.4:                               ; @.str.4
	.asciz	"items_read == ACCUMULATOR_SIZE * N_FEATURES"

	.comm	_input_layer_bias_q,512,1       ; @input_layer_bias_q
l_.str.5:                               ; @.str.5
	.asciz	"items_read == ACCUMULATOR_SIZE"

	.comm	_hidden_layer_1_weights_q,32768,1 ; @hidden_layer_1_weights_q
l_.str.6:                               ; @.str.6
	.asciz	"items_read == 32 * 2 * ACCUMULATOR_SIZE"

	.comm	_hidden_layer_1_bias_q,128,2    ; @hidden_layer_1_bias_q
l_.str.7:                               ; @.str.7
	.asciz	"items_read == 32"

	.comm	_hidden_layer_2_weights_q,2048,1 ; @hidden_layer_2_weights_q
l_.str.8:                               ; @.str.8
	.asciz	"items_read == 32 * 32"

	.comm	_hidden_layer_2_bias_q,128,2    ; @hidden_layer_2_bias_q
	.comm	_output_layer_weights_q,128,2   ; @output_layer_weights_q
	.comm	_output_layer_bias_q,4,2        ; @output_layer_bias_q
l_.str.9:                               ; @.str.9
	.asciz	"items_read == 1"

l_.str.10:                              ; @.str.10
	.asciz	"Read weights.\n"

l_.str.11:                              ; @.str.11
	.asciz	"network.quant.z"

l___func__.ReadWeightsCompressed:       ; @__func__.ReadWeightsCompressed
	.asciz	"ReadWeightsCompressed"

l_.str.12:                              ; @.str.12
	.asciz	"0 == memcmp(input_layer_weights_q, ptr, sizeof(input_layer_weights_q))"

l_.str.13:                              ; @.str.13
	.asciz	"0 == memcmp(input_layer_bias_q, ptr, sizeof(input_layer_bias_q))"

l_.str.14:                              ; @.str.14
	.asciz	"0 == memcmp(hidden_layer_1_weights_q, ptr, sizeof(hidden_layer_1_weights_q))"

l_.str.15:                              ; @.str.15
	.asciz	"0 == memcmp(hidden_layer_1_bias_q, ptr, sizeof(hidden_layer_1_bias_q))"

l_.str.16:                              ; @.str.16
	.asciz	"0 == memcmp(hidden_layer_2_weights_q, ptr, sizeof(hidden_layer_2_weights_q))"

l_.str.17:                              ; @.str.17
	.asciz	"0 == memcmp(hidden_layer_2_bias_q, ptr, sizeof(hidden_layer_2_bias_q))"

l_.str.18:                              ; @.str.18
	.asciz	"0 == memcmp(output_layer_weights_q, ptr, sizeof(output_layer_weights_q))"

l_.str.19:                              ; @.str.19
	.asciz	"0 == memcmp(output_layer_bias_q, ptr, sizeof(output_layer_bias_q))"

l_.str.20:                              ; @.str.20
	.asciz	"i=%d accumulator_w_q[i]=%d p->accumulator_w_q[i]=%d\n"

l_.str.21:                              ; @.str.21
	.asciz	"i=%d accumulator_b_q[i]=%d p->accumulator_b_q[i]=%d\n"

.subsections_via_symbols
