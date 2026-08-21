SRCDIR ?= /opt/fpp/src
include $(SRCDIR)/makefiles/common/setup.mk
include $(SRCDIR)/makefiles/platform/*.mk

all: libfpp-artnet-prop-control.$(SHLIB_EXT)
debug: all

OBJECTS_artnet_prop_control_so += src/ArtNetPropControl.o
LIBS_artnet_prop_control_so += -L$(SRCDIR) -lfpp
CXXFLAGS_src/ArtNetPropControl.o += -I$(SRCDIR)

%.o: %.cpp Makefile
	$(CCACHE) $(CC) $(CFLAGS) $(CXXFLAGS) $(CXXFLAGS_$@) -c $< -o $@

libfpp-artnet-prop-control.$(SHLIB_EXT): $(OBJECTS_artnet_prop_control_so) $(SRCDIR)/libfpp.$(SHLIB_EXT)
	$(CCACHE) $(CC) -shared $(CFLAGS_$@) $(OBJECTS_artnet_prop_control_so) $(LIBS_artnet_prop_control_so) $(LDFLAGS) -o $@

clean:
	rm -f libfpp-artnet-prop-control.so libfpp-artnet-prop-control.dylib $(OBJECTS_artnet_prop_control_so)
