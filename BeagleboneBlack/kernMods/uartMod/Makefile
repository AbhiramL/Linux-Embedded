obj-m += uartMod.o

# Set paths for cross-compilation
ARCH := arm
CROSS_COMPILE := arm-buildroot-linux-gnueabihf-

# Update this path to match your Buildroot output directory
KERNEL_DIR ?= ~/buildroot-2024.11/output/build/linux-custom

PWD := $(CURDIR)

# Name the user program executable
USER_PROGRAM := userPgrm

# Define paths for the user-space program source file
USER_PROGRAM_SRC := $(PWD)/userPgrm.c

# Add the rules for kernel module compilation
all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD) modules
	$(CROSS_COMPILE)gcc -o $(USER_PROGRAM) $(USER_PROGRAM_SRC)  # Build user-space program

# Clean both kernel module and user-space program
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f $(USER_PROGRAM)  # Remove the compiled user-space program
