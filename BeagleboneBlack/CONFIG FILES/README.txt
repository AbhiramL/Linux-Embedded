TO USE CONFIG FILES:

The config files must be either 

1. renamed to '.config' in their respective folders:
	buildroot.config : ~/buildroot/
	uboot.config     : ~/buildroot/output/build/uboot-version/
	busybox.config   : ~/buildroot/output/build/busybox-version/
	Linux.config     : ~/buildroot/output/build/linux-version/

OR

2. loaded using gui by calling make menuconfig(ex. make uboot-menuconfig, make busybox-menuconfig, make linux-menuconfig)
	-to accomplish this do:
		-copy the files to the respective folders highlighted at 1.
		-from ~/buildroot/ folder, call the make menuconfig command
		-do load, and put the config file name
		-once loaded, do save and change the saved file's name to '.config' so it will be its default configuration when buildroot builds