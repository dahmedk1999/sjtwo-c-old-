/**
 * References:
 * - https://sourceware.org/binutils/docs/ld/Basic-Script-Concepts.html#Basic-Script-Concepts
 * - https://arobenko.gitbooks.io/bare_metal_cpp/content/
 */

MEMORY
{
  flash (rx) : ORIGIN = 0x00000000, LENGTH = 512K
  ram_main (rwx)   : ORIGIN = 0x10000000, LENGTH = 64K
  ram_peripheral (rwx)   : ORIGIN = 0x20000000, LENGTH = 32K
}

/*************************************************
                                                  *
     64K RAM +-----Stack Top   +-----+ 32K RAM    *
             |    ||           |     |            *
             |    \/           |     |            *
             |     |           |     |            *
             |     |           |     |            *
             |    /\ heap      |    /\            *
             |    || overflow  |    ||            *
             .-----.           +-----Heap Start   *
             |bbbbb|           0x2000.0000        *
             |bbbbb| *bss                         *
             .-----.                              *
             |ddddd|                              *
             |ddddd| *data                        *
             .-----.                              *
            0x1000.0000                           *
                                                  *
**************************************************
*/

/* Not needed but just a linker hint that we expect an entry point of 'main' */
ENTRY(main)

SECTIONS
{
    .text : ALIGN(4)
    {
        KEEP(*(.interrupt_vector_table))
        *(.text*)
    } > flash

    .data : ALIGN(4)
    {
       *(.data*)
       _data_end = .;
    } > ram_main AT>flash

    .bss : ALIGN(4)
    {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        _bss_end = .;
    } > ram_main
}

_heap_start = ORIGIN(ram_peripheral);
_heap_end = ORIGIN(ram_peripheral) + LENGTH(ram_peripheral);
_bdata_lma = LOADADDR(.data);
_bdata_vma = ADDR(.data);
_estack = ORIGIN(ram_main) + LENGTH(ram_main);
__start_of_unused_ram64 = _bss_end;
__end_of_unused_ram64 = ORIGIN(ram_main) + LENGTH(ram_main);

