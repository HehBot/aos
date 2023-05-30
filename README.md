### Learning OSDev

## Prerequisites
In order to build, you will require cross-compiled build tools for the i386 platform. For this, you can refer to [this](https://wiki.osdev.org/GCC_Cross-Compiler#The_Build) excellent article on OSDev Wiki. (Note that you don't need a cross-compiled GDB if your computer architecture is x86).

In order to run, install `qemu-system-i386`.

## Build

    $ make disk -j$(nproc)

## Run

    $ make run

To run with GDB debugging

    $ make debug

## References
 - [Little OS Book](http://littleosbook.github.io)
 - [This PDF](https://www.cs.bham.ac.uk//~exr/lectures/opsys/10_11/lectures/os-dev.pdf)
 - [OSDev Wiki](https://wiki.osdev.org)
