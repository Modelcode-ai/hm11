/* STM32H743ZITx Linker Script for Flash execution
 *
 * Memory configuration for STM32H743ZIT6:
 *   Flash: 2MB (0x00200000)
 *   RAM: 1MB total across multiple regions
 *   Main RAM: D1 AXI-SRAM (0x00020000 @ 0x24000000)
 *   Additional RAM regions:
 *     DTCM: 128KB (0x00020000 @ 0x20000000)
 *     ITCM: 64KB (0x00010000 @ 0x00000000)
 *     D2 SRAM1-3: 288KB (0x00048000 @ 0x30000000)
 *     D3 SRAM4: 64KB (0x00010000 @ 0x38000000)
 *     Backup SRAM: 4KB (0x00001000 @ 0x38800000)
 */

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM_D1) + LENGTH(RAM_D1);

/* Minimum heap size to allocate */
_Min_Heap_Size = 0x400;

/* Minimum stack size for the application */
_Min_Stack_Size = 0x800;

/* Memory Spaces Definitions */
MEMORY
{
  FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 2048K
  RAM_D1 (xrw)    : ORIGIN = 0x24000000, LENGTH = 512K    /* AXI SRAM in D1 */
  RAM_D2 (xrw)    : ORIGIN = 0x30000000, LENGTH = 288K    /* SRAM1, SRAM2, SRAM3 in D2 */
  RAM_D3 (xrw)    : ORIGIN = 0x38000000, LENGTH = 64K     /* SRAM4 in D3 */
  DTCM_RAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K    /* DTCM RAM */
  ITCM_RAM (xrw)  : ORIGIN = 0x00000000, LENGTH = 64K     /* ITCM RAM */
}

/* Sections Definitions */
SECTIONS
{
  /* The startup code goes into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  /* Used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM_D1, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM_D1 AT> FLASH

  /* Uninitialized data section */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM_D1

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM_D1

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}