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

CTP7_MODULES_VER_MAJOR:=$(shell ./config/tag2rel.sh | awk '{split($$0,a," "); print a[1];}' | awk '{split($$0,b,":"); print b[2];}')
CTP7_MODULES_VER_MINOR:=$(shell ./config/tag2rel.sh | awk '{split($$0,a," "); print a[2];}' | awk '{split($$0,b,":"); print b[2];}')
CTP7_MODULES_VER_PATCH:=$(shell ./config/tag2rel.sh | awk '{split($$0,a," "); print a[3];}' | awk '{split($$0,b,":"); print b[2];}')

include $(BUILD_HOME)/$(Package)/config/mfZynq.mk
include $(BUILD_HOME)/$(Package)/config/mfCommonDefs.mk
include $(BUILD_HOME)/$(Package)/config/mfRPMRules.mk

IncludeDirs  = ${BUILD_HOME}/$(Package)/include
IncludeDirs += ${BUILD_HOME}/xhal/xhalcore/include
#IncludeDirs += /opt/cactus/include
IncludeDirs+= /opt/wiscrpcsvc/include
INC=$(IncludeDirs:%=-I%)

ifndef GEM_VARIANT
	GEM_VARIANT = ge11
endif

CFLAGS += -DGEM_VARIANT="${GEM_VARIANT}"

LDFLAGS+= -L${BUILD_HOME}/xhal/xhalarm/lib
LDFLAGS+= -L${BUILD_HOME}/$(Package)/lib
LDFLAGS+= -L/opt/wiscrpcsvc/lib

SRCS= $(shell echo ${BUILD_HOME}/${Package}/src/**/*.cpp)
TARGET_LIBS  = lib/memhub.so
TARGET_LIBS += lib/memory.so
TARGET_LIBS += lib/optical.so
TARGET_LIBS += lib/utils.so
TARGET_LIBS += lib/extras.so
TARGET_LIBS += lib/amc.so
TARGET_LIBS += lib/daq_monitor.so
TARGET_LIBS += lib/vfat3.so
TARGET_LIBS += lib/optohybrid.so
TARGET_LIBS += lib/calibration_routines.so
TARGET_LIBS += lib/gbt.so

.PHONY: clean rpc prerpm

default:
	@echo "Running default target"
	$(MakeDir) $(PackageDir)

_rpmprep: preprpm
	@echo "Running _rpmprep target"
preprpm: default
	@echo "Running preprpm target"
	@cp -rf lib $(PackageDir)

Objects = %(filter %.cpp, %(SRCS))
PackageTargetDir = lib/linux
TargetObjects = $(patsubst %.o $(PackageTargetDir)/%.o $(Objects) )

build: $(TARGET_LIBS)

_all: $(TARGET_LIBS)

# ### generate dependencies automatically, without needing to have specific rules for *every* file
# $(PackageTargetDir)/%.o: src/%.cpp include/%.h
# 	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) -fPIC -c -o $@ $<

# ### make this generic, per target lib
# ## need to also make the linking generic
# ## linking, ar or ld, (static or dynamic)?
# AMC_OBJ_FILES = $(wildcard $(PackageTargetDir)/amc/*.o)
# lib/amc.so: $(PackageTargetDir)/amc.o $(AMC_OBJ_FILES)
# 	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,amc.so -o $@ $< -l:utils -l:extras -lxhal -llmdb -lwisci2c -lmemsvc


lib/memhub.so: src/memhub.cpp 
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,memhub.so -o $@ $< -lwisci2c -lmemsvc 

lib/memory.so: src/memory.cpp 
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,memory.so -o $@ $< -lwisci2c -l:memhub.so

lib/optical.so: src/optical.cpp 
	$(CXX) $(CFLAGS) $(INC) $(LDFLAGS) -fPIC -shared -o $@ $< -lwisci2c

lib/utils.so: src/utils.cpp include/utils.h
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,utils.so -o $@ $< -lwisci2c -lxhal -llmdb -l:memhub.so

lib/extras.so: src/extras.cpp
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,extras.so -o $@ $< -lwisci2c -lxhal -llmdb -l:memhub.so

### AMC library dependencies #######################################################################################
lib/linux/amc/ttc.o: src/amc/ttc.cpp include/amc/ttc.h
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) -fPIC -c -o $@ $<

lib/linux/amc/daq.o: src/amc/daq.cpp include/amc/daq.h
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) -fPIC -c -o $@ $<

lib/linux/amc.o: src/amc.cpp include/amc.h
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) -fPIC -c -o $@ $<

lib/amc.so: lib/linux/amc/ttc.o lib/linux/amc/daq.o lib/linux/amc.o
	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,amc.so -o $@ $^ -l:utils.so -l:extras.so -lwisci2c -lxhal -llmdb
####################################################################################################################

lib/daq_monitor.so: src/daq_monitor.cpp
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,daq_monitor.so -o $@ $< -lwisci2c -lxhal -llmdb -l:utils.so -l:extras.so -l:amc.so

lib/vfat3.so: src/vfat3.cpp 
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,vfat3.so -o $@ $< -lwisci2c -lxhal -llmdb -l:utils.so -l:extras.so -l:amc.so

lib/optohybrid.so: src/optohybrid.cpp
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,optohybrid.so -o $@ $< -lwisci2c -lxhal -llmdb -l:utils.so -l:extras.so -l:amc.so

lib/calibration_routines.so: src/calibration_routines.cpp
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,calibration_routines.so -o $@ $< -lwisci2c -lxhal -llmdb -l:utils.so -l:extras.so -l:optohybrid.so -l:vfat3.so -l:amc.so

lib/gbt.so: src/gbt.cpp include/gbt.h include/hw_constants.h include/hw_constants_checks.h include/moduleapi.h include/memhub.h include/utils.h lib/utils.so
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,gbt.so -o $@ $< -l:memhub.so -l:utils.so

.PHONY: test
### local (PC) test functions, need standard gcc toolchain, dirs, and flags
test: test/tester.cpp
	g++ -O0 -g3 -fno-inline -o test/$@ $< $(INC) $(LDFLAGS) -lwiscrpcsvc

clean: cleanrpm
	-rm -rf lib/*.so
	-rm -rf $(PackageDir)

cleandoc: 
	@echo "TO DO"
