# Cross toolchain variables
# If these are not in your path, you can make them absolute.
XT_PRG_PREFIX := mipsel-linux-gnu-
CC := $(XT_PRG_PREFIX)gcc
LD := $(XT_PRG_PREFIX)ld

BUILD_DIR := ./build
SRC_DIR := ./include

PHASE1_SRC_DIR := $(SRC_DIR)/phase1
PHASE2_SRC_DIR := $(SRC_DIR)/phase2
PHASE3_SRC_DIR := $(SRC_DIR)/phase3

# Main function src by phase, since every phase have its main or share a main with another phase
PHASE1_MAIN_SRC := $(PHASE1_SRC_DIR)/p1test.c
PHASE2_MAIN_SRC := $(PHASE2_SRC_DIR)/initial.c
PHASE3_MAIN_SRC := $(PHASE2_SRC_DIR)/initial.c

# Simplistic search for the umps3 installation prefix.
# If you have umps3 installed on some weird location, set UMPS3_DIR_PREFIX by hand.
ifneq ($(wildcard /usr/bin/umps3),)
	UMPS3_DIR_PREFIX = /usr
else
	UMPS3_DIR_PREFIX = /usr/local
endif

# Phases C files that we want to compile (excluding its main function src)
PHASE1_SRCS := $(filter-out $(PHASE1_MAIN_SRC), $(shell find $(PHASE1_SRC_DIR) -name *.c))
PHASE2_SRCS := $(filter-out $(PHASE2_MAIN_SRC), $(shell find $(PHASE2_SRC_DIR) -name *.c))
PHASE3_SRCS := $(filter-out $(PHASE3_MAIN_SRC), $(shell find $(PHASE3_SRC_DIR) -name *.c))

# String substitution for every C file
# As an example, phase1/pcb.c turns into ./build/phase1/pcb.o
# Separating src objects from main objects since phases can be shared with each other
# more specifically, a phase requires all the previous phases except the main
PHASE1_OBJS := $(patsubst %,$(BUILD_DIR)/%, $(subst $(SRC_DIR)/,, $(patsubst %.c,%.o, $(PHASE1_SRCS))))
PHASE2_OBJS := $(patsubst %,$(BUILD_DIR)/%, $(subst $(SRC_DIR)/,, $(patsubst %.c,%.o, $(PHASE2_SRCS))))
PHASE3_OBJS := $(patsubst %,$(BUILD_DIR)/%, $(subst $(SRC_DIR)/,, $(patsubst %.c,%.o, $(PHASE3_SRCS))))
PHASE1_MAIN_OBJ := $(patsubst %,$(BUILD_DIR)/%, $(subst $(SRC_DIR)/,, $(patsubst %.c,%.o, $(PHASE1_MAIN_SRC))))
PHASE2_MAIN_OBJ := $(patsubst %,$(BUILD_DIR)/%, $(subst $(SRC_DIR)/,, $(patsubst %.c,%.o, $(PHASE2_MAIN_SRC))))
PHASE3_MAIN_OBJ := $(patsubst %,$(BUILD_DIR)/%, $(subst $(SRC_DIR)/,, $(patsubst %.c,%.o, $(PHASE3_MAIN_SRC))))

# uMPS3-related paths
UMPS3_DATA_DIR := $(UMPS3_DIR_PREFIX)/share/umps3
UMPS3_INCLUDE_DIR := $(UMPS3_DIR_PREFIX)/include/umps3
PANDOS_INCLUDE_DIR := $(SRC_DIR)/h

# Compiler options
CFLAGS_LANG := -ffreestanding -std=c11 -I.
CFLAGS_MIPS := -mips1 -mabi=32 -mno-gpopt -G 0 -mno-abicalls -fno-pic -mfp32
CFLAGS := $(CFLAGS_LANG) $(CFLAGS_MIPS) -I$(UMPS3_INCLUDE_DIR) -I$(PANDOS_INCLUDE_DIR) -Wall -O0

# Linker options
LDFLAGS := -G 0 -nostdlib -T $(UMPS3_DATA_DIR)/umpscore.ldscript

# Add the location of crt*.S to the search path
VPATH := $(UMPS3_DATA_DIR)

.PHONY : all clean

all : clean phase3

phase3: $(PHASE1_OBJS) $(filter-out ./build/phase2/p2test.o, $(PHASE2_OBJS)) $(PHASE3_OBJS) $(PHASE3_MAIN_OBJ) $(BUILD_DIR)/crtso.o $(BUILD_DIR)/libumps.o
	$(LD) -o kernel $^ $(LDFLAGS)
	umps3-elf2umps -k kernel

# temp remove of p2test.o
phase2: $(PHASE1_OBJS) $(PHASE2_OBJS) $(PHASE2_MAIN_OBJ) $(BUILD_DIR)/crtso.o $(BUILD_DIR)/libumps.o
    #$(error $(filter-out ./build/phase2/p2test.o, $(PHASE2_OBJS)))
	$(LD) -o kernel $^ $(LDFLAGS)
	umps3-elf2umps -k kernel

phase1: $(PHASE1_OBJS) $(PHASE1_MAIN_OBJ) $(BUILD_DIR)/crtso.o $(BUILD_DIR)/libumps.o
	$(LD) -o kernel $^ $(LDFLAGS)
	umps3-elf2umps -k kernel

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: %.S
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f -r $(BUILD_DIR)
	rm -f kernel *.umps
