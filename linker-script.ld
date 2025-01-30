INCLUDE link.ld

MEMORY {
    screen : ORIGIN = 0x400, LENGTH = 0x3f0
    video (rw) : ORIGIN = 0x4000, LENGTH = 0x4000
    color (rw) : ORIGIN = 0xd800, LENGTH = 0x3e8
}

SECTIONS {
  .video : {
    video_base = .;
    *(video*)
  } > video
  .screen (NOLOAD): {*(screendata)} >video
  .color (NOLOAD): {*(colordata)} >color
}
