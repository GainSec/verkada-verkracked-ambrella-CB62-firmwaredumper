ENTRY(_start)
SECTIONS
{
  . = 0x02000000;
  .text : { *(.text.entry) *(.text*) *(.rodata*) }
  /DISCARD/ : { *(.comment*) *(.note*) *(.eh_frame*) }
}
