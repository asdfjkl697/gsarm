
# define sources root directory before everything
SRC_ROOT := ..

# include rule.mk
include $(SRC_ROOT)/rule+.mk

# add you test case here
.PHONY: all
all:
	cp script_parser.sh $(BINDIR)/
	cp send_cmd_pipe.sh $(BINDIR)/
	#make -C example
	#make -C gsbox
	#make -C gsiot
	make -C gsrobot
	#make -C gsaudio
	#make -C gsvideo
	make -C gsh264
	#make -C cameratester
