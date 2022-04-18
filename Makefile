SRC	:= main.c uart.c wdt.c helper.c keypad.c gpio.c pll.c sdram.c
SRC	+= startup.S
NAME	= nandboot

TYPE	?= np1501

OBJ	= $(patsubst %.S,%.o,$(SRC:%.c=%.o))

CROSS	?= mipsel-linux-
AS	:= $(CROSS)gcc
CC	:= $(CROSS)gcc
CXX	:= $(CROSS)g++
LD	:= $(CROSS)g++
NM	:= $(CROSS)nm
SIZE	:= $(CROSS)size
OBJCOPY	:= $(CROSS)objcopy

ARGS	= -mips1 -g -O3 -mno-abicalls -fno-pic -fno-pie -nostdlib
CFLAGS	= $(ARGS) -D$(TYPE)
ASFLAGS	= $(ARGS) -D$(TYPE)
LDFLAGS	= $(ARGS) -Xlinker --gc-sections -T nandboot.ld

.DELETE_ON_ERROR:

.PHONY: all
all: $(NAME)-nand.bin

# Fill the backup section
CLEAN	+= $(NAME)-nand.bin
%-nand.bin: %-padded.bin
	cat $< $< > $@

CLEAN	+= $(NAME)-padded.bin
%-padded.bin: %.bin
	$(OBJCOPY) -I binary -O binary --gap-fill 0xff --pad-to 0x2000 $< $@

CLEAN	+= $(NAME).bin
%.bin: %.elf
	$(OBJCOPY) -O binary --gap-fill 0xff $< $@

CLEAN	+= $(NAME).elf
$(NAME).elf: $(OBJ)
	$(LD) -o $@ $^ $(LDFLAGS)
	$(SIZE) $@

CLEAN	+= $(OBJ)
%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.S
	$(AS) $(ASFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f $(CLEAN)
