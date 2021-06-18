obj-m+=hidefile.o
 
all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	$(CC) user_space.c -o hidding

load:
	insmod hidefile.ko

unload:
	rmmod hidefile

hide:
	./hidding

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm hidding
