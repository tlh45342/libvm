  .text
    // enter SVC mode (0x13) without vectoring
    cps    #19

    // write flags to SPSR, then read back
    msr    spsr_fsxc, #0xF0000000
    mrs    r1, spsr

    .word 0xDEADBEEF
	