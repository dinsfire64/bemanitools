avsdlls			+= unicorntail

ldflags_unicorntail   := \
    -lws2_32 \

deplibs_unicorntail	:= \
	avs \

libs_unicorntail	:= \
	ddrhook-util \
	p3io \
	p3ioemu \
	hook \
	hooklib \
	util \
	cconfig \
	security \

src_unicorntail		:= \
	dllmain.c \
	p3io.c \
	usbmem.c \
	config.c \

