
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
#CFLAGS := $(CFLAGS) -g
#LDFLAGS := -lscript
#LIBS := -levent -lpthread -lcrypt -lxml2 -lstdc++ -lrt -lscript
#LIBS += -levent -lpthread -lcrypt -lxml2 -lstdc++ -lrt
LDFLAGS := -lstdc++ -lsupc++ -lxml2 -lcrypt -lpthread -levent -lrt
#LDFLAGS := -levent -lpthread -lcrypt -lxml2 -lsupc++ -lrt

# include rule.mk
include $(SRC_ROOT)/rule+.mk

# define objects collection variable
gsbox_objs = client.o packet.o rs232.o taskmanager.o tcpserver.o obsolete.o udpserver.o radiopacket.o netpacket.o radionetwork.o radiosynchandler.o hardware.o gsbox.o

# add your target(s) to all
.PHONY: all
all: gsbox

# define you target, the target will be output to dragonboard/output/bin
# directory
gsbox: $(gsbox_objs)
	$(LINK_MSG)
	$(LINKX)

# change 'gsbox_objs' to your objects collection variable
$(gsbox_objs): %.o: %.cpp
	$(COMPILE_MSG)
	$(COMPILEX)

