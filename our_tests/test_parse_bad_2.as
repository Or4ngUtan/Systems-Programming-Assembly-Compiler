mcro a_mc
    inc r2
    mov A,r1
    mov r1, A
mcroend


;we allow only chars that are containable within 8bit signed
;i.e -128 up to 127

SOMELABEL1:   .data 1,2,3,50,   19,-3,   -9
SOMELABEL2:   .data         127     
SOMELABEL3: .data 128
.data 1
.data
K:      .data
LENGTH: .data 1, 2, -128
LENGTH: .data 1, 2, -129


a_mc