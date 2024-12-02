	.file	"recog.c"
# GNU C17 (Ubuntu 13.2.0-23ubuntu4) version 13.2.0 (x86_64-linux-gnu)
#	compiled by GNU C version 13.2.0, GMP version 6.3.0, MPFR version 4.2.1, MPC version 1.3.1, isl version isl-0.26-GMP

# GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
# options passed: -mtune=generic -march=x86-64 -O2 -fasynchronous-unwind-tables -fstack-protector-strong -fstack-clash-protection -fcf-protection
	.text
	.p2align 4
	.type	RecognizerKK, @function
RecognizerKK:
.LFB77:
	.cfi_startproc
	endbr64	
# recog.c:109:     *score = 0;
	movl	$0, (%rsi)	#, *score_2(D)
# recog.c:112: }
	xorl	%eax, %eax	#
	ret	
	.cfi_endproc
.LFE77:
	.size	RecognizerKK, .-RecognizerKK
	.globl	__popcountdi2
	.p2align 4
	.type	RecognizerKNKP, @function
RecognizerKNKP:
.LFB82:
	.cfi_startproc
	endbr64	
	pushq	%r13	#
	.cfi_def_cfa_offset 16
	.cfi_offset 13, -16
	pushq	%r12	#
	.cfi_def_cfa_offset 24
	.cfi_offset 12, -24
	movq	%rsi, %r12	# tmp126, score
	pushq	%rbp	#
	.cfi_def_cfa_offset 32
	.cfi_offset 6, -32
	pushq	%rbx	#
	.cfi_def_cfa_offset 40
	.cfi_offset 3, -40
	movq	%rdi, %rbx	# tmp125, p
	subq	$8, %rsp	#,
	.cfi_def_cfa_offset 48
# recog.c:380:     if (p->material_signature[Black] & SIGNATURE_BIT(Knight)) {
	movl	1292(%rdi), %ebp	# p_14(D)->material_signature[1], _2
	andl	$2, %ebp	#, _2
# recog.c:384:     if (CountBits(p->mask[color][Knight]) > 1 ||
	setne	%dl	#, _18
# recog.c:380:     if (p->material_signature[Black] & SIGNATURE_BIT(Knight)) {
	setne	%r13b	#, tmp98
# recog.c:384:     if (CountBits(p->mask[color][Knight]) > 1 ||
	movzbl	%dl, %edx	# _18, _18
	leaq	0(,%rdx,8), %rax	#, tmp102
	subq	%rdx, %rax	# _18, tmp103
	movq	1040(%rdi,%rax,8), %rdi	# p_14(D)->mask[_18][2],
	call	__popcountdi2@PLT	#
	movl	%eax, %edx	#, tmp127
# recog.c:386:         return Useless;
	movl	$4, %eax	#, <retval>
# recog.c:384:     if (CountBits(p->mask[color][Knight]) > 1 ||
	cmpl	$1, %edx	#, tmp127
	jg	.L3	#,
# recog.c:385:         p->mask[OPP(color)][King] & EdgeMask) {
	xorl	%ecx, %ecx	# tmp110
	testl	%ebp, %ebp	# _2
	sete	%cl	#, tmp110
	leaq	0(,%rcx,8), %rdx	#, tmp112
	subq	%rcx, %rdx	# tmp110, tmp113
# recog.c:385:         p->mask[OPP(color)][King] & EdgeMask) {
	movq	1072(%rbx,%rdx,8), %rdx	# p_14(D)->mask[_5][6], p_14(D)->mask[_5][6]
	andq	EdgeMask(%rip), %rdx	# EdgeMask, tmp117
# recog.c:384:     if (CountBits(p->mask[color][Knight]) > 1 ||
	jne	.L3	#,
# recog.c:392:         return UpperBound;
	xorl	%eax, %eax	# <retval>
# recog.c:389:     *score = 0;
	movl	$0, (%r12)	#, *score_15(D)
	movzbl	%r13b, %r13d	# tmp98, tmp98
# recog.c:392:         return UpperBound;
	cmpl	%r13d, 1220(%rbx)	# tmp98, p_14(D)->turn
	sete	%al	#, <retval>
	addl	$1, %eax	#, <retval>
.L3:
# recog.c:396: }
	addq	$8, %rsp	#,
	.cfi_def_cfa_offset 40
	popq	%rbx	#
	.cfi_def_cfa_offset 32
	popq	%rbp	#
	.cfi_def_cfa_offset 24
	popq	%r12	#
	.cfi_def_cfa_offset 16
	popq	%r13	#
	.cfi_def_cfa_offset 8
	ret	
	.cfi_endproc
.LFE82:
	.size	RecognizerKNKP, .-RecognizerKNKP
	.p2align 4
	.type	RecognizerKBKP, @function
RecognizerKBKP:
.LFB81:
	.cfi_startproc
	endbr64	
	pushq	%rbp	#
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	pushq	%rbx	#
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	movq	%rdi, %rbx	# tmp230, p
	subq	$24, %rsp	#,
	.cfi_def_cfa_offset 48
# recog.c:281:     if (p->material_signature[White] && p->material_signature[Black]) {
	movl	1288(%rdi), %eax	# p_94(D)->material_signature[0], _1
# recog.c:281:     if (p->material_signature[White] && p->material_signature[Black]) {
	testl	%eax, %eax	# _1
	jne	.L51	#,
# recog.c:359:             if (!(p->mask[Black][Pawn] & NotAFileMask) &&
	movq	1088(%rdi), %rax	# p_94(D)->mask[1][1], _69
# recog.c:359:             if (!(p->mask[Black][Pawn] & NotAFileMask) &&
	movq	%rax, %rdx	# _69, tmp219
	andq	NotAFileMask(%rip), %rdx	# NotAFileMask, tmp219
# recog.c:359:             if (!(p->mask[Black][Pawn] & NotAFileMask) &&
	je	.L52	#,
.L23:
# recog.c:365:             if (!(p->mask[Black][Pawn] & NotHFileMask) &&
	andq	NotHFileMask(%rip), %rax	# NotHFileMask, _69
# recog.c:322:             return Useless;
	movl	$4, %eax	#, <retval>
# recog.c:365:             if (!(p->mask[Black][Pawn] & NotHFileMask) &&
	jne	.L9	#,
# recog.c:366:                 !(p->mask[Black][Bishop] & WhiteSquaresMask) &&
	movq	1104(%rbx), %rdx	# p_94(D)->mask[1][3], p_94(D)->mask[1][3]
	andq	WhiteSquaresMask(%rip), %rdx	# WhiteSquaresMask, tmp225
# recog.c:365:             if (!(p->mask[Black][Pawn] & NotHFileMask) &&
	jne	.L9	#,
# recog.c:367:                 (p->mask[White][King] & CornerMaskH1)) {
	movq	1072(%rbx), %rdx	# p_94(D)->mask[0][6], p_94(D)->mask[0][6]
	andq	CornerMaskH1(%rip), %rdx	# CornerMaskH1, tmp227
# recog.c:366:                 !(p->mask[Black][Bishop] & WhiteSquaresMask) &&
	je	.L9	#,
.L22:
# recog.c:349:                 *score = 0;
	movl	$0, (%rsi)	#, *score_95(D)
# recog.c:350:                 return ExactScore;
	xorl	%eax, %eax	# <retval>
