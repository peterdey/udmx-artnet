/*
 *
 * A simple ArtNet proxy for the Anyma uDMX USB to DMX512 Controller and clones
 * Copyright (C) 2020 Peter Dey
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the LICENSE file for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (the LICENSE file).  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdint.h>
#include <artnet/artnet.h>

#include <usb.h>

#define cmd_SetSingleChannel 1
#define cmd_SetChannelRange 2
#define cmd_StartBootloader 0xf8
#define err_BadChannel 1
#define err_BadValue 2

// Vendor & Product ID used by Anyma uDMX Controller
#define USBDEV_SHARED_VENDOR 0x16C0
#define USBDEV_SHARED_PRODUCT 0x05DC

int debug = 0;
int verbose = 0 ;

/*
 * Called when we have dmx data pending
 */
int dmx_handler(artnet_node n, int port, void *h) {
    usb_dev_handle *handle = (usb_dev_handle *) h ;
    int nBytes;
    uint8_t *data ;
    int len;
    data = artnet_read_dmx(n, port, &len);
    int channel = 1;

    if (len == 1) {
        // Single value = we have a single <channel> / <value> pair
        if (debug) printf("Setting channel %d to %d\n", channel, data[len-1]);

        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                                 cmd_SetSingleChannel, data[len-1], channel-1, NULL, 0, 1000);
        if (nBytes < 0)
            fprintf(stderr, "USB error: %s\n", usb_strerror());
    }
    if (len > 1) {
        // More than 1 value to set, send the array
        if (debug) {
            printf("Setting channels %d-%d to", channel, channel + len - 1);
            for (int i=0; i<len; i++) printf(" %d",data[i]);
            printf("\n");
        }

        nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                                 cmd_SetChannelRange, len, channel - 1, (char *)data, len, 1000);
        if (nBytes < 0)
            fprintf(stderr, "USB error: %s\n", usb_strerror());
    }

    return 0;
}

static int usbGetStringAscii(usb_dev_handle *dev, int index, int langid, char *buf, int buflen) {
    char buffer[256];
    int rval, i;

    if ((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR,
                                (USB_DT_STRING << 8) + index, langid, buffer,
                                sizeof(buffer), 1000)) < 0)
        return rval;
    if (buffer[1] != USB_DT_STRING)
        return 0;
    if ((unsigned char)buffer[0] < rval)
        rval = (unsigned char)buffer[0];
    rval /= 2;
    // Lossy conversion to ISO Latin1
    for (i = 1; i < rval; i++) {
        if (i > buflen) // destination buffer overflow
            break;
        buf[i - 1] = buffer[2 * i];
        if (buffer[2 * i + 1] != 0) // outside of ISO Latin1 range
            buf[i - 1] = '?';
    }
    buf[i - 1] = 0;
    return i - 1;
}

/*
 * uDMX uses the free shared default VID/PID.
 * To avoid talking to some other device we check the vendor and
 * device strings returned.
 */
static usb_dev_handle *findDevice(void) {
    struct usb_bus *bus;
    struct usb_device *dev;
    char string[256];
    int len;
    usb_dev_handle *handle = 0;

    usb_find_busses();
    usb_find_devices();
    for (bus = usb_busses; bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == USBDEV_SHARED_VENDOR &&
                dev->descriptor.idProduct == USBDEV_SHARED_PRODUCT) {
                if (verbose) { printf("Found device with %x:%x\n",
                               USBDEV_SHARED_VENDOR, USBDEV_SHARED_PRODUCT); }

                // Open the device to query strings
                handle = usb_open(dev);
                if (!handle) {
                    fprintf(stderr, "Warning: cannot open USB device: %s\n", usb_strerror());
                    continue;
                }

                // Now find out whether the device actually is a uDMX
                len = usbGetStringAscii(handle, dev->descriptor.iManufacturer, 0x0409, string, sizeof(string));
                if (len < 0) {
                    fprintf(stderr, "Warning: cannot query manufacturer for device: %s\n", usb_strerror());
                    goto skipDevice;
                }
                if (verbose) { printf("Device vendor is %s\n",string); }
                if (strcmp(string, "www.anyma.ch") != 0)
                    goto skipDevice;

                len = usbGetStringAscii(handle, dev->descriptor.iProduct, 0x0409, string, sizeof(string));
                if (len < 0) {
                    fprintf(stderr, "Warning: cannot query product for device: %s\n", usb_strerror());
                    goto skipDevice;
                }
                if (verbose) { printf("Device product is %s\n",string); }
                if (strcmp(string, "uDMX") == 0)
                    break;

            skipDevice:
                usb_close(handle);
                handle = NULL;
            }
        }
        if (handle)
            break;
    }
    return handle;
}

static void usage(char *name) {
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "  %s [-v] [-d] [-a <ip-address>]\n", name);
    fprintf(stderr, "     -v              verbose, display interesting runtime info\n");
    fprintf(stderr, "     -d              debug, display debugging info\n");
    fprintf(stderr, "     -a <ip-address> IP address to listen on (defaults to all non-loopback interfaces)\n");
}

int main(int argc, char *argv[]) {
    usb_dev_handle *handle = NULL;
    artnet_node node ;
    char *ip_addr = NULL ;
    int optc ;

    // Parse options
    while ((optc = getopt (argc, argv, "dhva:")) != EOF) {
        switch  (optc) {
            case 'h':
                usage(argv[0]);
                exit(1);
             case 'a':
                ip_addr = (char *) strdup(optarg) ;
                break;
            case 'v':
                verbose = 1 ;
                break;
            case 'd':
                debug = 1 ;
                break;
              default:
                break;
            }
    }

    usb_set_debug(0);

    // USB Initialisation: Exit if we can not find the uDMX device
    usb_init();
    handle = findDevice();
    if (handle == NULL) {
        fprintf(stderr, "Could not find USB device www.anyma.ch/uDMX (vid=0x%x pid=0x%x)\n", USBDEV_SHARED_VENDOR, USBDEV_SHARED_PRODUCT);
        exit(1);
    }


    node = artnet_new(ip_addr, verbose);

    artnet_set_short_name(node, "udmx-artnet") ;
    artnet_set_long_name(node, "uDMX ArtNet Proxy");
    artnet_set_node_type(node, ARTNET_NODE) ;

    // Set the first port to output dmx data
    artnet_set_port_type(node, 0, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX) ;
    artnet_set_subnet_addr(node, 0x00) ;
    artnet_set_bcast_limit(node, 16);

    // Set the universe address of the first port
    artnet_set_port_addr(node, 0, ARTNET_OUTPUT_PORT, 0x00) ;

    artnet_set_dmx_handler(node, dmx_handler, handle);
    artnet_start(node) ;

    // Loop until Ctrl+C
    while(1) {
        artnet_read(node, 1) ;
    }

    // Never reached
    artnet_destroy(node) ;
    usb_close(handle);

    return 0;
}
