CFLAGS= -fomit-frame-pointer -pipe -fno-common -fno-builtin \
	-Wall \
	-march=armv7-a -mfpu=neon -mfloat-abi=hard \
	-mthumb-interwork -mtune=cortex-a9 \
	-DEMBED -Dlinux -D__linux__ -Dunix -fPIC \
	--sysroot=$(PETA_STAGE)

INC=	-I$(PETA_STAGE)/usr/include \
	-I$(PETA_STAGE)/include

LDLIBS= -L$(PETA_STAGE)/lib \
	-L$(PETA_STAGE)/usr/lib \
	-L$(PETA_STAGE)/ncurses

LDFLAGS= -L$(PETA_STAGE)/lib \
	-L$(PETA_STAGE)/usr/lib \
	-L$(PETA_STAGE)/ncurses

CXX=arm-linux-gnueabihf-g++
CC=arm-linux-gnueabihf-gcc

