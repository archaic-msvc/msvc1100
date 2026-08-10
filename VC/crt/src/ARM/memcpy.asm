;***
; Copyright (c) Microsoft Corporation.  All rights reserved.
;**********

#include "ksarm.h"

#define LOOKAHEAD_SIZE 64

        GBLA    COUNT

        TEXTAREA

        LEAF_ENTRY memcpy

        ALTERNATE_ENTRY __memcpy_forward

        cmp     r2, #16                                 ; less than 16 bytes?
        pld     [r1]                                    ; preload the first cache line
        mov     r3, r0                                  ; use r3 as our destination
        bhs     __memcpy_large                          ; go to the small copy case directly

CpySmal tbb     [pc, r2]                                ; branch to specialized bits for small copies
CTable  dcb     (Copy0 - CTable) / 2                    ; 0B
        dcb     (Copy1 - CTable) / 2                    ; 1B
        dcb     (Copy2 - CTable) / 2                    ; 2B
        dcb     (Copy3 - CTable) / 2                    ; 3B
        dcb     (Copy4 - CTable) / 2                    ; 4B
        dcb     (Copy5 - CTable) / 2                    ; 5B
        dcb     (Copy6 - CTable) / 2                    ; 6B
        dcb     (Copy7 - CTable) / 2                    ; 7B
        dcb     (Copy8 - CTable) / 2                    ; 8B
        dcb     (Copy9 - CTable) / 2                    ; 9B
        dcb     (Copy10 - CTable) / 2                   ; 10B
        dcb     (Copy11 - CTable) / 2                   ; 11B
        dcb     (Copy12 - CTable) / 2                   ; 12B
        dcb     (Copy13 - CTable) / 2                   ; 13B
        dcb     (Copy14 - CTable) / 2                   ; 14B
        dcb     (Copy15 - CTable) / 2                   ; 15B

Copy1   ldrb    r2, [r1]
        strb    r2, [r3]
Copy0   bx      lr

Copy2   ldrh    r2, [r1]
        strh    r2, [r3]
        bx      lr

