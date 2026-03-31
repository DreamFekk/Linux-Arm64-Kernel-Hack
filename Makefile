obj-m := Hack.o

Hack-objs := UMB.o

src := $(if $(filter /%,$(src)),$(src),$(srctree)/$(src))

KDIR := $(KDIR)
MDIR := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

$(info -- KDIR: $(KDIR))
$(info -- MDIR: $(MDIR))

ccflags-y += -Wno-implicit-function-declaration -Wno-strict-prototypes -Wno-int-conversion -Wno-gcc-compat
ccflags-y += -Wno-declaration-after-statement -Wno-unused-function -Wno-unused-variable

ccflags-y += -DBUILD_NO_CFI

all:
	make -C $(KDIR) M=$(MDIR) modules

clean:
	make -C $(KDIR) M=$(MDIR) clean

.PHONY: all clean
