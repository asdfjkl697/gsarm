
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
#CFLAGS := $(CFLAGS) -g
LDFLAGS := -lhighgui -lx264 -lfaad -lfaac -lasound -luuid -lpthread -lstdc++ -lgloox -lsqlite3 -lrt 

# include rule.mk
include $(SRC_ROOT)/rule.mk

# define objects collection variable
gsh264 = h264_xu_ctrls.o v4l2uvc.o nalu.o cap_desc_parser.o cap_desc.o video.o 

# add your target(s) to all
.PHONY: all
all: gsh264

# define you target, the target will be output to dragonboard/output/bin
# directory
gsh264: $(gsh264)
	$(LINK_MSG)
	$(LINKX)

# change 'gsh264' to your objects collection variable
$(gsh264): %.o: %.c
	$(COMPILE_MSG)
	$(COMPILEX)

