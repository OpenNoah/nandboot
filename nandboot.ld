/* Specify the memory areas */
MEMORY
{
  /* The boot rom only loads up to 8k from NAND */
  LOAD(xrw):  ORIGIN = 0x80000000, LENGTH = 0x2000
  RAM(xrw):   ORIGIN = 0x80002000, LENGTH = 0x2000
}

/* Stack starts from the end of available RAMs */
__stack_end__ = ORIGIN(RAM) + LENGTH(RAM);

/* Entry Point */
ENTRY(_entry)

/* Define output sections */
SECTIONS
{
  /* Header for bootrom to identify NAND type and entry */
  .header :
  {
    . = ALIGN(4);
    KEEP(*(.header))
  } >LOAD

  /* The program code and other data goes into TEXT */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP(*(.init))
    KEEP(*(.fini))
    _etext = .;        /* define a global symbols at end of code */
  } >LOAD

  /* Constant data goes into TEXT */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
  } >LOAD

  . = ALIGN(4);
  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >LOAD
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >LOAD
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >LOAD

  /* Initialized data sections goes into RAM, load LMA copy after code */
  . = ALIGN(4);
  .data :
  {
    __data_start__ = .;
    . = ALIGN(4);
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    /* Special program code inside RAM */
    *(.ram)
    *(.ram*)

    . = ALIGN(4);
    __data_end__ = .;
  } >LOAD AT>LOAD

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss (NOLOAD) :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    __bss_start__ = .;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    __bss_end__ = .;
  } >RAM

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
    *(.olist.*)
  }
}
