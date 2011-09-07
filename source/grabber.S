    .macro onepixel bitofs, incr
    lw  $10, ($9)               // load word from line buf
    lbu $11, ($2)               // load RGB
    ins $10, $11, \bitofs, 4    // insert 4 bits of rgb into $10, offset 0
    sw  $10, ($9)               // store result
    addiu $9, $9, \incr         // increment dest by \incr
    nop                         // 6 clock cycles
    .endm

    .macro ochenton from=0, to=7
    onepixel "(\from*4)", "((-1)*(\from==\to))"
    .if \to-\from
    ochenton "(\from+1)",\to
    .endif
    .endm

    .macro brutalton from=0, to=63
    ochenton
    .if \to-\from
    brutalton "(\from+1)",\to
    .endif
    .endm

    .global grabline
grabline:
    brutalton
    jr $ra



