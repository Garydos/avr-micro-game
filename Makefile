CC=avr-gcc
CFLAGS=-g -Os -mmcu=atmega32
DEPS=avr.h avr.c lcd.h lcd.c
OBJ=main.o avr.o lcd.o 
MAIN=main

all: $(MAIN)
.PHONY: all

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(MAIN): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm *.o $(MAIN)

.PHONY: flash
flash: $(MAIN)
	sudo avrdude -c atmelice_isp -p m32 -U flash:w:$(MAIN)

.PHONY: fuse
fuse:
	sudo avrdude -c atmelice_isp -p m32 -U lfuse:w:0xff:m -U hfuse:w:0x99:m

.PHONY: unfuse
unfuse:
	sudo avrdude -c atmelice_isp -p m32 -U lfuse:w:0xe1:m -U hfuse:w:0x99:m

.PHONY: disable_jtag_w_fuse
disable_jtag_w_fuse:
	sudo avrdude -c atmelice_isp -p m32 -U lfuse:w:0xff:m -U hfuse:w:0xd9:m

.PHONY: disable_jtag_wo_fuse
disable_jtag_wo_fuse:
	sudo avrdude -c atmelice_isp -p m32 -U lfuse:w:0xe1:m -U hfuse:w:0xd9:m
