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

IncludeDirs = ${BUILD_HOME}/$(Package)/include
IncludeDirs+= /opt/xhal/include
#IncludeDirs+= /opt/cactus/include
IncludeDirs+= /opt/wiscrpcsvc/include
INC=$(IncludeDirs:%=-I%)

ifndef GEM_VARIANT
GEM_VARIANT = ge11
endif

CFLAGS += -DGEM_VARIANT="${GEM_VARIANT}"

LDFLAGS+= -L${BUILD_HOME}/$(Package)/lib
LDFLAGS+= -L/opt/xhal/lib/arm
LDFLAGS+= -L/opt/wiscrpcsvc/lib

.PHONY: clean rpc prerpm

default:
	@echo "Running default target"
	$(MakeDir) $(PackageDir)

_rpmprep: preprpm
	@echo "Running _rpmprep target"

preprpm: default
	@echo "Running preprpm target"
	@cp -rf lib $(PackageDir)

PackageSourceDir=$(BUILD_HOME)/$(Project)/src
PackageIncludeDir=$(BUILD_HOME)/$(Project)/include
PackageLibraryDir=$(BUILD_HOME)/$(Project)/lib
PackageObjectDir=$(PackageSourceDir)/linux/$(Arch)
Sources        := $(wildcard $(PackageSourceDir)/*.cpp) $(wildcard $(PackageSourceDir)/*/*.cpp)
TargetObjects  := $(subst $(BUILD_HOME)/$(Project)/src/,, $(patsubst %.cpp,%.o, $(Sources)))
# TargetObjects  := $(subst $(BUILD_HOME)/$(Project)/src/,$(BUILD_HOME)/$(Project)/lib/linux/, $(Sources:%.cpp=%.o))
# TargetObjects  := $(subst $(BUILD_HOME)/$(Project)/src/,$(BUILD_HOME)/$(Project)/lib/linux/, $(patsubst %.cpp,%.o, $(Sources)))

## Specific groupings of functionality: all objects should be built into at most one of the libraries
TargetLibraries:= memhub memory optical utils extras amc daq_monitor vfat3 optohybrid calibration_routines

$(info Sources is $(Sources))
$(info TargetObjects is $(TargetObjects))
$(info TargetLibraries is $(TargetLibraries))

# Everything links against these three
BASE_LINKS = -lxhal -llmdb -lwisci2c

.PHONY: $(TargetObjects)
.PHONY: $(TargetLibraries)

