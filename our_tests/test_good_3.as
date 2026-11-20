;the input can be very messy but we're ok

        BLABLA:   bne X

X:stop
jmp     BLABLA[r3][r4]

mcro hi
    ;blabla
mcroend

hi
hi

.entry BLABLA

.extern HI

inc HI