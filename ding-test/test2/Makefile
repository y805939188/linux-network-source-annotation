obj-m:=test_afsock.o

KDIR:=/home/ding/ding-os/lib/modules/5.14.15/build
PWD:=$(shell pwd)
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	gcc test_client.c -o test_client.o -static
	gcc test_server.c -o test_server.o -static
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) modules clean
	rm -rf test_client.o
	rm -rf test_server.o