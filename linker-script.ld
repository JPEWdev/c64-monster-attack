/* C64 PRG file linker script.
 *
 * BASIC memory regions are freely overwritten, but any KERNAL regions are left alone.
 *
 * Produces a PRG file with a SYS command to start the program.
 */

__basic_zp_start = 0x0002;
__basic_zp_end = 0x0090;

MEMORY {
    screen : ORIGIN = 0x400, LENGTH = 0x3f0
    ram (rw) : ORIGIN = 0x0801, LENGTH = 0xA7FF
    video (rw) : ORIGIN = 0xC000, LENGTH = 0x4000
    stack (rw) : ORIGIN = 0xB000, LENGTH = 0x1000
}

INCLUDE commodore.ld

INPUT(unmap-basic.o)

/* With the BASIC ROM unmapped, set initial soft stack address to 
 * right before the I/O mapped $D000-DFFF area. (It grows down.) */
__stack = ORIGIN(stack) + LENGTH(stack);
color_data = 0xd800;

SECTIONS {
  .video : {
    video_base = .;
    *(video*)
  } > video
  .screen (NOLOAD): {*(screendata)} >video
}

OUTPUT_FORMAT {
    /* Tells the C64 LOAD command where to place the file's contents. */
    SHORT(ORIGIN(ram))
    TRIM(ram)
}
