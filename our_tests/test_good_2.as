mcro someMacro
    dec r1
    jmp BLABLA
mcroend

;we allow empty macros
mcro my_macro1
mcroend

mcro my_macro2
mcroend

someMacro
someMacro
my_macro1
my_macro2

;labels can be defined anywhere, not only in the end
MAT: .string "blabla"
.extern BLABLA
mov     BLABLA  ,   MAT[r1][r5]