## Build all object files the same way, they don't require any special treatment for now
## * only if their dependencies become complicated do we need special rules
$(TargetObjects): $(wildcard $(PackageSourceDir)/$(patsubst %.o,, $@)/*.cpp) $(wildcard $(PackageIncludeDir)/$(patsubst %.o,, $@)/*.h)
	$(MakeDir) $(shell dirname $(PackageObjectDir)/$@)
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) -fPIC -c -o $(PackageObjectDir)/$@ $(patsubst %.o,%.cpp, $(PackageSourceDir)/$@)

## This needs to be able to dynamically determine dependent targets and link time dependencies before it can be used
# $(TargetLibraries): $(TargetObjects) $(wildcard $(patsubst %.o,%, $(TargetObjects))/*.o)
# $(TargetLibraries): % : %.o %/*.o $(wildcard %.o,%, $(TargetObjects))/*.o)
# 	$(MakeDir) $(PackageLibraryDir)
# 	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,$@.so -o $(PackageLibraryDir)/$@.so $(PackageObjectDir)/$@.o $(wildcard $(PackageObjectDir)/$@/*.o) $(BASE_LINKS) -lmemsvc

## Further generalization to this would be fantastic, but may not be possible
# * rename libraries to lib<name>.so so that we can link with -l<name>?
# * fix to change dependencies to file targets rather than phonies, so that a rebuild won't always touch everything?

memhub: memhub.o
	$(MakeDir) $(PackageLibraryDir)
	@echo Executing memhub stage
	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,$@.so -o $(PackageLibraryDir)/$@.so $(PackageObjectDir)/$< $(BASE_LINKS) -lmemsvc
memory: memory.o memhub
	$(MakeDir) $(PackageLibraryDir)
	@echo Executing memory stage
	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,$@.so -o $(PackageLibraryDir)/$@.so $(PackageObjectDir)/$< $(BASE_LINKS) -lmemhub
optical: optical.o memhub
	$(MakeDir) $(PackageLibraryDir)
	@echo Executing optical stage
	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,$@.so -o $(PackageLibraryDir)/$@.so $(PackageObjectDir)/$< $(BASE_LINKS) -lmemhub
utils : utils.o memhub
	$(MakeDir) $(PackageLibraryDir)
	@echo Executing utils stage
	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,$@.so -o $(PackageLibraryDir)/$@.so $(PackageObjectDir)/$< $(BASE_LINKS) -lmemhub
extras: extras.o memhub utils
	$(MakeDir) $(PackageLibraryDir)
	@echo Executing extras stage
	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,$@.so -o $(PackageLibraryDir)/$@.so $(PackageObjectDir)/$@.o $(wildcard $(PackageObjectDir)/$@/*.o) $(BASE_LINKS) -l:lib/utils.so
amc: % : %/ttc.o %/daq.o %.o utils extras
	$(MakeDir) $(PackageLibraryDir)
	@echo Executing amc stage
	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,$@.so -o $(PackageLibraryDir)/$@.so $(PackageObjectDir)/$@.o $(wildcard $(PackageObjectDir)/$@/*.o) $(BASE_LINKS) -l:lib/utils.so -l:lib/extras.so
daq_monitor: daq_monitor.o amc
	$(MakeDir) $(PackageLibraryDir)
	@echo Executing daq_monitor stage
	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,$@.so -o $(PackageLibraryDir)/$@.so $(PackageObjectDir)/$@.o $(wildcard $(PackageObjectDir)/$@/*.o) $(BASE_LINKS) -l:lib/utils.so -l:lib/extras.so -l:lib/amc.so
vfat3:  vfat3.o amc
	$(MakeDir) $(PackageLibraryDir)
	@echo Executing vfat3 stage
	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,$@.so -o $(PackageLibraryDir)/$@.so $(PackageObjectDir)/$@.o $(wildcard $(PackageObjectDir)/$@/*.o) $(BASE_LINKS) -l:lib/utils.so -l:lib/extras.so -l:lib/amc.so
optohybrid: optohybrid.o amc
	$(MakeDir) $(PackageLibraryDir)
	@echo Executing optohybrid stage
	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,$@.so -o $(PackageLibraryDir)/$@.so $(PackageObjectDir)/$@.o $(wildcard $(PackageObjectDir)/$@/*.o) $(BASE_LINKS) -l:lib/utils.so -l:lib/extras.so -l:lib/amc.so
calibration_routines: calibration_routines.o optohybrid vfat3
	$(MakeDir) $(PackageLibraryDir)
	@echo Executing calibration_routines stage
	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,$@.so -o $(PackageLibraryDir)/$@.so $(PackageObjectDir)/$@.o $(wildcard $(PackageObjectDir)/$@/*.o) $(BASE_LINKS) -l:lib/utils.so -l:lib/extras.so -l:lib/optohybrid.so -l:lib/vfat3.so -l:lib/amc.so

build: $(TargetLibraries)
	@echo Executing build stage

_all: $(TargetLibraries)
	@echo Executing _all stage

# SRCS= $(shell echo ${BUILD_HOME}/${Package}/src/**/*.cpp)
# SRCS= $(shell find ${BUILD_HOME}/${Package}/src -iname '*.cpp')
# TARGET_LIBS  = lib/memhub.so
# TARGET_LIBS += lib/memory.so
# TARGET_LIBS += lib/optical.so
# TARGET_LIBS += lib/utils.so
# TARGET_LIBS += lib/extras.so
# TARGET_LIBS += lib/amc.so
# TARGET_LIBS += lib/daq_monitor.so
# TARGET_LIBS += lib/vfat3.so
# TARGET_LIBS += lib/optohybrid.so
# TARGET_LIBS += lib/calibration_routines.so

# $(info SRCS is $(SRCS))
# $(info TARGET_LIBS is $(TARGET_LIBS))

# build: $(TARGET_LIBS)

# _all: $(TARGET_LIBS)

# ## TODO
# ### generate dependencies automatically, without needing to have specific rules for *every* file
# $(PackageObjectDir)/%.o: src/%.cpp include/%.h
# 	@echo "Running $(CXX) on $^ to create $@"
# 	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) -fPIC -c -o $@ $^