.L9:
# recog.c:375: }
	addq	$24, %rsp	#,
	.cfi_remember_state
	.cfi_def_cfa_offset 24
	popq	%rbx	#
	.cfi_def_cfa_offset 16
	popq	%rbp	#
	.cfi_def_cfa_offset 8
	ret	
	.p2align 4,,10
	.p2align 3
.L51:
	.cfi_restore_state
# recog.c:281:     if (p->material_signature[White] && p->material_signature[Black]) {
	movl	1292(%rdi), %edx	# p_94(D)->material_signature[1], _2
# recog.c:281:     if (p->material_signature[White] && p->material_signature[Black]) {
	testl	%edx, %edx	# _2
	je	.L11	#,
# recog.c:289:         if (p->material_signature[Black] & SIGNATURE_BIT(Bishop)) {
	testb	$4, %dl	#, _2
	je	.L53	#,
# recog.c:293:         if (p->material_signature[color] & SIGNATURE_BIT(Pawn)) {
	andl	$1, %edx	#, _2
# recog.c:290:             color = Black;
	movl	$1, %ebp	#, color
# recog.c:293:         if (p->material_signature[color] & SIGNATURE_BIT(Pawn)) {
	je	.L14	#,
# recog.c:308:                 if (!(p->mask[Black][Pawn] & NotAFileMask) &&
	movq	1088(%rdi), %rax	# p_94(D)->mask[1][1], _26
# recog.c:308:                 if (!(p->mask[Black][Pawn] & NotAFileMask) &&
	movq	%rax, %rdx	# _26, tmp181
	andq	NotAFileMask(%rip), %rdx	# NotAFileMask, tmp181
# recog.c:308:                 if (!(p->mask[Black][Pawn] & NotAFileMask) &&
	jne	.L20	#,
# recog.c:309:                     !(p->mask[Black][Bishop] & BlackSquaresMask) &&
	movq	1104(%rdi), %rdx	# p_94(D)->mask[1][3], p_94(D)->mask[1][3]
	andq	BlackSquaresMask(%rip), %rdx	# BlackSquaresMask, tmp182
# recog.c:308:                 if (!(p->mask[Black][Pawn] & NotAFileMask) &&
	jne	.L20	#,
# recog.c:310:                     (p->mask[White][King] & CornerMaskA1)) {
	movq	1072(%rdi), %rdx	# p_94(D)->mask[0][6], p_94(D)->mask[0][6]
	andq	CornerMaskA1(%rip), %rdx	# CornerMaskA1, tmp184
# recog.c:309:                     !(p->mask[Black][Bishop] & BlackSquaresMask) &&
	jne	.L50	#,
	.p2align 4,,10
	.p2align 3
.L20:
# recog.c:314:                 if (!(p->mask[Black][Pawn] & NotHFileMask) &&
	andq	NotHFileMask(%rip), %rax	# NotHFileMask, _26
# recog.c:322:             return Useless;
	movl	$4, %eax	#, <retval>
# recog.c:314:                 if (!(p->mask[Black][Pawn] & NotHFileMask) &&
	jne	.L9	#,
# recog.c:315:                     !(p->mask[Black][Bishop] & WhiteSquaresMask) &&
	movq	1104(%rbx), %rdx	# p_94(D)->mask[1][3], p_94(D)->mask[1][3]
	andq	WhiteSquaresMask(%rip), %rdx	# WhiteSquaresMask, tmp187
# recog.c:314:                 if (!(p->mask[Black][Pawn] & NotHFileMask) &&
	jne	.L9	#,
# recog.c:316:                     (p->mask[White][King] & CornerMaskH1)) {
	movq	1072(%rbx), %rdx	# p_94(D)->mask[0][6], p_94(D)->mask[0][6]
	andq	CornerMaskH1(%rip), %rdx	# CornerMaskH1, tmp189
# recog.c:315:                     !(p->mask[Black][Bishop] & WhiteSquaresMask) &&
	je	.L9	#,
.L50:
# recog.c:317:                     *score = 0;
	movl	$0, (%rsi)	#, *score_95(D)
# recog.c:318:                     return (p->turn == Black) ? UpperBound : LowerBound;
	cmpl	$1, 1220(%rbx)	#, p_94(D)->turn
	jne	.L17	#,
	jmp	.L19	#
	.p2align 4,,10
	.p2align 3
.L11:
# recog.c:346:             if (!(p->mask[White][Pawn] & NotAFileMask) &&
	movq	1032(%rdi), %rax	# p_94(D)->mask[0][1], _52
# recog.c:346:             if (!(p->mask[White][Pawn] & NotAFileMask) &&
	movq	%rax, %rdx	# _52, tmp209
	andq	NotAFileMask(%rip), %rdx	# NotAFileMask, tmp209
# recog.c:346:             if (!(p->mask[White][Pawn] & NotAFileMask) &&
	je	.L54	#,
.L21:
# recog.c:352:             if (!(p->mask[White][Pawn] & NotHFileMask) &&
	andq	NotHFileMask(%rip), %rax	# NotHFileMask, _52
# recog.c:322:             return Useless;
	movl	$4, %eax	#, <retval>
# recog.c:352:             if (!(p->mask[White][Pawn] & NotHFileMask) &&
	jne	.L9	#,
# recog.c:353:                 !(p->mask[White][Bishop] & BlackSquaresMask) &&
	movq	1048(%rbx), %rdx	# p_94(D)->mask[0][3], p_94(D)->mask[0][3]
	andq	BlackSquaresMask(%rip), %rdx	# BlackSquaresMask, tmp215
# recog.c:352:             if (!(p->mask[White][Pawn] & NotHFileMask) &&
	jne	.L9	#,
# recog.c:354:                 (p->mask[Black][King] & CornerMaskH8)) {
	movq	1128(%rbx), %rdx	# p_94(D)->mask[1][6], p_94(D)->mask[1][6]
	andq	CornerMaskH8(%rip), %rdx	# CornerMaskH8, tmp217
# recog.c:353:                 !(p->mask[White][Bishop] & BlackSquaresMask) &&
	jne	.L22	#,
	jmp	.L9	#
	.p2align 4,,10
	.p2align 3
.L54:
# recog.c:347:                 !(p->mask[White][Bishop] & WhiteSquaresMask) &&
	movq	1048(%rdi), %rdx	# p_94(D)->mask[0][3], p_94(D)->mask[0][3]
	andq	WhiteSquaresMask(%rip), %rdx	# WhiteSquaresMask, tmp210
# recog.c:346:             if (!(p->mask[White][Pawn] & NotAFileMask) &&
	jne	.L21	#,
# recog.c:348:                 (p->mask[Black][King] & CornerMaskA8)) {
	movq	1128(%rdi), %rdx	# p_94(D)->mask[1][6], p_94(D)->mask[1][6]
	andq	CornerMaskA8(%rip), %rdx	# CornerMaskA8, tmp212
# recog.c:347:                 !(p->mask[White][Bishop] & WhiteSquaresMask) &&
	je	.L21	#,
	jmp	.L22	#
	.p2align 4,,10
	.p2align 3
.L52:
# recog.c:360:                 !(p->mask[Black][Bishop] & BlackSquaresMask) &&
	movq	1104(%rdi), %rdx	# p_94(D)->mask[1][3], p_94(D)->mask[1][3]
	andq	BlackSquaresMask(%rip), %rdx	# BlackSquaresMask, tmp220
