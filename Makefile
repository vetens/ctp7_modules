ifndef PETA_STAGE
$(error "Error: PETA_STAGE environment variable not set.")
endif

ifndef CTP7_MOD_ROOT
$(error "Error: CTP7_MOD_ROOT environment variable not set. Source setup.sh file")
endif

include apps.common.mk

IncludeDirs = ${CTP7_MOD_ROOT}/include
IncludeDirs += ${CTP7_MOD_ROOT}/src
IncludeDirs += ${XHAL_ROOT}/xhalcore/include
IncludeDirs += ${XHAL_ROOT}/xcompile/xerces-c-3.1.4/src
IncludeDirs += ${XHAL_ROOT}/xcompile/log4cplus-1.1.2/include
IncludeDirs += ${XHAL_ROOT}/xcompile/lmdb-LMDB_0.9.19/include
IncludeDirs += /opt/cactus/include
INC=$(IncludeDirs:%=-I%)

LDFLAGS+= -L$(XHAL_ROOT)/lib/arm
LDFLAGS+= -L$(XHAL_ROOT)/xcompile/lmdb-LMDB_0.9.19/lib
LDFLAGS+= -L$(CTP7_MOD_ROOT)/lib

SRCS= $(shell echo ${CTP7_MOD_ROOT}/src/*.cpp)
#TARGET_LIBS=$(addprefix lib/,$(patsubst %.cpp, %.so, $(notdir $(wildcard ./src/*.cpp))))
TARGET_LIBS=lib/memory.so
TARGET_LIBS+=lib/optical.so
TARGET_LIBS+=lib/utils.so
TARGET_LIBS+=lib/extras.so
TARGET_LIBS+=lib/daq_monitor.so
TARGET_LIBS+=lib/amc.so
TARGET_LIBS+=lib/vfat3.so
TARGET_LIBS+=lib/optohybrid.so
TARGET_LIBS+=lib/calibration_routines.so

all: $(TARGET_LIBS)

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

clean:
	-rm -rf lib/*.so

.PHONY: all
