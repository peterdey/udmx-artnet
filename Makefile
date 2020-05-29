CC              = gcc
LIBUSB_CONFIG   = libusb-config

# Make sure that libusb-config is in the search path or specify a full path.
# On Windows, there is no libusb-config and you must configure the options
# below manually. See examples.
CFLAGS          = `$(LIBUSB_CONFIG) --cflags` -O -Wall -g
#CFLAGS          = -I/usr/local/libusb/include
# On Windows replace `$(LIBUSB_CONFIG) --cflags` with appropriate "-I..."
# option to ensure that usb.h is found

LIBS            = `$(LIBUSB_CONFIG) --libs` -lartnet
#LIBS            = `$(LIBUSB_CONFIG) --libs` -framework CoreFoundation
# You may need "-framework CoreFoundation" on Mac OS X and Darwin.
#LIBS            = -L/usr/local/libusb/lib/gcc -lusb
# On Windows use somthing similar to the line above.

all: udmx-artnet

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o
	rm -f udmx-artnet

udmx-artnet: udmx-artnet.o
	$(CC) -o udmx-artnet udmx-artnet.o $(LIBS)
