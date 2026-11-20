.entry LENGTH
.extern ru 

piza: mov r1,r2

.extern lol


MAIN:   mov M1[r2][r7],LENGTH
        add r2,STR
        mov lol, ru
        mov lol, ru

LOOP:   jmp END
        prn #-5

        ;dasda

        mcro a_mc         pizza         
        mov M1[r3][r3],r3
        bne LOOP
        mcroend               
        
        mcro b_mcaro                 
        mcroend      
        

        
        a_mc

        b_mcaro
        b_mcaro
        b_mcaro

END:    stop

STR:    .string "abcdef"
