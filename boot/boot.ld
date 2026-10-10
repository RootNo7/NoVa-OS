OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386)
ENTRY(start)
SECTIONS
{
  . = 0x7C00;
  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  /DISCARD/ : { *(.data*) *(.bss*) *(.eh_frame*) }
  . = 0x7DFE;
  .sig : { SHORT(0xAA55); }
}
