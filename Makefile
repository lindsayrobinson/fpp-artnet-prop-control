SRCDIR ?= /opt/fpp/src

include $(SRCDIR)/makefiles/common/setup.mk
include $(SRCDIR)/makefiles/platform/*.mk

TARGET = libfpp-artnet-prop-control.$(SHLIB_EXT)
OBJECTS_artnet_prop_control_so += src/ArtNetPropControl.o
LIBS_artnet_prop_control_so += -L$(SRCDIR) -lfpp
CXXFLAGS_src/ArtNetPropControl.o += -I$(SRCDIR)

all: $(TARGET)
debug: all

%.o: %.cpp Makefile
	$(CCACHE) $(CC) $(CFLAGS) $(CXXFLAGS) $(CXXFLAGS_$@) -c $< -o $@

$(TARGET): $(OBJECTS_artnet_prop_control_so) $(SRCDIR)/libfpp.$(SHLIB_EXT)
	$(CCACHE) $(CC) -shared $(CFLAGS) $(CXXFLAGS) $(OBJECTS_artnet_prop_control_so) $(LIBS_artnet_prop_control_so) $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET) $(OBJECTS_artnet_prop_control_so)
