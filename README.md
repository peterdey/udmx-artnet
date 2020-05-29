udmx-artnet
===========
A simple ArtNet proxy for the Anyma uDMX USB to DMX512 Controller and generic clones

You can pick up cheap clones for [around AU$30](https://www.amazon.com/IMRELAX-Interface-Adapter-Controller-Conversion/dp/B07GFDDTHL/?tag=intermatrix-20).
Look for the ones with a silver USB memory-stick type dongle, and 3-pin XLR connector.

# Requirements
* LibArtnet - https://github.com/OpenLightingProject/libartnet
* LibUSB (``apt-get install libusb-dev``)

# Usage
```bash
  udmx-artnet [-v] [-d] [-a <ip-address>]
     -v              verbose, display interesting runtime info
     -d              debug, display debugging info
     -a <ip-address> IP address to listen on (defaults to all non-loopback interfaces)
```

# Acknowledgements
Inspired by Daniel Mack's [USB2DMX proxy](https://gist.github.com/zonque/10b7b7183519bf7d3112881cb31b6133) for FTDI DMX adapters

[uDMXArtnet](https://www.illutzmination.de/udmxartnet.html?&L=1) from ilLUTZmination.de did what I needed, but used a constant 2% CPU even when it wasn't doing anything.  There was also no author details, no source and no license supplied.

The code here is based largely on combining these two projects:
* Simon Newton's [Artnet RDM proxy](https://github.com/OpenLightingProject/artnet-examples/blob/master/src/artnet-rdm-output.c)
* Markus Baertschi's commandline [uDMX utility](https://github.com/markusb/uDMX-linux)

