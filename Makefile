obj-m	:= galmem.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD)

clean:
	rm -rf *.o *~ core .depend .*.cmd *.mod.c .tmp_versions .module-common.o *.order *.symvers *.ko *.mod

