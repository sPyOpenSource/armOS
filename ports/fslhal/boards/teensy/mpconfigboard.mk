MCU_SERIES = kinetis
CMSIS_MCU = m20dx256
AF_FILE = boards/mk20dx256_af.csv
LD_FILE = boards/mk20dx256.ld
#LDFLAGS_MPCONFIG = -msoft-float -mfloat-abi=soft
CFLAGS_MPCONFIG = -DF_CPU=96000000 -DUSB_SERIAL -D__MK20DX256__