# recog.c:359:             if (!(p->mask[Black][Pawn] & NotAFileMask) &&
	jne	.L23	#,
# recog.c:361:                 (p->mask[White][King] & CornerMaskA1)) {
	movq	1072(%rdi), %rdx	# p_94(D)->mask[0][6], p_94(D)->mask[0][6]
	andq	CornerMaskA1(%rip), %rdx	# CornerMaskA1, tmp222
# recog.c:360:                 !(p->mask[Black][Bishop] & BlackSquaresMask) &&
	je	.L23	#,
	jmp	.L22	#
	.p2align 4,,10
	.p2align 3
.L53:
# recog.c:293:         if (p->material_signature[color] & SIGNATURE_BIT(Pawn)) {
	andl	$1, %eax	#, _1
	movl	%eax, %ebp	# _1, color
	je	.L14	#,
# recog.c:295:                 if (!(p->mask[White][Pawn] & NotAFileMask) &&
	movq	1032(%rdi), %rax	# p_94(D)->mask[0][1], _7
# recog.c:295:                 if (!(p->mask[White][Pawn] & NotAFileMask) &&
	movq	%rax, %rdx	# _7, tmp171
	andq	NotAFileMask(%rip), %rdx	# NotAFileMask, tmp171
# recog.c:295:                 if (!(p->mask[White][Pawn] & NotAFileMask) &&
	jne	.L16	#,
# recog.c:296:                     !(p->mask[White][Bishop] & WhiteSquaresMask) &&
	movq	1048(%rdi), %rdx	# p_94(D)->mask[0][3], p_94(D)->mask[0][3]
	andq	WhiteSquaresMask(%rip), %rdx	# WhiteSquaresMask, tmp172
# recog.c:295:                 if (!(p->mask[White][Pawn] & NotAFileMask) &&
	jne	.L16	#,
# recog.c:297:                     (p->mask[Black][King] & CornerMaskA8)) {
	movq	1128(%rdi), %rdx	# p_94(D)->mask[1][6], p_94(D)->mask[1][6]
	andq	CornerMaskA8(%rip), %rdx	# CornerMaskA8, tmp174
# recog.c:296:                     !(p->mask[White][Bishop] & WhiteSquaresMask) &&
	jne	.L48	#,
	.p2align 4,,10
	.p2align 3
.L16:
# recog.c:301:                 if (!(p->mask[White][Pawn] & NotHFileMask) &&
	andq	NotHFileMask(%rip), %rax	# NotHFileMask, _7
# recog.c:322:             return Useless;
	movl	$4, %eax	#, <retval>
# recog.c:301:                 if (!(p->mask[White][Pawn] & NotHFileMask) &&
	jne	.L9	#,
# recog.c:302:                     !(p->mask[White][Bishop] & BlackSquaresMask) &&
	movq	1048(%rbx), %rdx	# p_94(D)->mask[0][3], p_94(D)->mask[0][3]
	andq	BlackSquaresMask(%rip), %rdx	# BlackSquaresMask, tmp177
# recog.c:301:                 if (!(p->mask[White][Pawn] & NotHFileMask) &&
	jne	.L9	#,
# recog.c:303:                     (p->mask[Black][King] & CornerMaskH8)) {
	movq	1128(%rbx), %rdx	# p_94(D)->mask[1][6], p_94(D)->mask[1][6]
	andq	CornerMaskH8(%rip), %rdx	# CornerMaskH8, tmp179
# recog.c:302:                     !(p->mask[White][Bishop] & BlackSquaresMask) &&
	je	.L9	#,
.L48:
# recog.c:298:                     *score = 0;
	movl	$0, (%rsi)	#, *score_95(D)
# recog.c:299:                     return (p->turn == White) ? UpperBound : LowerBound;
	movl	1220(%rbx), %eax	# p_94(D)->turn,
	testl	%eax, %eax	#
	jne	.L17	#,
.L19:
# recog.c:299:                     return (p->turn == White) ? UpperBound : LowerBound;
	movl	$2, %eax	#, <retval>
	jmp	.L9	#
	.p2align 4,,10
	.p2align 3
.L14:
# recog.c:324:             if (CountBits(p->mask[color][Bishop]) > 1 ||
	movslq	%ebp, %rdx	# color, color
	movq	%rsi, 8(%rsp)	# score, %sfp
	leaq	0(,%rdx,8), %rax	#, tmp193
	subq	%rdx, %rax	# color, tmp194
	movq	1048(%rbx,%rax,8), %rdi	# p_94(D)->mask[color_106][3],
	call	__popcountdi2@PLT	#
	movl	%eax, %edx	#, tmp232
# recog.c:322:             return Useless;
	movl	$4, %eax	#, <retval>
# recog.c:324:             if (CountBits(p->mask[color][Bishop]) > 1 ||
	cmpl	$1, %edx	#, tmp232
	jg	.L9	#,
# recog.c:325:                 p->mask[OPP(color)][King] & EdgeMask) {
	movl	%ebp, %edx	# color, tmp199
	xorl	$1, %edx	#, tmp199
# recog.c:325:                 p->mask[OPP(color)][King] & EdgeMask) {
	movslq	%edx, %rdx	# tmp199, tmp200
	leaq	0(,%rdx,8), %rcx	#, tmp202
	subq	%rdx, %rcx	# tmp200, tmp203
# recog.c:325:                 p->mask[OPP(color)][King] & EdgeMask) {
	movq	1072(%rbx,%rcx,8), %rdx	# p_94(D)->mask[_47][6], p_94(D)->mask[_47][6]
	andq	EdgeMask(%rip), %rdx	# EdgeMask, tmp207
# recog.c:324:             if (CountBits(p->mask[color][Bishop]) > 1 ||
	jne	.L9	#,
# recog.c:329:             *score = 0;
	movq	8(%rsp), %rsi	# %sfp, score
	movl	$0, (%rsi)	#, *score_95(D)
# recog.c:331:             if (color == p->turn) {
	cmpl	%ebp, 1220(%rbx)	# color, p_94(D)->turn
	je	.L19	#,
.L17:
# recog.c:299:                     return (p->turn == White) ? UpperBound : LowerBound;
	movl	$1, %eax	#, <retval>
	jmp	.L9	#
	.cfi_endproc
.LFE81:
	.size	RecognizerKBKP, .-RecognizerKBKP
	.p2align 4
	.type	RecognizerKNK, @function
RecognizerKNK:
.LFB80:
	.cfi_startproc
	endbr64	
	pushq	%rbx	#
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
# recog.c:260:     if (p->material_signature[White] && p->material_signature[Black]) {
	movl	1288(%rdi), %edx	# p_9(D)->material_signature[0],
# recog.c:259: static int RecognizerKNK(const struct Position *p, int *score) {
	movq	%rsi, %rbx	# tmp94, score
# recog.c:260:     if (p->material_signature[White] && p->material_signature[Black]) {
	testl	%edx, %edx	#
	jne	.L62	#,
# recog.c:268:             cnt = CountBits(p->mask[Black][Knight]);
	movq	1096(%rdi), %rdi	# p_9(D)->mask[1][2],
	call	__popcountdi2@PLT	#
.L58:
# recog.c:261:         return Useless;
	movl	$4, %edx	#, <retval>
