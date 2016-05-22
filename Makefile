SRC_C =
SRC_S =

-include board.mk
-include user.mk

ifndef $(CROSS_COMPILE)
	CROSS_COMPILE = arm-none-eabi-
endif

LD = $(CROSS_COMPILE)ld
CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

#CFLAGS = -g -Wall -mcpu=cortex-a7 -mfpu=neon -marm -mthumb-interwork -nostdlib -fno-builtin -mfloat-abi=hard -mlittle-endian
CFLAGS = -g -Wall -mcpu=cortex-a7 -mfpu=vfpv4 -marm -mthumb-interwork -nostdlib -fno-builtin -mlittle-endian -mfloat-abi=hard
CFLAGS+= -O2 -std=c99
CFLAGS+= -mstructure-size-boundary=8
CFLAGS+= -MMD
SFLAGS = -MMD

COMPILE.S = $(CC) $(CFLAGS) -c

SRC_C += boards/$(BOARD)/serial.c
SRC_S += boards/$(BOARD)/startup.S

SRC_C += user/$(SETTING)/user_init.c


SRC_S += memory_manage.S
SRC_C += port.c debug.c page_table.c hyp_call.c gic.c timer.c irq_handler.c virtual_gic.c vcpu.c logger.c cache.c default_schedule.c my_scheduler.c schedule.c fp_scheduler.c timer_event.c queue.c edf_scheduler.c rbtree.c virtual_device_handle.c vint_sender.c system_init.c vm.c virtual_psci.c coproc.c
OBJ = 
OBJ += $(SRC_C:.c=.o)
OBJ += $(SRC_S:.S=.o)
DEPS = $(OBJ:.o=.d)
INCLUDE = -I ./include -I boards/$(BOARD) -I user/$(SETTING)


ifndef $(UPLOAD_DIR)
	UPLOAD_DIR = /srv/tftp
endif

.PHONY: all clean

TARGET = main_$(BOARD)_$(SETTING)

ALL = $(TARGET).bin $(TARGET).asm board.mk user.mk

all: $(ALL)

$(TARGET).bin: $(TARGET)
	$(OBJCOPY) -O binary $< $@

$(TEST).bin: $(TEST)
	$(OBJCOPY) -O binary $< $@

$(TARGET).asm: $(TARGET)
	$(OBJDUMP) -d $<  > $@

$(TARGET): $(OBJ)
	$(LD) -nostdlib -nostdinc -T boards/$(BOARD)/main.lnk $^  -o $@ $(LIBS)

%.o: %.S
	$(COMPILE.S) $(INCLUDE) $(SFLAGS) $< -o $@

%.o: %.c
	$(CC) $(INCLUDE) -c $< -o $@ $(CFLAGS) 

clean:
	$(RM) $(OBJ) $(TARGET) $(TARGET).bin
	$(RM) $(DEPS)

upload: $(TARGET).bin
	cp $(TARGET).bin $(TARGET) $(UPLOAD_DIR)
	cp $(TARGET).bin  $(UPLOAD_DIR)/main.bin

-include $(DEPS)
