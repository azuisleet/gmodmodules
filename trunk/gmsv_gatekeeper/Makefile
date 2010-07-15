# GateKeeper 4.2 Makefile

# Path configuration
SOURCE_SDK=..
SRCDS_DIR=~/srcds/orangebox

PROJECT_DIR=.
OUT_DIR=./bin
OBJ_DIR=./obj

# Compilation Configuration
CPP=/usr/bin/g++
LINK=/usr/bin/gcc
CPPLIBS=/usr/lib/gcc/x86_64-linux-gnu/4.4/32/libstdc++.a /usr/lib/gcc/x86_64-linux-gnu/4.4/32/libgcc_eh.a

USER_CFLAGS=-m32
USER_LFLAGS=-m32

OPTFLAGS=-O1 -fomit-frame-pointer -ffast-math -fforce-addr -funroll-loops -fthread-jumps -fcrossjumping -foptimize-sibling-calls -fcse-follow-jumps -fcse-skip-blocks -fgcse -fgcse-lm -fexpensive-optimizations -frerun-cse-after-loop -fcaller-saves -fpeephole2 -fschedule-insns2 -fsched-interblock -fsched-spec -fregmove -fstrict-overflow -fdelete-null-pointer-checks -freorder-blocks -freorder-functions -falign-functions -falign-jumps -falign-loops -falign-labels -ftree-vrp -ftree-pre -finline-functions -funswitch-loops -fgcse-after-reload

# Things down here probably don't need to be changed

# Link Libraries
# This is incompatible with the sdk as of July 13th 2010
# rename the files to look like this and it'll work.
# (Changes should be pretty obvious)
LIBFILES= \
  $(SOURCE_SDK)/lib/linux/tier1.a \
  $(SOURCE_SDK)/lib/linux/choreoobjects.a \
  $(SOURCE_SDK)/lib/linux/particles.a \
  $(SOURCE_SDK)/lib/linux/dmxloader.a \
  $(SRCDS_DIR)/bin/libtier0.so \
  $(SRCDS_DIR)/bin/libvstdlib.so \
  $(SRCDS_DIR)/bin/libsteam_api.so \
  $(SOURCE_SDK)/lib/linux/tier3.a \
  $(SOURCE_SDK)/lib/linux/tier2.a \
  $(SOURCE_SDK)/lib/linux/tier1.a \
  $(SOURCE_SDK)/lib/linux/mathlib.a \
  
GATEKEEPER_OBJS = \
	$(OBJ_DIR)/main.o \
	$(OBJ_DIR)/sigscan.o \

TIER0_OBJS = \
	$(OBJ_DIR)/tier0/memoverride.o \

TIER1_OBJS = \
	$(OBJ_DIR)/tier1/interface.o \
	$(OBJ_DIR)/tier1/KeyValues.o \
	$(OBJ_DIR)/tier1/utlbuffer.o \

INCLUDE=-I$(SOURCE_SDK)/public -I$(SOURCE_SDK)/common -I$(SOURCE_SDK)/public/tier0 -I$(SOURCE_SDK)/public/tier1 -I$(SOURCE_SDK)/tier1 

CFLAGS=$(USER_CFLAGS) $(OPTFLAGS) -mtune=i686 -march=pentium -mmmx -msse -pipe -Dstrcmpi=strcasecmp -D_alloca=alloca -DVPROF_LEVEL=1 -DSWDS -D_LINUX -DLINUX -DNDEBUG -fpermissive -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp  -Usprintf=use_Q_snprintf_instead_of_sprintf -Ustrncpy=use_Q_strncpy_instead -Ufopen=dont_use_fopen -UPROTECTED_THINGS_ENABLE
LFLAGS=$(USER_LFLAGS) -lm -ldl $(LIBFILES) $(LIBFILES) -shared -Wl

DO_CC=$(CPP) $(INCLUDE) -w $(CFLAGS) -DARCH=i486 -o $@ -c $<

all: dirs gatekeeper install

dirs:
	-mkdir $(OUT_DIR)
	-mkdir $(OBJ_DIR)
	-mkdir $(OBJ_DIR)/tier0
	-mkdir $(OBJ_DIR)/tier1

gatekeeper: $(GATEKEEPER_OBJS) $(TIER1_OBJS) $(TIER0_OBJS)
	$(LINK) -o $(OUT_DIR)/gmsv_gatekeeper_linux.dll $(GATEKEEPER_OBJS) $(TIER1_OBJS) $(TIER0_OBJS) $(CPPLIBS) $(LFLAGS) $(CPPLIBS)

install:
	cp $(OUT_DIR)/gmsv_gatekeeper_linux.dll $(SRCDS_DIR)/garrysmod/lua/includes/modules/

$(OBJ_DIR)/%.o: $(PROJECT_DIR)/%.cpp
	$(DO_CC)

$(OBJ_DIR)/tier0/%.o: $(SOURCE_SDK)/public/tier0/%.cpp
	$(DO_CC)

$(OBJ_DIR)/tier1/%.o: $(SOURCE_SDK)/tier1/%.cpp
	$(DO_CC)

clean:
	-rm -rf $(OBJ_DIR)