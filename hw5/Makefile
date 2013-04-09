obj-m := minix.o
minix-y := bitmap.o dir.o file.o inode.o itree_v1.o itree_v2.o namei.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
 
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
