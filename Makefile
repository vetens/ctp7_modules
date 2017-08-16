ifndef PETA_STAGE
$(error "Error: PETA_STAGE environment variable not set.")
endif

ifndef CTP7_MOD_ROOT
$(error "Error: CTP7_MOD_ROOT environment variable not set. Source setup.sh file")
endif


include apps.common.mk

IncludeDirs = ${CTP7_MOD_ROOT}/include
IncludeDirs += ${CTP7_MOD_ROOT}/src
IncludeDirs += ${XHAL_ROOT}/include
IncludeDirs += ${XHAL_ROOT}/xcompile/xerces-c-3.1.4/src
IncludeDirs += ${XHAL_ROOT}/xcompile/log4cplus-1.1.2/include
IncludeDirs += ${XHAL_ROOT}/xcompile/lmdb-LMDB_0.9.19/include
IncludeDirs += /opt/cactus/include
INC=$(IncludeDirs:%=-I%)



LDFLAGS+= -L$(XHAL_ROOT)/lib
LDFLAGS+= -L$(XHAL_ROOT)/xcompile/lmdb-LMDB_0.9.19/lib

SRCS= $(shell echo ${CTP7_MOD_ROOT}/src/*.cpp)
TARGET_LIBS=$(addprefix lib/,$(patsubst %.cpp, %.so, $(notdir $(wildcard ./src/*.cpp))))

all: $(TARGET_LIBS)

lib/optical.so: src/optical.cpp 
	$(CXX) $(CFLAGS) $(INC) $(LDFLAGS) -fPIC -shared -o $@ $< -lwisci2c

#$(TARGET_LIBS): $(SRCS)
lib/%.so:src/%.cpp
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -o $@ $^ -lwisci2c -lxhal_ctp7 -llmdb

clean:
	-rm -rf lib/*.so

.PHONY: all

