ifndef PETA_STAGE
$(error "Error: PETA_STAGE environment variable not set.")
endif

BUILD_HOME   := $(shell dirname `pwd`)
Project      := ctp7_modules
Package      := ctp7_modules
ShortPackage := ctp7_modules
LongPackage  := ctp7_modules
PackageName  := $(ShortPackage)
PackagePath  := $(shell pwd)
PackageDir   := pkg/$(ShortPackage)
Arch         := arm
Packager     := Mykhailo Dalchenko

CTP7_MODULES_VER_MAJOR=1
CTP7_MODULES_VER_MINOR=0
CTP7_MODULES_VER_PATCH=0

include $(BUILD_HOME)//$(Package)/config/mfZynq.mk
include $(BUILD_HOME)//$(Package)/config/mfCommonDefs.mk
include $(BUILD_HOME)//$(Package)/config/mfRPMRules.mk

IncludeDirs  = ${BUILD_HOME}/$(Package)/include
IncludeDirs += ${BUILD_HOME}/xhal/xhalcore/include
#IncludeDirs += /opt/cactus/include
INC=$(IncludeDirs:%=-I%)

LDFLAGS+= -L${BUILD_HOME}/xhal/xhalarm/lib
LDFLAGS+= -L${BUILD_HOME}/$(Package)/lib

SRCS= $(shell echo ${BUILD_HOME}/${Package}/src/*.cpp)
TARGET_LIBS  = lib/memory.so
TARGET_LIBS += lib/optical.so
TARGET_LIBS += lib/utils.so
TARGET_LIBS += lib/extras.so
TARGET_LIBS += lib/vfat3.so
TARGET_LIBS += lib/optohybrid.so
TARGET_LIBS += lib/calibration_routines.so
TARGET_LIBS += lib/amc.so
TARGET_LIBS += lib/daq_monitor.so

.PHONY: clean rpc prerpm

default:
	@echo "Running default target"
	$(MakeDir) $(PackageDir)

_rpmprep: preprpm
	@echo "Running _rpmprep target"
preprpm: default
	@echo "Running preprpm target"
	@cp -rf lib $(PackageDir)

build: $(TARGET_LIBS)

_all: $(TARGET_LIBS)

lib/memory.so: src/memory.cpp 
	$(CXX) $(CFLAGS) $(INC) $(LDFLAGS) -fPIC -shared -o $@ $< -lwisci2c

lib/optical.so: src/optical.cpp 
	$(CXX) $(CFLAGS) $(INC) $(LDFLAGS) -fPIC -shared -o $@ $< -lwisci2c

lib/utils.so: src/utils.cpp
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,utils.so -o $@ $< -lwisci2c -lxhal -llmdb

lib/extras.so: src/extras.cpp
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,extras.so -o $@ $< -lwisci2c -lxhal -llmdb

lib/daq_monitor.so: src/daq_monitor.cpp
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,daq_monitor.so -o $@ $< -lwisci2c -lxhal -llmdb -l:utils.so -l:extras.so

lib/amc.so: src/amc.cpp
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,amc.so -o $@ $< -lwisci2c -lxhal -llmdb -l:utils.so -l:extras.so

lib/vfat3.so: src/vfat3.cpp 
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,vfat3.so -o $@ $< -lwisci2c -lxhal -llmdb -l:utils.so -l:extras.so -l:amc.so

lib/optohybrid.so: src/optohybrid.cpp
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,optohybrid.so -o $@ $< -lwisci2c -lxhal -llmdb -l:utils.so -l:extras.so -l:amc.so

lib/calibration_routines.so: src/calibration_routines.cpp
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,calibration_routines.so -o $@ $< -lwisci2c -lxhal -llmdb -l:utils.so -l:extras.so -l:optohybrid.so -l:vfat3.so -l:amc.so

clean: cleanrpm
	-rm -rf lib/*.so
	-rm -rf $(PackageDir)

cleandoc: 
	@echo "TO DO"
