obj-m += helloMod.o

# Set paths for cross-compilation
ARCH := arm
CROSS_COMPILE := arm-buildroot-linux-gnueabihf-

# Update this path to match your Buildroot output directory
KERNEL_DIR ?= ~/buildroot-2024.11.1/output/build/linux-custom

PWD := $(CURDIR)

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
