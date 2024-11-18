### Learning OSDev

## Prerequisites
In order to build, you will require cross-compiled build tools for the i386 platform. For this, you can refer to [this](https://wiki.osdev.org/GCC_Cross-Compiler#The_Build) excellent article on OSDev Wiki. (Note that you don't need a cross-compiled GDB if your computer architecture is `x86_64`).

In order to run, install `qemu-system-x86_64`.

## Build

    $ make disk -j$(nproc)

## Run

    $ make run

To run with GDB debugging

    $ make debug

## References
 - [OSDev Wiki](https://wiki.osdev.org)
 - [Phil Opperman's guide](https://os.phil-opp.com)
