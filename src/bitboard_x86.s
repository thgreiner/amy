        .text
.globl _CountBits
_CountBits:
        movl    4(%esp), %ecx
        xorl    %eax, %eax
        testl   %ecx, %ecx
        jz      l1
l0:
        leal    -1(%ecx), %edx
        incl    %eax
        andl    %edx, %ecx
        jnz     l0
l1:
        movl    8(%esp), %ecx
        testl   %ecx, %ecx
        jz      l3
l2:
        leal    -1(%ecx), %edx
        incl    %eax
        andl    %edx, %ecx
        jnz     l2
l3:
        ret

/*----------------------------------------------------------------------------*/

        .globl  _FindSetBit
_FindSetBit:
        cmpl    $1, 8(%esp)
        sbbl    %eax, %eax
        movl    8(%esp,%eax,4), %edx
        bsrl    %edx, %ecx
        jz      l4
        andl    $32, %eax
        subl    $31, %ecx
        subl    %ecx, %eax
        ret
l4:     movl    $64, %eax
        ret

