ifndef PETA_STAGE
$(error "Error: PETA_STAGE environment variable not set.")
endif

ifndef CTP7_MOD_ROOT
$(error "Error: CTP7_MOD_ROOT environment variable not set. Source setup.sh file")
endif


include apps.common.mk

INC+= -I$(CTP7_MOD_ROOT)/include
INC+= -I$(XHAL_ROOT)/include
LDFLAGS+= -L$(XHAL_ROOT)/lib

SRCS= $(shell echo ${CTP7_MOD_ROOT}/src/*.cpp)
TARGET_LIBS=$(addprefix lib/,$(patsubst %.cpp, %.so, $(notdir $(wildcard ./src/*.cpp))))

all: $(TARGET_LIBS)

lib/optical.so: src/optical.cpp 
	$(CXX) $(CFLAGS) $(INC) $(LDFLAGS) -fPIC -shared -o $@ $< -lwisci2c

$(TARGET_LIBS): $(SRCS)
	$(CXX) $(CFLAGS) $(INC) $(LDFLAGS) -fPIC -shared -o $@ $< -lwisci2c -lxhal_ctp7

clean:
	-rm -rf lib/*.so

.PHONY: all

