# Commodore 64 Monster Attack

Commodore 64 game where you fight an endless wave of monsters

This game is basically a tech demo for an adventure game engine. I would like
to use it to write a full adventure game someday.

## Download

The D64 disk image for the latest release can be downloaded from the
[GitHub Releases](https://github.com/JPEWdev/c64-monster-attack/releases) page.

## Game play

Move around with Joysick 2, use the fire button to extend your sword. Destroy
the skeletons and they might drop coins or hearts (which heal you). Try to get
the highest score before running out of health.

You have a choice of 3 weapons, which can be selected using the function keys

**F1** - Sword. Does damage by stabbing enemies when the fire button is
pressed.

**F3** - Flail. Extends out in the direction the player is facing and starts
spinning around while the fire button is held. The faster it spins, the more
damage it does on contact with an enemy. If it collides with too many enemies
it will be withdrawn and need to be re-deployed.

**F5** - Bow and arrow. Pressing and holding the fire button will start drawing
back an arrow. Once it is fully drawn back, releasing the button will loose the
arrow.

Pressing the **s** key will take you to the store where you can spend coins

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
 3. Other enemies