# recog.c:271:         if (cnt < 3) {
	cmpl	$2, %eax	#, cnt
	jg	.L55	#,
# recog.c:272:             *score = 0;
	movl	$0, (%rbx)	#, *score_12(D)
# recog.c:273:             return ExactScore;
	xorl	%edx, %edx	# <retval>
.L55:
# recog.c:278: }
	movl	%edx, %eax	# <retval>,
	popq	%rbx	#
	.cfi_remember_state
	.cfi_def_cfa_offset 8
	ret	
	.p2align 4,,10
	.p2align 3
.L62:
	.cfi_restore_state
# recog.c:260:     if (p->material_signature[White] && p->material_signature[Black]) {
	movl	1292(%rdi), %eax	# p_9(D)->material_signature[1],
# recog.c:261:         return Useless;
	movl	$4, %edx	#, <retval>
# recog.c:260:     if (p->material_signature[White] && p->material_signature[Black]) {
	testl	%eax, %eax	#
	jne	.L55	#,
# recog.c:266:             cnt = CountBits(p->mask[White][Knight]);
	movq	1040(%rdi), %rdi	# p_9(D)->mask[0][2],
	call	__popcountdi2@PLT	#
	jmp	.L58	#
	.cfi_endproc
.LFE80:
	.size	RecognizerKNK, .-RecognizerKNK
	.p2align 4
	.type	RecognizerKBNK, @function
RecognizerKBNK:
.LFB79:
	.cfi_startproc
	endbr64	
	pushq	%r15	#
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	pushq	%r14	#
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13	#
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12	#
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp	#
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	movq	%rsi, %rbp	# tmp223, score
	pushq	%rbx	#
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx	# tmp222, p
	subq	$24, %rsp	#,
	.cfi_def_cfa_offset 80
# recog.c:184:     if (p->material_signature[White] && p->material_signature[Black]) {
	movl	1288(%rdi), %edx	# p_48(D)->material_signature[0],
# recog.c:184:     if (p->material_signature[White] && p->material_signature[Black]) {
	movl	1292(%rdi), %eax	# p_48(D)->material_signature[1], pretmp_147
# recog.c:184:     if (p->material_signature[White] && p->material_signature[Black]) {
	testl	%edx, %edx	#
	je	.L64	#,
# recog.c:184:     if (p->material_signature[White] && p->material_signature[Black]) {
	testl	%eax, %eax	# pretmp_147
	je	.L64	#,
# recog.c:190:         if (CountBits(p->mask[White][0] | p->mask[Black][0]) > 4) {
	movq	1024(%rdi), %rdi	# p_48(D)->mask[0][0], p_48(D)->mask[0][0]
	orq	1080(%rbx), %rdi	# p_48(D)->mask[1][0], tmp146
	call	__popcountdi2@PLT	#
# recog.c:191:             return Useless;
	movl	$4, %edx	#, <retval>
# recog.c:190:         if (CountBits(p->mask[White][0] | p->mask[Black][0]) > 4) {
	cmpl	$4, %eax	#, tmp224
	jg	.L63	#,
# recog.c:194:         if (EdgeMask & (p->mask[White][King] | p->mask[Black][King])) {
	movq	1072(%rbx), %rax	# p_48(D)->mask[0][6], p_48(D)->mask[0][6]
	orq	1128(%rbx), %rax	# p_48(D)->mask[1][6], tmp149
# recog.c:194:         if (EdgeMask & (p->mask[White][King] | p->mask[Black][King])) {
	andq	EdgeMask(%rip), %rax	# EdgeMask, tmp151
# recog.c:194:         if (EdgeMask & (p->mask[White][King] | p->mask[Black][King])) {
	jne	.L63	#,
# recog.c:198:         *score = 0;
	movl	$0, 0(%rbp)	#, *score_49(D)
# recog.c:199:         return ExactScore;
	xorl	%edx, %edx	# <retval>
	jmp	.L63	#
	.p2align 4,,10
	.p2align 3
.L64:
# recog.c:210:         if (p->material_signature[Black]) {
	xorl	%r13d, %r13d	# _75
	testl	%eax, %eax	# pretmp_147
# recog.c:220:             if (p->turn != color || CountBits(atkd) > 1) {
	movl	1220(%rbx), %esi	# p_48(D)->turn, pretmp_146
# recog.c:218:         atkd = p->atkTo[p->kingSq[OPP(color)]] & p->mask[color][0];
	sete	%cl	#, _13
# recog.c:218:         atkd = p->atkTo[p->kingSq[OPP(color)]] & p->mask[color][0];
	setne	%r12b	#, _75
# recog.c:210:         if (p->material_signature[Black]) {
	setne	%r13b	#, _75
# recog.c:218:         atkd = p->atkTo[p->kingSq[OPP(color)]] & p->mask[color][0];
	movzbl	%r12b, %r12d	# _75, _75
# recog.c:218:         atkd = p->atkTo[p->kingSq[OPP(color)]] & p->mask[color][0];
	movzbl	%cl, %ecx	# _13, _13
	movslq	1280(%rbx,%rcx,4), %r15	# p_48(D)->kingSq[_13],
# recog.c:218:         atkd = p->atkTo[p->kingSq[OPP(color)]] & p->mask[color][0];
	leaq	0(,%r12,8), %rax	#, tmp161
	subq	%r12, %rax	# _75, tmp162
# recog.c:218:         atkd = p->atkTo[p->kingSq[OPP(color)]] & p->mask[color][0];
	movq	(%rbx,%r15,8), %rdi	# p_48(D)->atkTo[_14], p_48(D)->atkTo[_14]
# recog.c:218:         atkd = p->atkTo[p->kingSq[OPP(color)]] & p->mask[color][0];
	movq	%r15, %r14	#,
# recog.c:219:         if (atkd) {
	andq	1024(%rbx,%rax,8), %rdi	# p_48(D)->mask[_75][0], atkd
	je	.L66	#,
# recog.c:191:             return Useless;
	movl	$4, %edx	#, <retval>
# recog.c:220:             if (p->turn != color || CountBits(atkd) > 1) {
	cmpl	%esi, %r13d	# pretmp_146, _75
	jne	.L63	#,
	movl	%edx, 12(%rsp)	# <retval>, %sfp
# recog.c:220:             if (p->turn != color || CountBits(atkd) > 1) {
	call	__popcountdi2@PLT	#
# recog.c:220:             if (p->turn != color || CountBits(atkd) > 1) {
	movl	$4, %edx	#, <retval>
	cmpl	$1, %eax	#, tmp225
	jne	.L63	#,
# recog.c:231:             (KingDist(p->kingSq[White], p->kingSq[Black]) == 2)) {
	movl	1284(%rbx), %esi	# p_48(D)->kingSq[1], pretmp_82
# recog.c:231:             (KingDist(p->kingSq[White], p->kingSq[Black]) == 2)) {
	movl	1280(%rbx), %edx	# p_48(D)->kingSq[0], pretmp_86
# inline.h:9:     int file_dist = ABS((sq1 & 7) - (sq2 & 7));
	movl	%esi, %eax	# pretmp_82, tmp169
	movl	%edx, %ecx	# pretmp_86, tmp168
	andl	$7, %eax	#, tmp169
	andl	$7, %ecx	#, tmp168
	subl	%eax, %ecx	# tmp169, tmp170
