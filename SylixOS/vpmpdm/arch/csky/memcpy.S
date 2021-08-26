/*
 * Copyright (C) 2017 Hangzhou C-SKY Microsystems co.,ltd.
 *
 * Licensed under the LGPL v2.1 or later, see the file COPYING.LIB
 * in this tarball.
 */
#include "macro.S"
/*
 * memcpy implement.
 */
	.text
	.align 4
	.type   __memcpy, @function
__memcpy:
	mov	r9, r0
	/* Copy from front to back.  */
	/* Test if dest or src is not 4 bytes aligned.  */
	or	r4, r0, r1
	andi	r4, r4, 3
	cmpnei	r4, 0
	movi	r5, 0
	bf	.L_dest_aligned
	/* Test if len less than 4 bytes.  */
	cmplti	r2, 4
	mov	r5, r2
	/* copy all by byte.  */
	bt	.L_copy_by_byte
	/* Test if dest and src both are not 4 bytes aligned.  */
	xor	r4, r0, r1
	andi	r4, r4, 3
	cmpnei	r4, 0
	mov	r5, r2
	/* copy all by byte.  */
	bt	.L_copy_by_byte
	/* copy unaligned part by byte.  */
	andi	r5, r0, 3
	movi	r4, 4
	subu	r5, r4, r5
	bf	.L_copy_by_byte
	/* 4 bytes aligned.  */
.L_dest_aligned:
	subu	r2, r5
	bez	r2, .L_return
	/* If dest is aligned, then copy.  */
	zext	r7, r2, 31, 4
	/* Test if len less than 16 bytes.  */
	bez	r7, .L_len_less_16bytes
	movi	r8, 0

	LABLE_ALIGN
.L_len_larger_16bytes:
#if defined(__CSKY_VDSPV2__)
	vldx.8	vr0, (r1), r8
	PRE_BNEZAD (r7)
	addi	r1, 16
	vstx.8	vr0, (r0), r8
	addi	r0, 16
#elif defined(__CK860__)
	ldw	r6, (r1, 0)
	stw	r6, (r0, 0)
	ldw	r6, (r1, 4)
	stw	r6, (r0, 4)
	ldw	r6, (r1, 8)
	stw	r6, (r0, 8)
	ldw	r6, (r1, 12)
	addi	r1, 16
	stw	r6, (r0, 12)
	addi	r0, 16
#else
	ldw	r20, (r1, 0)
	ldw	r21, (r1, 4)
	ldw	r22, (r1, 8)
	ldw	r23, (r1, 12)
	stw	r20, (r0, 0)
	stw	r21, (r0, 4)
	stw	r22, (r0, 8)
	stw	r23, (r0, 12)
	PRE_BNEZAD (r7)
	addi	r1, 16
	addi	r0, 16
#endif
	BNEZAD (r7, .L_len_larger_16bytes)

.L_len_less_16bytes:
	zext	r7, r2, 3, 2
	zext	r5, r2, 1, 0
	bez	r7, .L_copy_by_byte_to_end
.L_len_less_16bytes_loop:
	ldw	r6, (r1, 0)
	PRE_BNEZAD (r7)
	addi	r1, 4
	stw	r6, (r0, 0)
	addi	r0, 4
	BNEZAD (r7, .L_len_less_16bytes_loop)

.L_copy_by_byte_to_end:
	mov	r7, r5
	cmpnei	r7, 0
	bt	.L_copy_by_byte_loop
	bf	.L_return
	/* copy all by byte.  */
	/* copy unaligned part by byte.  */
.L_copy_by_byte:
	mov	r7, r5
	bez	r7, .L_dest_aligned
.L_copy_by_byte_loop:
	ldb	r6, (r1, 0)
	PRE_BNEZAD (r7)
	addi	r1, 1
	stb	r6, (r0, 0)
	addi	r0, 1
	BNEZAD (r7, .L_copy_by_byte_loop)
.L_return:
	mov	r0, r9
	pop	r4-r11
	rts

	.size __memcpy,.-__memcpy

/*
 * memcpy and memmove implement.
 */
	.text
	.global memcpy
	.global memmove
	.type   memcpy, @function
	.type   memmove,@function
	.align 4
