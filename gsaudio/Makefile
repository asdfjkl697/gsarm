
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
#CFLAGS := $(CFLAGS) -g
#LDFLAGS := -lscript
LDFLAGS := -lhighgui -lx264 -lfaad -lfaac -lasound -luuid -lpthread -lstdc++ -lgloox -lsqlite3 -lrt 
#jyc20170505 modify -lstdcpp -lstdc++ add -lrt

# include rule.mk
include $(SRC_ROOT)/rule+.mk

# define objects collection variable
gsaudio_objs = player.o faadtest.o recorder.o \
main.o

# add your target(s) to all
.PHONY: all
all: gsaudio

# define you target, the target will be output to dragonboard/output/bin
# directory
gsaudio: $(gsaudio_objs)
	$(LINK_MSG)
	$(LINKX)
	
# change 'gsrobot_objs' to your objects collection variable
$(gsaudio_objs): %.o: %.cpp
	$(COMPILE_MSG)
	$(COMPILEX)