# inline.h:9:     int file_dist = ABS((sq1 & 7) - (sq2 & 7));
	movl	%ecx, %eax	# tmp170, tmp228
	negl	%eax	# tmp228
	cmovs	%ecx, %eax	# tmp228,, tmp170, tmp171
# inline.h:10:     int rank_dist = ABS((sq1 >> 3) - (sq2 >> 3));
	sarl	$3, %edx	#, tmp172
	sarl	$3, %esi	#, tmp173
	subl	%esi, %edx	# tmp173, tmp174
# inline.h:10:     int rank_dist = ABS((sq1 >> 3) - (sq2 >> 3));
	movl	%edx, %ecx	# tmp174, tmp229
	negl	%ecx	# tmp229
	cmovns	%ecx, %edx	# tmp229,, tmp175
# inline.h:12:     return MAX(file_dist, rank_dist);
	cmpl	%edx, %eax	# tmp175, tmp171
	cmovl	%edx, %eax	# tmp171,, tmp175, prephitmp_123
.L67:
# recog.c:239:         if (p->mask[color][Bishop] & BlackSquaresMask) {
	leaq	0(,%r12,8), %rdx	#, tmp195
# recog.c:208:         int sqx = 0;
	movl	$0, %ecx	#, sqx
# recog.c:239:         if (p->mask[color][Bishop] & BlackSquaresMask) {
	subq	%r12, %rdx	# _75, tmp196
	movq	1048(%rbx,%rdx,8), %rdx	# p_48(D)->mask[_75][3], _25
# recog.c:239:         if (p->mask[color][Bishop] & BlackSquaresMask) {
	movq	%rdx, %rsi	# _25, tmp200
	andq	BlackSquaresMask(%rip), %rsi	# BlackSquaresMask, tmp200
# recog.c:239:         if (p->mask[color][Bishop] & BlackSquaresMask) {
	je	.L68	#,
# recog.c:240:             sqx = KBNKTab[p->kingSq[OPP(color)]];
	leaq	KBNKTab(%rip), %rcx	#, tmp201
	movl	(%rcx,%r15,4), %ecx	# KBNKTab[_14], sqx
.L68:
# recog.c:243:         if (p->mask[color][Bishop] & WhiteSquaresMask) {
	andq	WhiteSquaresMask(%rip), %rdx	# WhiteSquaresMask, tmp203
# recog.c:243:         if (p->mask[color][Bishop] & WhiteSquaresMask) {
	je	.L69	#,
# recog.c:244:             sqx = KBNKTab[7 ^ p->kingSq[OPP(color)]];
	xorl	$7, %r14d	#, tmp205
# recog.c:244:             sqx = KBNKTab[7 ^ p->kingSq[OPP(color)]];
	leaq	KBNKTab(%rip), %rdx	#, tmp204
	movslq	%r14d, %r14	# tmp205, tmp206
	movl	(%rdx,%r14,4), %ecx	# KBNKTab[_30], sqx
.L69:
# recog.c:247:         *score = p->material[color] + 3 * Value[Pawn] + sqx -
	movl	4+Value(%rip), %edx	# Value[1], Value[1]
# recog.c:248:                  125 * KingDist(p->kingSq[White], p->kingSq[Black]);
	imull	$-125, %eax, %eax	#, prephitmp_123, tmp217
# recog.c:247:         *score = p->material[color] + 3 * Value[Pawn] + sqx -
	leal	(%rdx,%rdx,2), %edx	#, tmp211
# recog.c:247:         *score = p->material[color] + 3 * Value[Pawn] + sqx -
	addl	1240(%rbx,%r12,4), %edx	# p_48(D)->material[_75], tmp214
# recog.c:247:         *score = p->material[color] + 3 * Value[Pawn] + sqx -
	addl	%ecx, %edx	# sqx, tmp216
# recog.c:247:         *score = p->material[color] + 3 * Value[Pawn] + sqx -
	addl	%edx, %eax	# tmp216, _39
# recog.c:255:         return LowerBound;
	movl	$1, %edx	#, <retval>
# recog.c:247:         *score = p->material[color] + 3 * Value[Pawn] + sqx -
	movl	%eax, 0(%rbp)	# _39, *score_49(D)
# recog.c:250:         if (p->turn != color) {
	cmpl	%r13d, 1220(%rbx)	# _75, p_48(D)->turn
	je	.L63	#,
# recog.c:251:             *score = -*score;
	negl	%eax	# tmp218
# recog.c:252:             return UpperBound;
	movl	$2, %edx	#, <retval>
# recog.c:251:             *score = -*score;
	movl	%eax, 0(%rbp)	# tmp218, *score_49(D)
.L63:
# recog.c:257: }
	addq	$24, %rsp	#,
	.cfi_remember_state
	.cfi_def_cfa_offset 56
	movl	%edx, %eax	# <retval>,
	popq	%rbx	#
	.cfi_def_cfa_offset 48
	popq	%rbp	#
	.cfi_def_cfa_offset 40
	popq	%r12	#
	.cfi_def_cfa_offset 32
	popq	%r13	#
	.cfi_def_cfa_offset 24
	popq	%r14	#
	.cfi_def_cfa_offset 16
	popq	%r15	#
	.cfi_def_cfa_offset 8
	ret	
	.p2align 4,,10
	.p2align 3
.L66:
	.cfi_restore_state
# recog.c:231:             (KingDist(p->kingSq[White], p->kingSq[Black]) == 2)) {
	movl	1284(%rbx), %r8d	# p_48(D)->kingSq[1], pretmp_135
# recog.c:231:             (KingDist(p->kingSq[White], p->kingSq[Black]) == 2)) {
	movl	1280(%rbx), %edx	# p_48(D)->kingSq[0], pretmp_136
# inline.h:9:     int file_dist = ABS((sq1 & 7) - (sq2 & 7));
	movl	%r8d, %eax	# pretmp_135, tmp177
	movl	%edx, %edi	# pretmp_136, tmp176
	andl	$7, %eax	#, tmp177
	andl	$7, %edi	#, tmp176
	subl	%eax, %edi	# tmp177, tmp178
# inline.h:9:     int file_dist = ABS((sq1 & 7) - (sq2 & 7));
	movl	%edi, %eax	# tmp178, tmp226
	negl	%eax	# tmp226
	cmovs	%edi, %eax	# tmp226,, tmp178, tmp179
# inline.h:10:     int rank_dist = ABS((sq1 >> 3) - (sq2 >> 3));
	sarl	$3, %edx	#, tmp180
	sarl	$3, %r8d	#, tmp181
	subl	%r8d, %edx	# tmp181, tmp182
# inline.h:10:     int rank_dist = ABS((sq1 >> 3) - (sq2 >> 3));
	movl	%edx, %edi	# tmp182, tmp227
	negl	%edi	# tmp227
	cmovns	%edi, %edx	# tmp227,, tmp183
# inline.h:12:     return MAX(file_dist, rank_dist);
	cmpl	%edx, %eax	# tmp183, tmp179
	cmovl	%edx, %eax	# tmp179,, tmp183, prephitmp_123
# recog.c:230:         if (p->turn != color && (p->mask[OPP(color)][King] & EdgeMask) &&
	cmpl	%esi, %r13d	# pretmp_146, _75
	je	.L67	#,
# recog.c:230:         if (p->turn != color && (p->mask[OPP(color)][King] & EdgeMask) &&
	leaq	0(,%rcx,8), %rdx	#, tmp186
	subq	%rcx, %rdx	# _13, tmp187
