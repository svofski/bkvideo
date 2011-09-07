#DEFINES=-DBAUDRATE=230400
DEFINES=-DBAUDRATE=1500000

CFLAGS=$(DEFINES) -I/opt/local/include -x objective-c -framework Foundation -framework IOKit -L/opt/local/lib -lusb-1.0
#CXXFLAGS=-O3 `sdl-config --cflags` -x objective-c
#LDFLAGS=`sdl-config --libs` -lSDL_ttf -x objective-c -framework Foundation -framework IOKit -L/opt/local/lib -lusb-1.0

CXXFLAGS=$(DEFINES) -g -O3 `sdl-config --cflags` -I/opt/local/include 
LDFLAGS=`sdl-config --libs` -lSDL_ttf -framework Foundation -framework IOKit -framework OpenGL -L/opt/local/lib -lusb-1.0 
CC=g++

SNUU_OFILES=snuu-usb.o custom_async.o

all:    snuu-usb custom_async_bench

snuu-usb.o:	frame.h

clean:
	rm $(SNUU_OFILES) snuu-usb custom_async_bench

snuu-usb:   $(SNUU_OFILES)