# ### make this generic, per target lib
# ## need to also make the linking generic
# ## linking, ar or ld, (static or dynamic)?
# OBJ_FILES := $(wildcard $(PackageObjectDir)/*.o) $(wildcard $(PackageObjectDir)/**/*.o)
# AMC_OBJ_FILES := $(wildcard $(PackageObjectDir)/amc/*.o)
# # lib/amc.so: $(PackageObjectDir)/amc.o $(AMC_OBJ_FILES)
# # 	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,amc.so -o $@ $^ -l:utils -l:extras $(BASE_LINKS) -lmemsvc

# # lib/%.so: $(filter %.o, $(OBJ_FILES))
# # lib/memhub.so: src/memhub.cpp
# lib/memhub.so: $(filter %memhub%, $(OBJ_FILES))
# 	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,memhub.so -o $@ $< -lwisci2c -lmemsvc
# 	# $(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,memhub.so -o $@ $< -lwisci2c -lmemsvc

# lib/memory.so: src/memory.cpp
# 	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,memory.so -o $@ $< -lwisci2c -l:memhub.so

# lib/optical.so: src/optical.cpp
# 	$(CXX) $(CFLAGS) $(INC) $(LDFLAGS) -fPIC -shared -o $@ $< -lwisci2c

# lib/utils.so: src/utils.cpp include/utils.h
# 	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,utils.so -o $@ $< -lwisci2c -lxhal -llmdb -l:memhub.so

# lib/extras.so: src/extras.cpp
# 	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,extras.so -o $@ $< -lwisci2c -lxhal -llmdb -l:memhub.so

# ### AMC library dependencies #######################################################################################
# lib/linux/amc/ttc.o: src/amc/ttc.cpp include/amc/ttc.h
# 	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) -fPIC -c -o $@ $<

# lib/linux/amc/daq.o: src/amc/daq.cpp include/amc/daq.h
# 	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) -fPIC -c -o $@ $<

# lib/linux/amc.o: src/amc.cpp include/amc.h
# 	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) -fPIC -c -o $@ $<

# lib/amc.so: lib/linux/amc/ttc.o lib/linux/amc/daq.o lib/linux/amc.o
# 	$(CXX) $(LDFLAGS) -fPIC -shared -Wl,-soname,amc.so -o $@ $^ -l:utils.so -l:extras.so -lwisci2c -lxhal -llmdb
# ####################################################################################################################

# lib/daq_monitor.so: src/daq_monitor.cpp
# 	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,daq_monitor.so -o $@ $< -lwisci2c -lxhal -llmdb -l:utils.so -l:extras.so -l:amc.so

# lib/vfat3.so: src/vfat3.cpp
# 	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,vfat3.so -o $@ $< -lwisci2c -lxhal -llmdb -l:utils.so -l:extras.so -l:amc.so

# lib/optohybrid.so: src/optohybrid.cpp
# 	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,optohybrid.so -o $@ $< -lwisci2c -lxhal -llmdb -l:utils.so -l:extras.so -l:amc.so

# lib/calibration_routines.so: src/calibration_routines.cpp
# 	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,calibration_routines.so -o $@ $< -lwisci2c -lxhal -llmdb -l:utils.so -l:extras.so -l:optohybrid.so -l:vfat3.so -l:amc.so

lib/gbt.so: src/gbt.cpp include/gbt.h include/hw_constants.h include/hw_constants_checks.h include/moduleapi.h include/memhub.h include/utils.h lib/utils.so
	$(CXX) $(CFLAGS) -std=c++1y -O3 -pthread $(INC) $(LDFLAGS) -fPIC -shared -Wl,-soname,gbt.so -o $@ $< -l:memhub.so -l:utils.so

.PHONY: test
### local (PC) test functions, need standard gcc toolchain, dirs, and flags
test: test/tester.cpp
	g++ -O0 -g3 -fno-inline -o test/$@ $< $(INC) $(LDFLAGS) -lwiscrpcsvc

clean: cleanrpm
	@echo Cleaning up all built files
	-rm -rf $(PackageDir)
	-rm -rf src/linux
	-rm -rf lib

cleandoc:
	@echo "TO DO"