# recog.c:230:         if (p->turn != color && (p->mask[OPP(color)][King] & EdgeMask) &&
	movq	1072(%rbx,%rdx,8), %rdx	# p_48(D)->mask[_13][6], p_48(D)->mask[_13][6]
	andq	EdgeMask(%rip), %rdx	# EdgeMask, tmp191
# recog.c:230:         if (p->turn != color && (p->mask[OPP(color)][King] & EdgeMask) &&
	je	.L67	#,
# recog.c:230:         if (p->turn != color && (p->mask[OPP(color)][King] & EdgeMask) &&
	cmpl	$2, %eax	#, prephitmp_123
	jne	.L67	#,
# recog.c:191:             return Useless;
	movl	$4, %edx	#, <retval>
	jmp	.L63	#
	.cfi_endproc
.LFE79:
	.size	RecognizerKBNK, .-RecognizerKBNK
	.p2align 4
	.type	RecognizerKBK, @function
RecognizerKBK:
.LFB78:
	.cfi_startproc
	endbr64	
	pushq	%r15	#
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
# recog.c:118:     if (p->material_signature[Black]) {
	xorl	%eax, %eax	# tmp133
# recog.c:114: static int RecognizerKBK(const struct Position *p, int *score) {
	pushq	%r14	#
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13	#
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	movq	%rsi, %r13	# tmp198, score
	pushq	%r12	#
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp	#
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx	#
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	movq	%rdi, %rbx	# tmp197, p
	subq	$24, %rsp	#,
	.cfi_def_cfa_offset 80
# recog.c:118:     if (p->material_signature[Black]) {
	movl	1292(%rdi), %r12d	# p_35(D)->material_signature[1], _1
# recog.c:118:     if (p->material_signature[Black]) {
	testl	%r12d, %r12d	# _1
# recog.c:122:     pcs = p->mask[color][Bishop];
	setne	%r14b	#, _68
# recog.c:118:     if (p->material_signature[Black]) {
	setne	%al	#, tmp133
# recog.c:122:     pcs = p->mask[color][Bishop];
	movzbl	%r14b, %r14d	# _68, _68
# recog.c:118:     if (p->material_signature[Black]) {
	movl	%eax, 12(%rsp)	# tmp133, %sfp
# recog.c:122:     pcs = p->mask[color][Bishop];
	leaq	0(,%r14,8), %rax	#, tmp137
	subq	%r14, %rax	# _68, tmp138
	leaq	(%rdi,%rax,8), %r15	#, tmp140
	movq	1048(%r15), %rbp	# p_35(D)->mask[_68][3], pcs
# recog.c:128:     if (CountBits(pcs) < 2) {
	movq	%rbp, %rdi	# pcs,
	call	__popcountdi2@PLT	#
# recog.c:128:     if (CountBits(pcs) < 2) {
	cmpl	$1, %eax	#, tmp199
	jle	.L91	#,
# recog.c:137:     if (!((pcs & WhiteSquaresMask) && (pcs & BlackSquaresMask))) {
	movq	%rbp, %rax	# pcs, tmp143
	andq	WhiteSquaresMask(%rip), %rax	# WhiteSquaresMask, tmp143
# recog.c:137:     if (!((pcs & WhiteSquaresMask) && (pcs & BlackSquaresMask))) {
	je	.L91	#,
# recog.c:137:     if (!((pcs & WhiteSquaresMask) && (pcs & BlackSquaresMask))) {
	andq	BlackSquaresMask(%rip), %rbp	# BlackSquaresMask, tmp144
# recog.c:137:     if (!((pcs & WhiteSquaresMask) && (pcs & BlackSquaresMask))) {
	je	.L91	#,
# recog.c:146:     if (p->atkTo[p->kingSq[OPP(color)]] & p->mask[color][0]) {
	xorl	%r8d, %r8d	# _7
	testl	%r12d, %r12d	# _1
# recog.c:147:         return Useless;
	movl	$4, %edx	#, <retval>
# recog.c:146:     if (p->atkTo[p->kingSq[OPP(color)]] & p->mask[color][0]) {
	sete	%r8b	#, _7
	movslq	1280(%rbx,%r8,4), %rax	# p_35(D)->kingSq[_7],
	movq	%rax, %rsi	#,
# recog.c:146:     if (p->atkTo[p->kingSq[OPP(color)]] & p->mask[color][0]) {
	movq	(%rbx,%rax,8), %rax	# p_35(D)->atkTo[_8], p_35(D)->atkTo[_8]
	andq	1024(%r15), %rax	# p_35(D)->mask[_68][0], tmp157
# recog.c:146:     if (p->atkTo[p->kingSq[OPP(color)]] & p->mask[color][0]) {
	jne	.L88	#,
# recog.c:156:         (KingDist(p->kingSq[White], p->kingSq[Black]) == 2)) {
	movl	1284(%rbx), %r9d	# p_35(D)->kingSq[1], pretmp_113
# recog.c:156:         (KingDist(p->kingSq[White], p->kingSq[Black]) == 2)) {
	movl	1280(%rbx), %eax	# p_35(D)->kingSq[0], pretmp_114
# inline.h:9:     int file_dist = ABS((sq1 & 7) - (sq2 & 7));
	movl	%r9d, %ecx	# pretmp_113, tmp160
	movl	%eax, %edi	# pretmp_114, tmp159
	andl	$7, %ecx	#, tmp160
	andl	$7, %edi	#, tmp159
	subl	%ecx, %edi	# tmp160, tmp161
# inline.h:9:     int file_dist = ABS((sq1 & 7) - (sq2 & 7));
	movl	%edi, %ecx	# tmp161, tmp200
	negl	%ecx	# tmp200
	cmovs	%edi, %ecx	# tmp200,, tmp161, tmp162
# inline.h:10:     int rank_dist = ABS((sq1 >> 3) - (sq2 >> 3));
	sarl	$3, %eax	#, tmp163
	sarl	$3, %r9d	#, tmp164
	subl	%r9d, %eax	# tmp164, tmp165
# inline.h:10:     int rank_dist = ABS((sq1 >> 3) - (sq2 >> 3));
	movl	%eax, %edi	# tmp165, tmp201
	negl	%edi	# tmp201
	cmovns	%edi, %eax	# tmp201,, tmp166
# inline.h:12:     return MAX(file_dist, rank_dist);
	cmpl	%eax, %ecx	# tmp166, tmp162
	cmovl	%eax, %ecx	# tmp162,, tmp166, _123
# recog.c:155:     if (p->turn != color && (p->mask[OPP(color)][King] & EdgeMask) &&
	movl	12(%rsp), %eax	# %sfp, tmp133
	cmpl	%eax, 1220(%rbx)	# tmp133, p_35(D)->turn
	je	.L92	#,
# recog.c:155:     if (p->turn != color && (p->mask[OPP(color)][King] & EdgeMask) &&
	leaq	0(,%r8,8), %rax	#, tmp169
	subq	%r8, %rax	# _7, tmp170
# recog.c:155:     if (p->turn != color && (p->mask[OPP(color)][King] & EdgeMask) &&
	movq	1072(%rbx,%rax,8), %rax	# p_35(D)->mask[_7][6], p_35(D)->mask[_7][6]
	andq	EdgeMask(%rip), %rax	# EdgeMask, tmp174
