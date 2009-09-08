# Name: Makefile
# Project: Thermanator
# Author: Ben Holt
# Creation Date: 2009-09-07
# Tabsize: 4
# Copyright: (c) 2009 by Ben Holt
# License: GNU GPL v2 ("Free as in herpes")


Device = atmega168
F_CPU = 16000000  # 16M in Hz
Programmer = usbtiny

FuseL = 0xff
FuseH = 0xdc
FuseE = 0xdc

BjhlibObjects = bjhlib/debounce.o bjhlib/time.o

CFlags = -Iusbdrv -I. -DDEBUG_LEVEL=0
UsbObjects = usbdrv/usbdrv.o usbdrv/usbdrvasm.o usbdrv/oddebug.o

AllObjects = $(BjhlibObjects) $(UsbObjects) main.o

CompileOld = avr-gcc -Wall -Werror -Os -DF_CPU=$(F_CPU) $(CFlags) -mmcu=$(Device)  # HACK: usbdrvasm can't cope with c99
Compile = $(CompileOld) -std=c99

# symbolic targets:
all: hex

help:
	@echo "make hex ....... to build main.hex"
	@echo "make program ... to flash fuses and firmware"
	@echo "make fuse ...... to flash the fuses"
	@echo "make flash ..... to flash the firmware"
	@echo "make clean ..... to delete library copies, objects and hex file"

hex: main.hex

program: flash fuse

# rule for programming fuse bits:
Avrdude = avrdude -c $(Programmer) -p $(Device)
fuse:
	@[ "$(FuseH)" != "" -a "$(FuseL)" != "" -a "$(FuseE)" != "" ] || \
		{ echo "*** Edit Makefile and choose values for FuseL, FuseH and FuseE!"; exit 1; }
	$(Avrdude) -U hfuse:w:$(FuseH):m -U lfuse:w:$(FuseL):m -U efuse:w:$(FuseE):m

# rule for uploading firmware:
flash: main.hex
	$(Avrdude) -U flash:w:main.hex:i

# Generic rule for compiling C files:
.c.o:
	$(Compile) -c $< -o $@

# Generic rule for assembling Assembler source files:
.S.o:
	$(CompileOld) -x assembler-with-cpp -c $< -o $@  # HACK: usbdrvasm can't cope with c99
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

# Generic rule for compiling C to assembler, used for debugging only.
.c.s:
	$(Compile) -S $< -o $@

# file targets:

# Since we don't want to keep multiple copies around...
usbdrv:
	cp -r ../../vusb/usbdrv .

cleanUsbdrv:
	rm -rf usbdrv

bjhlib:
	cp -r ../../bjhlib .

cleanBjhlib:
	rm -rf bjhlib

main.elf: usbdrv bjhlib $(AllObjects)	# FIXME: abstract lib dependencies
	$(Compile) -o main.elf $(AllObjects)

main.hex: main.elf
	rm -f main.hex main.eep.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-size main.hex

# rule for deleting dependent files (those which can be built by Make):
clean: cleanUsbdrv cleanBjhlib
	rm -f main.hex main.lst main.obj main.cof main.list main.map main.eep.hex main.elf *.o main.s

# debugging targets:

disasm:	main.elf
	avr-objdump -d main.elf

cpp:
	$(Compile) -E main.c
