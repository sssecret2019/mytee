OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
  /DISCARD/ : {
    *(.ARM.exidx*)
    *(.ARM.extab*)
    *(.data)
  }
  . = 0;
  _text = .;
  .text : {
    _start = .;
    *(.start)
    *(.text)
    *(.text.*)
    *(.fixup)
    *(.gnu.warning)
    *(.glue_7t)
    *(.glue_7)
  }
  .rodata : {
    *(.rodata)
    *(.rodata.*)
  }
  .piggydata : {
    *(.piggydata)
  }
  . = ALIGN(4);
  _etext = .;
  .got.plt : { *(.got.plt) }
  _got_start = .;
  .got : { *(.got) }
  _got_end = .;
  .pad : { BYTE(0); . = ALIGN(8); }
  _edata = .;
  .image_end (NOLOAD) : {
    _edata_real = .;
  }
  _magic_sig = (0x016f2818);
  _magic_start = (_start);
  _magic_end = (_edata);
  . = ALIGN(8);
  __bss_start = .;
  .bss : { *(.bss) }
  _end = .;
  . = ALIGN(8);
  .stack : { *(.stack) }
  PROVIDE(__pecoff_data_size = ALIGN(512) - ADDR(.data));
  PROVIDE(__pecoff_end = ALIGN(512));
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }
}