# recog.c:155:     if (p->turn != color && (p->mask[OPP(color)][King] & EdgeMask) &&
	je	.L92	#,
# recog.c:155:     if (p->turn != color && (p->mask[OPP(color)][King] & EdgeMask) &&
	cmpl	$2, %ecx	#, _123
	je	.L88	#,
.L92:
# inline.h:42:     int filedist = MIN(sq & 7, 7 - (sq & 7));
	movl	%esi, %eax	# _8, tmp178
# inline.h:43:     int rankdist = MIN(sq >> 3, 7 - (sq >> 3));
	movl	%esi, %edi	# _8, _64
# inline.h:42:     int filedist = MIN(sq & 7, 7 - (sq & 7));
	andl	$7, %esi	#, tmp180
# inline.h:43:     int rankdist = MIN(sq >> 3, 7 - (sq >> 3));
	movl	$7, %edx	#, tmp183
# inline.h:42:     int filedist = MIN(sq & 7, 7 - (sq & 7));
	notl	%eax	# tmp178
# inline.h:43:     int rankdist = MIN(sq >> 3, 7 - (sq >> 3));
	sarl	$3, %edi	#, _64
# inline.h:42:     int filedist = MIN(sq & 7, 7 - (sq & 7));
	andl	$7, %eax	#, tmp179
# inline.h:42:     int filedist = MIN(sq & 7, 7 - (sq & 7));
	cmpl	%esi, %eax	# tmp180, tmp179
	cmovg	%esi, %eax	# tmp179,, tmp180, filedist
# inline.h:43:     int rankdist = MIN(sq >> 3, 7 - (sq >> 3));
	subl	%edi, %edx	# _64, tmp182
# recog.c:164:     *score = p->material[color] + 2 * Value[Pawn] -
	movl	4+Value(%rip), %esi	# Value[1], Value[1]
# inline.h:43:     int rankdist = MIN(sq >> 3, 7 - (sq >> 3));
	cmpl	%edi, %edx	# _64, tmp182
	cmovg	%edi, %edx	# tmp182,, _64, rankdist
# inline.h:45:     return MAX(filedist, rankdist);
	cmpl	%edx, %eax	# rankdist, filedist
	cmovl	%edx, %eax	# filedist,, rankdist, tmp176
# recog.c:164:     *score = p->material[color] + 2 * Value[Pawn] -
	movl	1240(%rbx,%r14,4), %edx	# p_35(D)->material[_68], p_35(D)->material[_68]
# recog.c:166:              125 * KingDist(p->kingSq[White], p->kingSq[Black]);
	imull	$-125, %ecx, %ecx	#, _123, tmp193
# recog.c:165:              250 * EdgeDist(p->kingSq[OPP(color)]) -
	imull	$-250, %eax, %eax	#, tmp176, tmp184
# recog.c:164:     *score = p->material[color] + 2 * Value[Pawn] -
	leal	(%rdx,%rsi,2), %edx	#, tmp190
# recog.c:164:     *score = p->material[color] + 2 * Value[Pawn] -
	addl	%edx, %eax	# tmp190, tmp192
# recog.c:173:     return LowerBound;
	movl	$1, %edx	#, <retval>
# recog.c:165:              250 * EdgeDist(p->kingSq[OPP(color)]) -
	addl	%ecx, %eax	# tmp193, _28
# recog.c:168:     if (p->turn != color) {
	movl	12(%rsp), %ecx	# %sfp, tmp133
# recog.c:164:     *score = p->material[color] + 2 * Value[Pawn] -
	movl	%eax, 0(%r13)	# _28, *score_40(D)
# recog.c:168:     if (p->turn != color) {
	cmpl	%ecx, 1220(%rbx)	# tmp133, p_35(D)->turn
	je	.L88	#,
# recog.c:169:         *score = -*score;
	negl	%eax	# tmp194
# recog.c:170:         return UpperBound;
	movl	$2, %edx	#, <retval>
# recog.c:169:         *score = -*score;
	movl	%eax, 0(%r13)	# tmp194, *score_40(D)
# recog.c:170:         return UpperBound;
	jmp	.L88	#
	.p2align 4,,10
	.p2align 3
.L91:
# recog.c:129:         *score = 0;
	movl	$0, 0(%r13)	#, *score_40(D)
# recog.c:130:         return ExactScore;
	xorl	%edx, %edx	# <retval>
.L88:
# recog.c:174: }
	addq	$24, %rsp	#,
	.cfi_def_cfa_offset 56
	movl	%edx, %eax	# <retval>,
	popq	%rbx	#
	.cfi_def_cfa_offset 48
	popq	%rbp	#
	.cfi_def_cfa_offset 40
	popq	%r12	#
	.cfi_def_cfa_offset 32
	popq	%r13	#
	.cfi_def_cfa_offset 24
	popq	%r14	#
	.cfi_def_cfa_offset 16
	popq	%r15	#
	.cfi_def_cfa_offset 8
	ret	
	.cfi_endproc
.LFE78:
	.size	RecognizerKBK, .-RecognizerKBK
	.p2align 4
	.globl	RecogInit
	.type	RecogInit, @function
RecogInit:
.LFB75:
	.cfi_startproc
	endbr64	
	leaq	RecognizerKBKP(%rip), %rax	#, _150
# recog.c:71:         Recognizers[i] = NULL;
	leaq	Recognizers(%rip), %rdx	#, tmp90
	movl	$64, %ecx	#, tmp93
	movq	%rax, %xmm1	# _150, _150
	movq	%rdx, %rdi	# tmp90, tmp90
	xorl	%eax, %eax	# tmp92
	rep stosq
# recog.c:75:         RecognizerAvailable[i] = 0;
	leaq	20+RecognizerAvailable(%rip), %rdi	#, tmp101
# recog.c:57:     Recognizers[CALCULATE_INDEX(white_sig, black_sig)] = funct;
	movdqa	%xmm1, %xmm0	# _150, tmp111
# recog.c:75:         RecognizerAvailable[i] = 0;
	leaq	12+RecognizerAvailable(%rip), %rcx	#, tmp94
	andq	$-8, %rdi	#, tmp101
# recog.c:57:     Recognizers[CALCULATE_INDEX(white_sig, black_sig)] = funct;
	leaq	RecognizerKBNK(%rip), %rdx	#, tmp107
# recog.c:75:         RecognizerAvailable[i] = 0;
	movq	$0, 12+RecognizerAvailable(%rip)	#,
# recog.c:57:     Recognizers[CALCULATE_INDEX(white_sig, black_sig)] = funct;
	leaq	RecognizerKBK(%rip), %rsi	#, tmp116
# recog.c:75:         RecognizerAvailable[i] = 0;
	subq	%rdi, %rcx	# tmp101, tmp95
# recog.c:57:     Recognizers[CALCULATE_INDEX(white_sig, black_sig)] = funct;
	movq	%rdx, %xmm2	# tmp107, tmp107
# recog.c:75:         RecognizerAvailable[i] = 0;
	movq	$0, 120+RecognizerAvailable(%rip)	#,
	addl	$116, %ecx	#, tmp96
# recog.c:57:     Recognizers[CALCULATE_INDEX(white_sig, black_sig)] = funct;
	punpcklqdq	%xmm2, %xmm0	# tmp107, tmp111
