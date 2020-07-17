# skernel

A toy elf_i386 kernel implemented by C and a little ASM.

Support bootloader, basic keyboard input and screen content output.

This kernel can only be compiled under *nix environment. The following assumes you want to run this toy kernel in Ubuntu.

# Installation

## Clone

```
git clone https://github.com/Soptq/skernel.git
cd skernel
```

## Compile kernel

```
sudo apt install -y nasm qemu-system-i386
chmod +x compile.sh
./compile.sh
```

## Run kernel in QEMU simulator

```
qemu-system-i386 -kernel kernel
# if you are in a remote SSH, you might want to run simulator with:
# qemu-system-i386 -kernel kernel -curses
# and quit the simulator by press 'esc' and '2', then press 'q' and 'enter'.
```