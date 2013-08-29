project = simple-gtk-drawspace

sources_dir = source

sources = main.cpp \
          simple-gtk-drawspace.cpp

CXXFLAGS += `pkg-config --cflags gtk+-3.0`
CXXFLAGS += `pkg-config --libs gtk+-3.0`

#CXXFLAGS += -DSGDS_DEBUG

all: $(project)

include Makefile.inc