# recog.c:75:         RecognizerAvailable[i] = 0;
	shrl	$3, %ecx	#,
	rep stosq
# recog.c:57:     Recognizers[CALCULATE_INDEX(white_sig, black_sig)] = funct;
	movq	%rdx, 48+Recognizers(%rip)	# tmp107, Recognizers[6]
# recog.c:59:     RecognizerAvailable[white_sig] |= (1 << black_sig);
	movq	.LC0(%rip), %rdx	#, tmp118
# recog.c:57:     Recognizers[CALCULATE_INDEX(white_sig, black_sig)] = funct;
	leaq	RecognizerKK(%rip), %rax	#, tmp129
	movups	%xmm0, 296+Recognizers(%rip)	# tmp111, MEM <vector(2) long unsigned int> [(int (*<Tb5e>) (const struct Position *, int *) *)&Recognizers + 296B]
	movq	%rsi, %xmm0	# tmp116, tmp116
	punpcklqdq	%xmm1, %xmm0	# _150, tmp115
	movq	%rax, Recognizers(%rip)	# tmp129, Recognizers[0]
	leaq	RecognizerKNK(%rip), %rax	#, tmp110
# recog.c:59:     RecognizerAvailable[white_sig] |= (1 << black_sig);
	movq	%rdx, 16+RecognizerAvailable(%rip)	# tmp118, MEM <vector(2) int> [(int *)&RecognizerAvailable + 16B]
# recog.c:57:     Recognizers[CALCULATE_INDEX(white_sig, black_sig)] = funct;
	leaq	RecognizerKNKP(%rip), %rdx	#, tmp121
	movq	%rax, 16+Recognizers(%rip)	# tmp110, Recognizers[2]
	movq	%rdx, %xmm3	# tmp121, tmp121
	movaps	%xmm0, 32+Recognizers(%rip)	# tmp115, MEM <vector(2) long unsigned int> [(int (*<Tb5e>) (const struct Position *, int *) *)&Recognizers + 32B]
	movq	%rax, %xmm0	# tmp110, tmp120
# recog.c:60:     RecognizerAvailable[black_sig] |= (1 << white_sig);
	movq	.LC1(%rip), %rax	#, tmp125
# recog.c:59:     RecognizerAvailable[white_sig] |= (1 << black_sig);
	movl	$1, 24+RecognizerAvailable(%rip)	#, RecognizerAvailable[6]
# recog.c:57:     Recognizers[CALCULATE_INDEX(white_sig, black_sig)] = funct;
	punpcklqdq	%xmm3, %xmm0	# tmp121, tmp120
# recog.c:59:     RecognizerAvailable[white_sig] |= (1 << black_sig);
	movl	$23, 8+RecognizerAvailable(%rip)	#, RecognizerAvailable[2]
# recog.c:60:     RecognizerAvailable[black_sig] |= (1 << white_sig);
	movq	%rax, RecognizerAvailable(%rip)	# tmp125, MEM <vector(2) int> [(int *)&RecognizerAvailable]
# recog.c:57:     Recognizers[CALCULATE_INDEX(white_sig, black_sig)] = funct;
	movaps	%xmm0, 272+Recognizers(%rip)	# tmp120, MEM <vector(2) long unsigned int> [(int (*<Tb5e>) (const struct Position *, int *) *)&Recognizers + 272B]
# recog.c:93: }
	ret	
	.cfi_endproc
.LFE75:
	.size	RecogInit, .-RecogInit
	.p2align 4
	.globl	ProbeRecognizer
	.type	ProbeRecognizer, @function
ProbeRecognizer:
.LFB76:
	.cfi_startproc
	endbr64	
# recog.c:96:     int index = RECOGNIZER_INDEX(p);
	movl	1292(%rdi), %ecx	# p_12(D)->material_signature[1], _2
	movslq	1288(%rdi), %rdx	# p_12(D)->material_signature[0],
# recog.c:95: int ProbeRecognizer(const struct Position *p, int *score) {
	movq	%rsi, %r8	# tmp116, score
# recog.c:96:     int index = RECOGNIZER_INDEX(p);
	testl	%ecx, %ecx	# _2
	setne	%sil	#, tmp100
	xorl	%eax, %eax	# tmp103
	testl	%edx, %edx	# _1
	setne	%al	#, tmp103
	andl	%esi, %eax	# tmp100, tmp104
# recog.c:96:     int index = RECOGNIZER_INDEX(p);
	movl	%edx, %esi	# _1, tmp106
	orl	%ecx, %esi	# _2, tmp106
# recog.c:96:     int index = RECOGNIZER_INDEX(p);
	sall	$5, %eax	#, tmp105
# recog.c:96:     int index = RECOGNIZER_INDEX(p);
	addl	%esi, %eax	# tmp106, index
# recog.c:97:     RECOGNIZER *rec = Recognizers[index];
	leaq	Recognizers(%rip), %rsi	#, tmp98
	cltq
	movq	(%rsi,%rax,8), %rax	# Recognizers[index_13], rec
# recog.c:98:     if (rec != NULL) {
	testq	%rax, %rax	# rec
	je	.L107	#,
# recog.c:99:         if (RecognizerAvailable[p->material_signature[White]] &
	leaq	RecognizerAvailable(%rip), %rsi	#, tmp109
	movl	(%rsi,%rdx,4), %edx	# RecognizerAvailable[_1], RecognizerAvailable[_1]
# recog.c:99:         if (RecognizerAvailable[p->material_signature[White]] &
	btl	%ecx, %edx	# _2, RecognizerAvailable[_1]
	jc	.L113	#,
.L107:
# recog.c:106: }
	movl	$4, %eax	#,
	ret	
	.p2align 4,,10
	.p2align 3
.L113:
# recog.c:101:             return rec(p, score);
	movq	%r8, %rsi	# score,
	jmp	*%rax	# rec
	.cfi_endproc
.LFE76:
	.size	ProbeRecognizer, .-ProbeRecognizer
	.section	.rodata
	.align 32
	.type	KBNKTab, @object
	.size	KBNKTab, 256
KBNKTab:
	.long	500
	.long	450
	.long	425
	.long	400
	.long	375
	.long	350
	.long	325
	.long	300
	.long	450
	.long	300
	.long	300
	.long	300
	.long	300
	.long	300
	.long	300
	.long	325
	.long	425
	.long	300
	.long	100
	.long	100
	.long	100
	.long	100
	.long	300
	.long	350
	.long	400
	.long	300
	.long	100
	.long	0
	.long	0
	.long	100
	.long	300
	.long	375
	.long	375
	.long	300
	.long	100
	.long	0
	.long	0
	.long	100
	.long	300
	.long	400
	.long	350
	.long	300
	.long	100
	.long	100
	.long	100
	.long	100
	.long	300
	.long	425
	.long	325
	.long	300
	.long	300
	.long	300
	.long	300
	.long	300
	.long	300
	.long	450
	.long	300
	.long	325
	.long	350
	.long	375
	.long	400
	.long	425
	.long	450
	.long	500
	.local	RecognizerAvailable
	.comm	RecognizerAvailable,128,32
	.local	Recognizers
	.comm	Recognizers,512,32
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC0:
	.long	7
	.long	3
	.align 8
.LC1:
	.long	117
	.long	52
	.ident	"GCC: (Ubuntu 13.2.0-23ubuntu4) 13.2.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
