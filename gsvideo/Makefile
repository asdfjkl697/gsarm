
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
#CFLAGS := $(CFLAGS) -g
#LDFLAGS := -lscript
#LDFLAGS := -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -lopencv_videoio -lx264 -lfaad -lfaac -lasound -luuid -lpthread -lstdc++ -lgloox -lsqlite3 -lrt 
LDFLAGS := -lhighgui -lx264 -lfaad -lfaac -lasound -luuid -lpthread -lstdc++ -lgloox -lsqlite3 -lrt 
#jyc20170505 modify -lstdcpp -lstdc++ add -lrt

# include rule.mk
include $(SRC_ROOT)/rule+.mk

# define objects collection variable
gsvideo_objs = video.o \

# add your target(s) to all
.PHONY: all
all: gsvideo

# define you target, the target will be output to dragonboard/output/bin
# directory
gsvideo: $(gsvideo_objs)
	$(LINK_MSG)
	$(LINKX)
	
# change 'gsrobot_objs' to your objects collection variable
$(gsvideo_objs): %.o: %.cpp
	$(COMPILE_MSG)
	$(COMPILEX)

