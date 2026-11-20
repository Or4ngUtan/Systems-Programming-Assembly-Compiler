mcro someMacro
    mov r1, r2
    dec A
mcroend

someMacro

mov r1,r2
inc #-400

;this is a macroend without opening 'mcro'
mcroend

blabla