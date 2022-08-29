OUTPUT_FORMAT("elf32-m68k");
ENTRY(_DeclHeader);

SECTIONS {
  .text : {
    *(.text.begin)
    *(.text .text.* .sdata .sdata.* .data .data.* .rodata .rodata.*)
    PROVIDE(_DRVREnd = .);
    . = ALIGN(4);
    *(.fblock)
    PROVIDE(ROMSize = .);
    PROVIDE(DirectoryOffset = ((0-.)+20) & 0xFFFFFF);
  }
  /DISCARD/ : { *(*) }
}
