# Commodore 64 Monster Attack

Commodore 64 game where you fight an endless wave of monsters

This game is basically a tech demo for an adventure game engine. I would like
to use it to write a full adventure game someday.

## Game play

Move around with Joysick 2, use the fire button to extend your sword. Destroy
the skeletons and they might drop coins (which are currently useless) or hearts
(which heal you). Try to get the highest score before running out of health.

## Building from Source

The game is primarily written in C and compiled using
[llvm-mos](https://llvm-mos.org/), which was by far had the best code
generation of all the 6502 compilers I tried (save the interrupt routine code,
which I just hand wrote in assembly).

In order to build the program, you will need:
 1. llvm-mos
 2. cmake
 3. VICE
 4. Python 3

Make sure you have all these programs installed and in your `$PATH` variable
before continuing

### Configuring

Configuration is done using `cmake`. To get started, clone down the source code
and use the following commands:

```shell
mkdir build
cd build
cmake .. -G Ninja
```

It is possible to enable debugging output where the border color will change
depending on what the program is doing by passing `-DDEBUG_MODE=ON` to `cmake`

### Compiling

The program can be compiled by running `ninja` after configuring. This will
build the game as a `.prg` file, as well as create a `.d64` disk image for it.

### Running

To test the program in `vice`, run `ninja run`. This will load the program and
also debug symbols so that it can be debugged. Running `ninja debug` will do
the same, but automatically place a breakpoint at the beginning of `main()`,
which is useful for debugging early problems

### Debugging

Debugging is primarily done using the `vice` monitor. To aid in navigating the
generated code, the build produces a `.map` file which shows the location of
the symbols, and a `.lst` file which is an assembly code listing of the entire
program.

Be aware that llvm-mos does extensive link-time-optimizations, which is great
for code efficiency, but also means that functions may be completely optimized
away or inlined and thus have no symbols.

### Graphics Resources

Graphics are edited using the awesome [PETSCII Editor](http://petscii.krissz.hu/),
then exported in a text formats that can be easily parsed by a python script
(the exact format depends on the resource). You can open the "game.pe" file on
this web app to see them.

I'd prefer to extract the resources directly from the "game.pe" file, but it
appears to be some sort of binary encoding.

## TODO

While the game is certainly playable right now, there are many things I would
still like to add:

 1. Level progression where the levels get harder
 2. More weapons
 3. A store where coins can be spent for power ups (better weapons, more
    health, etc.)
 4. Other enemies
 5. Skeleton AI should respect impassable terrain