memcpy:
memmove:
	push	r4-r11
	mov	r9, r0
	/* Test if len equal to 0.  */
	cmpnei r2, 0
	bf	.L_return_m
	/* Test if dest equal to src.  */
	cmpne	r0, r1
	bf	.L_return_m
	/* Select cpoy direction.  */
	cmphs	r1, r0
	bt	__memcpy

	/* Copy from back to front.  */
	addu	r0, r0, r2
	addu	r1, r1, r2

	/* Test if dest or src is not 4 bytes aligned.  */
	or	r4, r0, r1
	andi	r4, r4, 3
	cmpnei	r4, 0
	movi	r5, 0
	bf	.L_dest_aligned_m
	/* Test if len less than 4 bytes.  */
	cmplti	r2, 4
	mov	r5, r2
	/* copy all by byte.  */
	bt	.L_copy_by_byte_m
	/* Test if dest and src both are not 4 bytes aligned.  */
	xor	r4, r0, r1
	andi	r4, r4, 3
	cmpnei	r4, 0
	mov	r5, r2
	/* copy all by byte.  */
	bt	.L_copy_by_byte_m
	/* copy unaligned part by byte.  */
	andi	r5, r0, 3
	movi	r4, 4
	subu	r5, r4, r5
	bf	.L_copy_by_byte_m

	/* 4 bytes aligned.  */
.L_dest_aligned_m:
	subu	r2, r5
	bez	r2, .L_return_m
	/* If dest is aligned, then copy.  */
	zext	r7, r2, 31, 4
	/* Test if len less than 16 bytes.  */
	bez	r7, .L_len_less_16bytes_m
	movi	r8, 0

	/* len > 16 bytes */
	LABLE_ALIGN
.L_len_larger_16bytes_m:
	subi	r1, 16
	subi	r0, 16
#if defined(__CSKY_VDSPV2__)
	vldx.8	vr0, (r1), r8
	PRE_BNEZAD (r7)
	vstx.8	vr0, (r0), r8
#elif defined(__CK860__)
	ldw	r6, (r1, 12)
	stw	r6, (r0, 12)
	ldw	r6, (r1, 8)
	stw	r6, (r0, 8)
	ldw	r6, (r1, 4)
	stw	r6, (r0, 4)
	ldw	r6, (r1, 0)
	stw	r6, (r0, 0)
#else
	ldw	r20, (r1, 0)
	ldw	r21, (r1, 4)
	ldw	r22, (r1, 8)
	ldw	r23, (r1, 12)
	stw	r20, (r0, 0)
	stw	r21, (r0, 4)
	stw	r22, (r0, 8)
	stw	r23, (r0, 12)
	PRE_BNEZAD (r7)
#endif
	BNEZAD (r7, .L_len_larger_16bytes_m)

.L_len_less_16bytes_m:
	zext	r7, r2, 3, 2
	zext	r5, r2, 1, 0
	bez	r7, .L_copy_by_byte_to_end_m
.L_len_less_16bytes_loop_m:
	subi	r1, 4
	subi	r0, 4
	ldw	r6, (r1, 0)
	PRE_BNEZAD (r7)
	stw	r6, (r0, 0)
	BNEZAD (r7, .L_len_less_16bytes_loop_m)

.L_copy_by_byte_to_end_m:
	mov	r7, r5
	cmpnei	r7, 0
	bt	.L_copy_by_byte_loop_m
	bf	.L_return_m
	/* copy all by byte.  */
	/* copy unaligned part by byte.  */
.L_copy_by_byte_m:
	mov	r7, r5
	bez	r7, .L_dest_aligned
.L_copy_by_byte_loop_m:
	subi	r1, 1
	subi	r0, 1
	ldb	r6, (r1, 0)
	PRE_BNEZAD (r7)
	stb	r6, (r0, 0)
	BNEZAD (r7, .L_copy_by_byte_loop_m)

.L_return_m:
	mov	r0, r9
	pop	r4-r11
	rts
	.size memmove,.-memmove
	.size memcpy,.-memcpy