Copy3   ldrh    r2, [r1]
        ldrb    r1, [r1, #2]
        strh    r2, [r3]
        strb    r1, [r3, #2]
        bx      lr

Copy4   ldr     r2, [r1]
        str     r2, [r3]
        bx      lr

Copy5   ldr     r2, [r1]
        ldrb    r1, [r1, #4]
        str     r2, [r3]
        strb    r1, [r3, #4]
        bx      lr

Copy6   ldr     r2, [r1]
        ldrh    r1, [r1, #4]
        str     r2, [r3]
        strh    r1, [r3, #4]
        bx      lr

Copy7   ldr     r12, [r1]
        ldrh    r2, [r1, #4]
        ldrb    r1, [r1, #6]
        str     r12, [r3]
        strh    r2, [r3, #4]
        strb    r1, [r3, #6]
        bx      lr

Copy8   ldr     r2, [r1]
        ldr     r1, [r1, #4]
        str     r2, [r3]
        str     r1, [r3, #4]
        bx      lr

Copy9   ldr     r12, [r1]
        ldr     r2, [r1, #4]
        ldrb    r1, [r1, #8]
        str     r12, [r3]
        str     r2, [r3, #4]
        strb    r1, [r3, #8]
        bx      lr

Copy10  ldr     r12, [r1]
        ldr     r2, [r1, #4]
        ldrh    r1, [r1, #8]
        str     r12, [r3]
        str     r2, [r3, #4]
        strh    r1, [r3, #8]
        bx      lr

Copy11  ldr     r12, [r1]
        ldr     r2, [r1, #4]
        str     r12, [r3]
        str     r2, [r3, #4]
        ldrh    r2, [r1, #8]
        ldrb    r1, [r1, #10]
        strh    r2, [r3, #8]
        strb    r1, [r3, #10]
        bx      lr

Copy12  ldr     r12, [r1]
        ldr     r2, [r1, #4]
        ldr     r1, [r1, #8]
        str     r12, [r3]
        str     r2, [r3, #4]
        str     r1, [r3, #8]
        bx      lr

Copy13  ldr     r12, [r1]
        ldr     r2, [r1, #4]
        str     r12, [r3]
        str     r2, [r3, #4]
        ldr     r2, [r1, #8]
        ldrb    r1, [r1, #12]
        str     r2, [r3, #8]
        strb    r1, [r3, #12]
        bx      lr

Copy14  ldr     r12, [r1]
        ldr     r2, [r1, #4]
        str     r12, [r3]
        str     r2, [r3, #4]
        ldr     r2, [r1, #8]
        ldrh    r1, [r1, #12]
        str     r2, [r3, #8]
        strh    r1, [r3, #12]
        bx      lr

Copy15  ldr     r12, [r1]
        ldr     r2, [r1, #4]
        str     r12, [r3]
        str     r2, [r3, #4]
        ldr     r12, [r1, #8]
        ldrh    r2, [r1, #12]
        ldrb    r1, [r1, #14]
        str     r12, [r3, #8]
        strh    r2, [r3, #12]
        strb    r1, [r3, #14]
        bx      lr

        LEAF_END memcpy


        NESTED_ENTRY __memcpy_large

        PROLOG_NOP tst r3, #7                           ; check alignment up front
        PROLOG_PUSH {r4-r11}
        beq     CAlign4                                 ; if already aligned, just skip ahead

        tst     r3, #1                                  ; need to align to 2 byte boundary?
        beq     CAlign1                                 ; skip if not
        ldrb    r4, [r1], #1                            ; fetch byte
        subs    r2, r2, #1                              ; decrement count
        strb    r4, [r3], #1                            ; store byte
CAlign1 tst     r3, #2                                  ; need to align to 4 byte boundary?
        beq     CAlign2                                 ; skip if not
        ldrh    r4, [r1], #2                            ; fetch halfword
        subs    r2, r2, #2                              ; decrement count
        strh    r4, [r3], #2                            ; store halfword
CAlign2 tst     r3, #4                                  ; need to align to 8 byte boundary?
        beq     CAlign4                                 ; skip if not
        ldr     r4, [r1], #4                            ; fetch word
        subs    r2, r2, #4                              ; decrement count
        str     r4, [r3], #4                            ; store word

CAlign4 tst     r1, #3                                  ; is the source now word-aligned?
        bne     CpySlow                                 ; if not, we have to use the slow path

CpyFast subs    r2, r2, #LOOKAHEAD_SIZE                 ; pre-decrement to simplify the loop
        blo     CFMed                                   ; skip over the loop if we don't have enough
    IF (LOOKAHEAD_SIZE > 32)
        pld     [r1, #32]                               ; pre-load one block ahead
    ENDIF
    IF (LOOKAHEAD_SIZE > 64)
        pld     [r1, #64]                               ; pre-load two blocks ahead
    ENDIF
    IF (LOOKAHEAD_SIZE > 96)
        pld     [r1, #96]                               ; pre-load three blocks ahead
    ENDIF

CFLarge pld     [r1, #LOOKAHEAD_SIZE]                   ; prefetch ahead
        ldm     r1!, {r4-r10, r12}                      ; load 32 bytes
        subs    r2, r2, #32                             ; count the bytes for this block
        stm     r3!, {r4-r10, r12}                      ; store 32 bytes
        bhs     CFLarge                                 ; keep going until we're done

CFMed   adds    r2, r2, #(LOOKAHEAD_SIZE - 8)           ; recover original count, and pre-decrement
        blo     CFMSkip                                 ; if not enough remaining, skip this loop
CFMLoop ldrd    r4, r5, [r1], #8                        ; fetch pair of words
        subs    r2, r2, #8                              ; decrement count
        strd    r4, r5, [r3], #8                        ; store pair of words
        bhs     CFMLoop                                 ; loop while we still have data remaining
CFMSkip adds    r2, r2, #8                              ; recover final count

        EPILOG_POP {r4-r11}
        EPILOG_NOP bne CpySmal                          ; if some left, continue with small
        EPILOG_RETURN                                   ; else just return

CpySlow subs    r2, r2, #LOOKAHEAD_SIZE                 ; pre-decrement to simplify the loop
        blo     CSMed                                   ; skip over the loop if we don't have enough
    IF (LOOKAHEAD_SIZE > 32)
        pld     [r1, #32]                               ; pre-load one block ahead
    ENDIF
    IF (LOOKAHEAD_SIZE > 64)
        pld     [r1, #64]                               ; pre-load two blocks ahead
    ENDIF
    IF (LOOKAHEAD_SIZE > 96)
        pld     [r1, #96]                               ; pre-load three blocks ahead
    ENDIF

CSLarge pld     [r1, #LOOKAHEAD_SIZE]                   ; prefetch ahead
        ldr     r4, [r1, #0]                            ; load 32 bytes
        ldr     r5, [r1, #4]                            ;
        ldr     r6, [r1, #8]                            ;
        ldr     r7, [r1, #12]                           ;
        ldr     r8, [r1, #16]                           ;
        ldr     r9, [r1, #20]                           ;
        ldr     r10, [r1, #24]                          ;
        ldr     r12, [r1, #28]                          ;
        adds    r1, r1, #32                             ; update pointer
        subs    r2, r2, #32                             ; count the bytes for this block
        stm     r3!, {r4-r10, r12}                      ; store 32 bytes
        bhs     CSLarge                                 ; keep going until we're done

CSMed   adds    r2, r2, #(LOOKAHEAD_SIZE - 8)           ; recover original count, and pre-decrement
        blo     CSMSkip                                 ; if not enough remaining, skip this loop
CSMLoop ldr     r4, [r1]                                ; fetch pair of words
        ldr     r5, [r1, #4]                            ;
        adds    r1, r1, #8                              ; update pointer
        subs    r2, r2, #8                              ; decrement count
        strd    r4, r5, [r3], #8                        ; store pair of words
        bhs     CSMLoop                                 ; loop while we still have data remaining
CSMSkip adds    r2, r2, #8                              ; recover final count

        EPILOG_POP {r4-r11}
        EPILOG_NOP bne CpySmal                          ; if some left, continue with small
        EPILOG_RETURN                                   ; else just return

        NESTED_END __memcpy_large



        LEAF_ENTRY memmove

        sub     r3, r0, r1                              ; compute dest - source
        cmp     r3, r2                                  ; compare against size
        bhs     memcpy                                  ; if no overlap, we can just do memcpy

        ALTERNATE_ENTRY __memmove_reverse

        cmp     r2, #16                                 ; less than 16 bytes?
        pld     [r1]                                    ; preload the first cache line
        bhs     __memmove_large                         ; go to the small copy case directly

MovSmal tbb     [pc, r2]                                ; branch to specialized bits for small copies
MTable  dcb     (Move0 - MTable) / 2                    ; 0B
        dcb     (Move1 - MTable) / 2                    ; 1B
        dcb     (Move2 - MTable) / 2                    ; 2B
        dcb     (Move3 - MTable) / 2                    ; 3B
        dcb     (Move4 - MTable) / 2                    ; 4B
        dcb     (Move5 - MTable) / 2                    ; 5B
        dcb     (Move6 - MTable) / 2                    ; 6B
        dcb     (Move7 - MTable) / 2                    ; 7B
        dcb     (Move8 - MTable) / 2                    ; 8B
        dcb     (Move9 - MTable) / 2                    ; 9B
        dcb     (Move10 - MTable) / 2                   ; 10B
        dcb     (Move11 - MTable) / 2                   ; 11B
        dcb     (Move12 - MTable) / 2                   ; 12B
        dcb     (Move13 - MTable) / 2                   ; 13B
        dcb     (Move14 - MTable) / 2                   ; 14B
        dcb     (Move15 - MTable) / 2                   ; 15B

Move1   ldrb    r2, [r1]
        strb    r2, [r0]
Move0   bx      lr

Move2   ldrh    r2, [r1]
        strh    r2, [r0]
        bx      lr

Move3   ldrh    r2, [r1]
        ldrb    r1, [r1, #2]
        strh    r2, [r0]
        strb    r1, [r0, #2]
        bx      lr

Move4   ldr     r2, [r1]
        str     r2, [r0]
        bx      lr

Move5   ldr     r2, [r1]
        ldrb    r1, [r1, #4]
        str     r2, [r0]
        strb    r1, [r0, #4]
        bx      lr

Move6   ldr     r2, [r1]
        ldrh    r1, [r1, #4]
        str     r2, [r0]
        strh    r1, [r0, #4]
        bx      lr

Move7   ldr     r3, [r1]
        ldrh    r2, [r1, #4]
        ldrb    r1, [r1, #6]
        str     r3, [r0]
        strh    r2, [r0, #4]
        strb    r1, [r0, #6]
        bx      lr

Move8   ldr     r2, [r1]
        ldr     r1, [r1, #4]
        str     r2, [r0]
        str     r1, [r0, #4]
        bx      lr

Move9   ldr     r3, [r1]
        ldr     r2, [r1, #4]
        ldrb    r1, [r1, #8]
        str     r3, [r0]
        str     r2, [r0, #4]
        strb    r1, [r0, #8]
        bx      lr

Move10  ldr     r3, [r1]
        ldr     r2, [r1, #4]
        ldrh    r1, [r1, #8]
        str     r3, [r0]
        str     r2, [r0, #4]
        strh    r1, [r0, #8]
        bx      lr

Move11  ldr     r12, [r1]
        ldr     r3, [r1, #4]
        ldrh    r2, [r1, #8]
        ldrb    r1, [r1, #10]
        str     r12, [r0]
        str     r3, [r0, #4]
        strh    r2, [r0, #8]
        strb    r1, [r0, #10]
        bx      lr

Move12  ldr     r12, [r1]
        ldr     r2, [r1, #4]
        ldr     r1, [r1, #8]
        str     r12, [r0]
        str     r2, [r0, #4]
        str     r1, [r0, #8]
        bx      lr

Move13  ldr     r12, [r1]
        ldr     r3, [r1, #4]
        ldr     r2, [r1, #8]
        ldrb    r1, [r1, #12]
        str     r12, [r0]
        str     r3, [r0, #4]
        str     r2, [r0, #8]
        strb    r1, [r0, #12]
        bx      lr

Move14  ldr     r12, [r1]
        ldr     r3, [r1, #4]
        ldr     r2, [r1, #8]
        ldrh    r1, [r1, #12]
        str     r12, [r0]
        str     r3, [r0, #4]
        str     r2, [r0, #8]
        strh    r1, [r0, #12]
        bx      lr

Move15  ldrh    r3, [r1, #12]
        ldrb    r2, [r1, #14]
        strh    r3, [r0, #12]
        strb    r2, [r0, #14]
        ldr     r3, [r1]
        ldr     r2, [r1, #4]
        ldr     r1, [r1, #8]
        str     r3, [r0]
        str     r2, [r0, #4]
        str     r1, [r0, #8]
        bx      lr

        LEAF_END memmove


        NESTED_ENTRY __memmove_large

        PROLOG_NOP adds r3, r0, r2                      ; advance destination to end
        PROLOG_NOP adds r1, r1, r2                      ; advance source to end
        PROLOG_NOP tst r3, #7                           ; check dest alignment up front
        PROLOG_PUSH {r4-r11}
        beq     MAlign4                                 ; if already aligned, just skip ahead

        tst     r3, #1                                  ; need to align to 2 byte boundary?
        beq     MAlign1                                 ; skip if not
        ldrb    r4, [r1, #-1]!                          ; fetch byte
        subs    r2, r2, #1                              ; decrement count
        strb    r4, [r3, #-1]!                          ; store byte
MAlign1 tst     r3, #2                                  ; need to align to 4 byte boundary?
        beq     MAlign2                                 ; skip if not
        ldrh    r4, [r1, #-2]!                          ; fetch halfword
        subs    r2, r2, #2                              ; decrement count
        strh    r4, [r3, #-2]!                          ; store halfword
MAlign2 tst     r3, #4                                  ; need to align to 8 byte boundary?
        beq     MAlign4                                 ; skip if not
        ldr     r4, [r1, #-4]!                          ; fetch word
        subs    r2, r2, #4                              ; decrement count
        str     r4, [r3, #-4]!                          ; store word

MAlign4 tst     r1, #3                                  ; is the source now word-aligned?
        bne     MovSlow                                 ; if not, we have to use the slow path

MovFast subs    r2, r2, #LOOKAHEAD_SIZE                 ; pre-decrement to simplify the loop
        blo     MFMed                                   ; skip over the loop if we don't have enough
    IF (LOOKAHEAD_SIZE > 32)
        pld     [r1, #-32]                              ; pre-load one block ahead
    ENDIF
    IF (LOOKAHEAD_SIZE > 64)
        pld     [r1, #-64]                              ; pre-load two blocks ahead
    ENDIF
    IF (LOOKAHEAD_SIZE > 96)
        pld     [r1, #-96]                              ; pre-load three blocks ahead
    ENDIF

MFLarge pld     [r1, #-LOOKAHEAD_SIZE]                  ; prefetch ahead
        ldmdb   r1!, {r4-r10, r12}                      ; load 32 bytes
        subs    r2, r2, #32                             ; count the bytes for this block
        stmdb   r3!, {r4-r10, r12}                      ; store 32 bytes
        bhs     MFLarge                                 ; keep going until we're done

MFMed   adds    r2, r2, #(LOOKAHEAD_SIZE - 8)           ; recover original count, and pre-decrement
        blo     MFMSkip                                 ; if not enough remaining, skip this loop
MFMLoop ldrd    r4, r5, [r1, #-8]!                      ; fetch pair of words
        subs    r2, r2, #8                              ; decrement count
        strd    r4, r5, [r3, #-8]!                      ; store pair of words
        bhs     MFMLoop                                 ; loop while we still have data remaining
MFMSkip adds    r2, r2, #8                              ; determine final count
        sub     r1, r1, r2                              ; recover original source

        EPILOG_POP {r4-r11}
        EPILOG_NOP bne MovSmal                          ; if some left, continue with small
        EPILOG_RETURN                                   ; else just return

MovSlow subs    r2, r2, #LOOKAHEAD_SIZE                 ; pre-decrement to simplify the loop
        blo     MSMed                                   ; skip over the loop if we don't have enough
    IF (LOOKAHEAD_SIZE > 32)
        pld     [r1, #-32]                              ; pre-load one block ahead
    ENDIF
    IF (LOOKAHEAD_SIZE > 64)
        pld     [r1, #-64]                              ; pre-load two blocks ahead
    ENDIF
    IF (LOOKAHEAD_SIZE > 96)
        pld     [r1, #-96]                              ; pre-load three blocks ahead
    ENDIF

MSLarge pld     [r1, #-LOOKAHEAD_SIZE]                  ; prefetch ahead
        ldr     r4, [r1, #-32]!                         ; load 32 bytes
        ldr     r5, [r1, #4]                            ;
        ldr     r6, [r1, #8]                            ;
        ldr     r7, [r1, #12]                           ;
        ldr     r8, [r1, #16]                           ;
        ldr     r9, [r1, #20]                           ;
        ldr     r10, [r1, #24]                          ;
        ldr     r12, [r1, #28]                          ;
        subs    r2, r2, #32                             ; count the bytes for this block
        stmdb   r3!, {r4-r10, r12}                      ; store 32 bytes
        bhs     MSLarge                                 ; keep going until we're done

MSMed   adds    r2, r2, #(LOOKAHEAD_SIZE - 8)           ; recover original count, and pre-decrement
        blo     MSMSkip                                 ; if not enough remaining, skip this loop
MSMLoop ldr     r4, [r1, #-8]!                          ; fetch pair of words
        ldr     r5, [r1, #4]                            ;
        subs    r2, r2, #8                              ; decrement count
        strd    r4, r5, [r3, #-8]!                      ; store pair of words
        bhs     MSMLoop                                 ; loop while we still have data remaining
MSMSkip adds    r2, r2, #8                              ; determine final count
        sub     r1, r1, r2                              ; recover original source

        EPILOG_POP {r4-r11}
        EPILOG_NOP bne MovSmal                          ; if some left, continue with small
        EPILOG_RETURN                                   ; else just return

        NESTED_END __memmove_large

        END
