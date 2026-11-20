mov M1[r3][r3],r3

;we test for reserved words both in declarations and references
mov mov[r3][r3],r3
bne LOOP            
sub r1, M1[lbl][r3]
inc #1
mov r1, #20
mov #-20, r1
mov #128, r1
dec r1
stop 1
stop
label: fictitiousCmdName r1,r2


.mat [  4 ][2  ]
.mat [-1][2  ]
.mat[4][2]
M1:     .mat [4][2] 1,-2,3,-4
M2:     .mat [1][2] 1,2,3,4

.mat[200][2]