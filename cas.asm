.sect "kernelTEXT"
.arm
; This function use to as CAS(Compare and Set) atomic, the lockless queue would based on it
; It proteced your critical-area lockless when both interrupt and backend-task enter your API.
;
; ret atomic_cas(lockAddr)
; r0:      lockAddr
; r1:      val
; r2:      retry or not
; ret(r0): return success(0) or fail(1)
.def asm_atomic_cas
.asmfunc

asm_atomic_cas
RETRY:
    LDREX        R4,        [R0]           ; load the lock value
    CMP          R4,        R1             ; is the lock free ?
    BEQ          LOCKED_RET                ; if locked, return fail
    MOV          R5,        #10            ; retry count = 10
    STREX        R3,        R1,        [R0]; try and claim the lock
    CMPEQ        R2,        #0x1           ; if enable retry, do it
    CMP          R3,        #0
    BEQ          SUCCESS
    SUB          R5,        R5,        #1  ; retry count --
    CMP          R5,        #0
    BNE          RETRY
SUCCESS:
    MOV          R0,        R3
    MOV          PC,        LR              ; return
LOCKED_RET：
    MOV          R0,        #0x1
    MOV          PC,        LR
