    .extern main
    .extern __stack_end__
    .extern __bss_start__
    .extern __bss_end__

    .global _header

    .section .header, "axR"
_header:
    .byte   0xff    # Bus width: 8-bit
    .byte   0x55    # Check code
    .byte   0x55
    .byte   0x55
    .byte   0x55
    .byte   0x55
    .byte   0x55
    .byte   0x55
    .byte   0xff    # Row address: 3 cycles
    .byte   0xff    # Page size: 2/4 KiB
    .byte   0x00    # Page size: 4 KiB
    .byte   0x00    # Reserved, unused
    b       _entry
    nop                     # delay slot
    .size   _header, .-_header

    .section .text, "axR"
_entry:
    # Clear .bss
    la      $8, __bss_start__
    la      $9, __bss_end__
_bss:
    beq     $8, $9, _bss_done
    nop                     # delay slot
    sw      $0, ($8)
    addi    $8, $8, 4
    j       _bss
    nop                     # delay slot

_bss_done:
    # No need to copy .data
    # Setup stack, jump to C code
    la	$29, __stack_end__
    j	main
    nop
    .size   _entry, .-_entry
