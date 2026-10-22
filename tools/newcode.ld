/* The new C code is loaded right after the memory of the original module. */
SECTIONS
{
  . = 0x09D65500;
  .text   : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data   : { *(.data*) *(.sdata*) }
  .bss    : { *(.bss*) *(.sbss*) *(COMMON) }
  _newcode_end = .;
  /DISCARD/ : { *(.comment) *(.MIPS.abiflags) *(.reginfo) *(.pdr) *(.gnu.attributes) *(.mdebug*) }
}
INCLUDE build/syms.ld
