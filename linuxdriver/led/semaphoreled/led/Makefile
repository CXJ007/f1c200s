KERNELDIR := /home/cxj/Desktop/linux/linux-5.7.1
CURRENT_PATH := $(shell pwd)

obj-m := ledsemaphore.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean