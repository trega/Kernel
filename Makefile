obj-$(CONFIG_TREGA_SLABALLOCDRV) +=SlabAllocDrv.o

clean:
	rm -rf built-in.o .built-in.o.cmd modules.order Module.symvers .SlabAllocDrv.ko.cmd SlabAllocDrv.mod.c SlabAllocDrv.mod.o .SlabAllocDrv.mod.o.cmd SlabAllocDrv.o .SlabAllocDrv.o.cmd 
