; Planar-float AVX2: eight pixels per loop; scalar tail in C.

bits 64
default rel

section .rdata align=16
red_weight:   dd 0.299
green_weight: dd 0.587
blue_weight:  dd 0.114
max_gray:     dd 255.0

section .text
    global planar_avx2_kernel

planar_avx2_kernel:
    mov r10, [rsp + 40]
    shr r10, 3
    jz .done

    vbroadcastss ymm4, [red_weight]
    vbroadcastss ymm5, [green_weight]
    vbroadcastss ymm3, [blue_weight]

.loop:
    vmovups ymm0, [rcx]
    vmulps ymm0, ymm0, ymm4

    vmovups ymm1, [rdx]
    vmulps ymm1, ymm1, ymm5
    vaddps ymm0, ymm0, ymm1

    vmovups ymm1, [r8]
    vmulps ymm1, ymm1, ymm3
    vaddps ymm0, ymm0, ymm1

    vxorps ymm1, ymm1, ymm1
    vmaxps ymm0, ymm0, ymm1
    vbroadcastss ymm1, [max_gray]
    vminps ymm0, ymm0, ymm1

    vroundps ymm0, ymm0, 8
    vcvttps2dq ymm0, ymm0

    ; Pack eight rounded intensities into eight bytes.
    vextracti128 xmm1, ymm0, 1
    vpackssdw xmm0, xmm0, xmm1
    vpackuswb xmm0, xmm0, xmm0
    vmovq [r9], xmm0

    add rcx, 32
    add rdx, 32
    add r8, 32
    add r9, 8
    dec r10
    jnz .loop

.done:
    vzeroupper
    ret
