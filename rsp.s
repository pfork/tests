/* vectors.s */
.cpu cortex-m3
.thumb

;@-----------------------
.thumb_func
.globl rsp_dump
rsp_dump:
    bx lr
;@-----------------------
.thumb_func
.globl rsp_finish
rsp_finish:
   b  rsp_finish
;@-----------------------
.thumb_func
.globl rsp_detach
rsp_detach:
    bx lr
.end
