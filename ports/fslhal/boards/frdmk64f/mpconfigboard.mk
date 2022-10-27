MCU_SERIES = kinetis
CMSIS_MCU = mk64fn1M0xxx12
AF_FILE = boards/mk64fn1M0xxx12_af.csv
LD_FILE = boards/mk64fn1M0xxx12.ld
#LDFLAGS_MPCONFIG =  -Xlinker --gc-sections -Wl, -Xlinker -z  -Xlinker muldefs  -Xlinker --defsym=__ram_vector_table__=1  --specs=nano.specs          -Wall  -fno-common  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin  -Os  -mapcs  -Xlinker -static  -Xlinker --defsym=__stack_size__=0x1000  -Xlinker --defsym=__heap_size__=0x0400
CFLAGS_MPCONFIG = -DF_CPU=96000000 -DUSB_SERIAL -DCPU_MK64FN1M0VLL12
SRC_MPCONFIG_S = $(addprefix fslhal/$(BOARD)/,\
	startup_MK64F12.s \
	)
