/***************************************************************************
                          ftdi.c  -  description
                             -------------------
    begin                : Fri Apr 4 2003
    copyright            : (C) 2003-2008 by Intra2net AG
    email                : opensource@intra2net.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License           *
 *   version 2.1 as published by the Free Software Foundation;             *
 *                                                                         *
 ***************************************************************************/

/**
    \mainpage libftdi API documentation

    Library to talk to FTDI chips. You find the latest versions of libftdi at
    http://www.intra2net.com/en/developer/libftdi/

    The library is easy to use. Have a look at this short example:
    \include simple.c

    More examples can be found in the "examples" directory.
*/
/** \addtogroup libftdi */
/* @{ */

#include <libusb.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "ftdi.h"

#define ftdi_error_return(code, str) do {  \
        ftdi->error_str = str;             \
        return code;                       \
   } while(0);


/**
    Internal function to close usb device pointer.
    Sets ftdi->usb_dev to NULL.
    \internal

    \param ftdi pointer to ftdi_context

    \retval none
*/
static void ftdi_usb_close_internal (struct ftdi_context *ftdi)
{
    if (ftdi->usb_dev)
    {
       libusb_close (ftdi->usb_dev);
       ftdi->usb_dev = NULL;
    }
}

/**
    Initializes a ftdi_context.

    \param ftdi pointer to ftdi_context

    \retval  0: all fine
    \retval -1: couldn't allocate read buffer

    \remark This should be called before all functions
*/
int ftdi_init(struct ftdi_context *ftdi)
{
    ftdi->usb_dev = NULL;
    ftdi->usb_read_timeout = 5000;
    ftdi->usb_write_timeout = 5000;

    ftdi->type = TYPE_BM;    /* chip type */
    ftdi->baudrate = -1;
    ftdi->bitbang_enabled = 0;  /* 0: normal mode 1: any of the bitbang modes enabled */

    ftdi->readbuffer = NULL;
    ftdi->readbuffer_offset = 0;
    ftdi->readbuffer_remaining = 0;
    ftdi->writebuffer_chunksize = 4096;
    ftdi->max_packet_size = 0;

    ftdi->interface = 0;
    ftdi->index = 0;
    ftdi->in_ep = 0x02;
    ftdi->out_ep = 0x81;
    ftdi->bitbang_mode = 1; /* when bitbang is enabled this holds the number of the mode  */

    ftdi->error_str = NULL;

    ftdi->eeprom_size = FTDI_DEFAULT_EEPROM_SIZE;

    /* All fine. Now allocate the readbuffer */
    return ftdi_read_data_set_chunksize(ftdi, 4096);
}

/**
    Allocate and initialize a new ftdi_context

    \return a pointer to a new ftdi_context, or NULL on failure
*/
struct ftdi_context *ftdi_new(void)
{
    struct ftdi_context * ftdi = (struct ftdi_context *)malloc(sizeof(struct ftdi_context));

    if (ftdi == NULL)
    {
        return NULL;
    }

    if (ftdi_init(ftdi) != 0)
    {
        free(ftdi);
        return NULL;
    }

    return ftdi;
}

/**
    Open selected channels on a chip, otherwise use first channel.

    \param ftdi pointer to ftdi_context
    \param interface Interface to use for FT2232C/2232H/4232H chips.

    \retval  0: all fine
    \retval -1: unknown interface
*/
int ftdi_set_interface(struct ftdi_context *ftdi, enum ftdi_interface interface)
{
    switch (interface)
    {
        case INTERFACE_ANY:
        case INTERFACE_A:
            /* ftdi_usb_open_desc cares to set the right index, depending on the found chip */
            break;
        case INTERFACE_B:
            ftdi->interface = 1;
            ftdi->index     = INTERFACE_B;
            ftdi->in_ep     = 0x04;
            ftdi->out_ep    = 0x83;
            break;
        case INTERFACE_C:
            ftdi->interface = 2;
            ftdi->index     = INTERFACE_C;
            ftdi->in_ep     = 0x06;
            ftdi->out_ep    = 0x85;
            break;
        case INTERFACE_D:
            ftdi->interface = 3;
            ftdi->index     = INTERFACE_D;
            ftdi->in_ep     = 0x08;
            ftdi->out_ep    = 0x87;
            break;
        default:
            ftdi_error_return(-1, "Unknown interface");
    }
    return 0;
}

/**
    Deinitializes a ftdi_context.

    \param ftdi pointer to ftdi_context
*/
void ftdi_deinit(struct ftdi_context *ftdi)
{
    ftdi_usb_close_internal (ftdi);

    if (ftdi->readbuffer != NULL)
    {
        free(ftdi->readbuffer);
        ftdi->readbuffer = NULL;
    }
}

/**
    Deinitialize and free an ftdi_context.

    \param ftdi pointer to ftdi_context
*/
void ftdi_free(struct ftdi_context *ftdi)
{
    ftdi_deinit(ftdi);
    free(ftdi);
}

/**
    Use an already open libusb device.

    \param ftdi pointer to ftdi_context
    \param usb libusb libusb_device_handle to use
*/
void ftdi_set_usbdev (struct ftdi_context *ftdi, libusb_device_handle *usb)
{
    ftdi->usb_dev = usb;
}


/**
    Finds all ftdi devices on the usb bus. Creates a new ftdi_device_list which
    needs to be deallocated by ftdi_list_free() after use.

    \param ftdi pointer to ftdi_context
    \param devlist Pointer where to store list of found devices
    \param vendor Vendor ID to search for
    \param product Product ID to search for

    \retval >0: number of devices found
    \retval -3: out of memory
    \retval -4: libusb_init() failed
    \retval -5: libusb_get_device_list() failed
    \retval -6: libusb_get_device_descriptor() failed
*/
int ftdi_usb_find_all(struct ftdi_context *ftdi, struct ftdi_device_list **devlist, int vendor, int product)
{
    struct ftdi_device_list **curdev;
    libusb_device *dev;
    libusb_device **devs;
    int count = 0;
    int i = 0;

    if (libusb_init(NULL) < 0)
        ftdi_error_return(-4, "libusb_init() failed");

    if (libusb_get_device_list(NULL, &devs) < 0)
        ftdi_error_return(-5, "libusb_get_device_list() failed");

    curdev = devlist;
    *curdev = NULL;

    while ((dev = devs[i++]) != NULL)
    {
        struct libusb_device_descriptor desc;

        if (libusb_get_device_descriptor(dev, &desc) < 0)
            ftdi_error_return(-6, "libusb_get_device_descriptor() failed");

        if (desc.idVendor == vendor && desc.idProduct == product)
        {
            *curdev = (struct ftdi_device_list*)malloc(sizeof(struct ftdi_device_list));
            if (!*curdev)
                ftdi_error_return(-3, "out of memory");
	      
            (*curdev)->next = NULL;
            (*curdev)->dev = dev;

            curdev = &(*curdev)->next;
            count++;
        }
    }

    return count;
}

/**
    Frees a usb device list.

    \param devlist USB device list created by ftdi_usb_find_all()
*/
void ftdi_list_free(struct ftdi_device_list **devlist)
{
    struct ftdi_device_list *curdev, *next;

    for (curdev = *devlist; curdev != NULL;)
    {
        next = curdev->next;
        free(curdev);
        curdev = next;
    }

    *devlist = NULL;
}

/**
    Frees a usb device list.

    \param devlist USB device list created by ftdi_usb_find_all()
*/
void ftdi_list_free2(struct ftdi_device_list *devlist)
{
    ftdi_list_free(&devlist);
}

/**
    Return device ID strings from the usb device.

    The parameters manufacturer, description and serial may be NULL
    or pointer to buffers to store the fetched strings.

    \note Use this function only in combination with ftdi_usb_find_all()
          as it closes the internal "usb_dev" after use.

    \param ftdi pointer to ftdi_context
    \param dev libusb usb_dev to use
    \param manufacturer Store manufacturer string here if not NULL
    \param mnf_len Buffer size of manufacturer string
    \param description Store product description string here if not NULL
    \param desc_len Buffer size of product description string
    \param serial Store serial string here if not NULL
    \param serial_len Buffer size of serial string

    \retval   0: all fine
    \retval  -1: wrong arguments
    \retval  -4: unable to open device
    \retval  -7: get product manufacturer failed
    \retval  -8: get product description failed
    \retval  -9: get serial number failed
    \retval -11: libusb_get_device_descriptor() failed
*/
int ftdi_usb_get_strings(struct ftdi_context * ftdi, struct libusb_device * dev,
                         char * manufacturer, int mnf_len, char * description, int desc_len, char * serial, int serial_len)
{
    struct libusb_device_descriptor desc;

    if ((ftdi==NULL) || (dev==NULL))
        return -1;

    if (libusb_open(dev, &ftdi->usb_dev) < 0)
        ftdi_error_return(-4, "libusb_open() failed");

    if (libusb_get_device_descriptor(dev, &desc) < 0)
        ftdi_error_return(-11, "libusb_get_device_descriptor() failed");

    if (manufacturer != NULL)
    {
        if (libusb_get_string_descriptor_ascii(ftdi->usb_dev, desc.iManufacturer, (unsigned char *)manufacturer, mnf_len) < 0)
        {
            ftdi_usb_close_internal (ftdi);
            ftdi_error_return(-7, "libusb_get_string_descriptor_ascii() failed");
        }
    }

    if (description != NULL)
    {
        if (libusb_get_string_descriptor_ascii(ftdi->usb_dev, desc.iProduct, (unsigned char *)description, desc_len) < 0)
        {
            ftdi_usb_close_internal (ftdi);
            ftdi_error_return(-8, "libusb_get_string_descriptor_ascii() failed");
        }
    }

    if (serial != NULL)
    {
        if (libusb_get_string_descriptor_ascii(ftdi->usb_dev, desc.iSerialNumber, (unsigned char *)serial, serial_len) < 0)
        {
            ftdi_usb_close_internal (ftdi);
            ftdi_error_return(-9, "libusb_get_string_descriptor_ascii() failed");
        }
    }

    ftdi_usb_close_internal (ftdi);

    return 0;
}

/**
 * Internal function to determine the maximum packet size.
 * \param ftdi pointer to ftdi_context
 * \param dev libusb usb_dev to use
 * \retval Maximum packet size for this device
 */
static unsigned int _ftdi_determine_max_packet_size(struct ftdi_context *ftdi, libusb_device *dev)
{
    struct libusb_device_descriptor desc;
    struct libusb_config_descriptor *config0;
    unsigned int packet_size;

    // Determine maximum packet size. Init with default value.
    // New hi-speed devices from FTDI use a packet size of 512 bytes
    // but could be connected to a normal speed USB hub -> 64 bytes packet size.
    if (ftdi->type == TYPE_2232H || ftdi->type == TYPE_4232H)
        packet_size = 512;
    else
        packet_size = 64;

    if (libusb_get_device_descriptor(dev, &desc) < 0)
        return packet_size;

    if (libusb_get_config_descriptor(dev, 0, &config0) < 0)
        return packet_size;

    if (desc.bNumConfigurations > 0)
    {
        if (ftdi->interface < config0->bNumInterfaces)
        {
            struct libusb_interface interface = config0->interface[ftdi->interface];
            if (interface.num_altsetting > 0)
            {
                struct libusb_interface_descriptor descriptor = interface.altsetting[0];
                if (descriptor.bNumEndpoints > 0)
                {
                    packet_size = descriptor.endpoint[0].wMaxPacketSize;
                }
            }
        }
    }

    libusb_free_config_descriptor (config0);
    return packet_size;
}

/**
    Opens a ftdi device given by an usb_device.

    \param ftdi pointer to ftdi_context
    \param dev libusb usb_dev to use

    \retval  0: all fine
    \retval -3: unable to config device
    \retval -4: unable to open device
    \retval -5: unable to claim device
    \retval -6: reset failed
    \retval -7: set baudrate failed
    \retval -9: libusb_get_device_descriptor() failed
    \retval -10: libusb_get_config_descriptor() failed
    \retval -11: libusb_etach_kernel_driver() failed
    \retval -12: libusb_get_configuration() failed
*/
int ftdi_usb_open_dev(struct ftdi_context *ftdi, libusb_device *dev)
{
    struct libusb_device_descriptor desc;
    struct libusb_config_descriptor *config0;
    int cfg, cfg0;

    if (libusb_open(dev, &ftdi->usb_dev) < 0)
        ftdi_error_return(-4, "libusb_open() failed");

    if (libusb_get_device_descriptor(dev, &desc) < 0)
        ftdi_error_return(-9, "libusb_get_device_descriptor() failed");

    if (libusb_get_config_descriptor(dev, 0, &config0) < 0)
        ftdi_error_return(-10, "libusb_get_config_descriptor() failed");
    cfg0 = config0->bConfigurationValue;
    libusb_free_config_descriptor (config0);

#ifdef LIBUSB_HAS_GET_DRIVER_NP
    // Try to detach ftdi_sio kernel module.
    // Returns ENODATA if driver is not loaded.
    //
    // The return code is kept in a separate variable and only parsed
    // if usb_set_configuration() or usb_claim_interface() fails as the
    // detach operation might be denied and everything still works fine.
    // Likely scenario is a static ftdi_sio kernel module.
    ret = libusb_detach_kernel_driver(ftdi->usb_dev, ftdi->interface);
    if (ret < 0 && ret != LIBUSB_ERROR_NOT_FOUND)
        ftdi_error_return(-11, "libusb_detach_kernel_driver () failed");
#endif

    if (libusb_get_configuration (ftdi->usb_dev, &cfg) < 0)
        ftdi_error_return(-12, "libusb_get_configuration () failed");

    // set configuration (needed especially for windows)
    // tolerate EBUSY: one device with one configuration, but two interfaces
    //    and libftdi sessions to both interfaces (e.g. FT2232)
    if (desc.bNumConfigurations > 0 && cfg != cfg0)
    {
        if (libusb_set_configuration(ftdi->usb_dev, cfg0) < 0)
        {
            ftdi_usb_close_internal (ftdi);
            ftdi_error_return(-3, "unable to set usb configuration. Make sure ftdi_sio is unloaded!");
        }
    }

    if (libusb_claim_interface(ftdi->usb_dev, ftdi->interface) < 0)
    {
        ftdi_usb_close_internal (ftdi);
        ftdi_error_return(-5, "unable to claim usb device. Make sure ftdi_sio is unloaded!");
    }

    if (ftdi_usb_reset (ftdi) != 0)
    {
        ftdi_usb_close_internal (ftdi);
        ftdi_error_return(-6, "ftdi_usb_reset failed");
    }

    // Try to guess chip type
    // Bug in the BM type chips: bcdDevice is 0x200 for serial == 0
    if (desc.bcdDevice == 0x400 || (desc.bcdDevice == 0x200
            && desc.iSerialNumber == 0))
        ftdi->type = TYPE_BM;
    else if (desc.bcdDevice == 0x200)
        ftdi->type = TYPE_AM;
    else if (desc.bcdDevice == 0x500)
        ftdi->type = TYPE_2232C;
    else if (desc.bcdDevice == 0x600)
        ftdi->type = TYPE_R;
    else if (desc.bcdDevice == 0x700)
        ftdi->type = TYPE_2232H;
    else if (desc.bcdDevice == 0x800)
        ftdi->type = TYPE_4232H;

    // Set default interface on dual/quad type chips
    switch(ftdi->type)
    {
        case TYPE_2232C:
        case TYPE_2232H:
        case TYPE_4232H:
            if (!ftdi->index)
                ftdi->index = INTERFACE_A;
            break;
        default:
            break;
    }

    // Determine maximum packet size
    ftdi->max_packet_size = _ftdi_determine_max_packet_size(ftdi, dev);

    if (ftdi_set_baudrate (ftdi, 9600) != 0)
    {
        ftdi_usb_close_internal (ftdi);
        ftdi_error_return(-7, "set baudrate failed");
    }

    ftdi_error_return(0, "all fine");
}

/**
    Opens the first device with a given vendor and product ids.

    \param ftdi pointer to ftdi_context
    \param vendor Vendor ID
    \param product Product ID

    \retval same as ftdi_usb_open_desc()
*/
int ftdi_usb_open(struct ftdi_context *ftdi, int vendor, int product)
{
    return ftdi_usb_open_desc(ftdi, vendor, product, NULL, NULL);
}

/**
    Opens the first device with a given, vendor id, product id,
    description and serial.

    \param ftdi pointer to ftdi_context
    \param vendor Vendor ID
    \param product Product ID
    \param description Description to search for. Use NULL if not needed.
    \param serial Serial to search for. Use NULL if not needed.

    \retval  0: all fine
    \retval -3: usb device not found
    \retval -4: unable to open device
    \retval -5: unable to claim device
    \retval -6: reset failed
    \retval -7: set baudrate failed
    \retval -8: get product description failed
    \retval -9: get serial number failed
    \retval -11: libusb_init() failed
    \retval -12: libusb_get_device_list() failed
    \retval -13: libusb_get_device_descriptor() failed
*/
int ftdi_usb_open_desc(struct ftdi_context *ftdi, int vendor, int product,
                       const char* description, const char* serial)
{
    return ftdi_usb_open_desc_index(ftdi,vendor,product,description,serial,0);
}

/**
    Opens the index-th device with a given, vendor id, product id,
    description and serial.

    \param ftdi pointer to ftdi_context
    \param vendor Vendor ID
    \param product Product ID
    \param description Description to search for. Use NULL if not needed.
    \param serial Serial to search for. Use NULL if not needed.
    \param index Number of matching device to open if there are more than one, starts with 0.

    \retval  0: all fine
    \retval -1: usb_find_busses() failed
    \retval -2: usb_find_devices() failed
    \retval -3: usb device not found
    \retval -4: unable to open device
    \retval -5: unable to claim device
    \retval -6: reset failed
    \retval -7: set baudrate failed
    \retval -8: get product description failed
    \retval -9: get serial number failed
    \retval -10: unable to close device
*/
int ftdi_usb_open_desc_index(struct ftdi_context *ftdi, int vendor, int product,
                       const char* description, const char* serial, unsigned int index)
{
    libusb_device *dev;
    libusb_device **devs;
    char string[256];
    int i = 0;

    if (libusb_init(NULL) < 0)
        ftdi_error_return(-11, "libusb_init() failed");

    if (libusb_get_device_list(NULL, &devs) < 0)
        ftdi_error_return(-12, "libusb_get_device_list() failed");

    while ((dev = devs[i++]) != NULL)
    {
        struct libusb_device_descriptor desc;

        if (libusb_get_device_descriptor(dev, &desc) < 0)
            ftdi_error_return(-13, "libusb_get_device_descriptor() failed");

        if (desc.idVendor == vendor && desc.idProduct == product)
        {
            if (libusb_open(dev, &ftdi->usb_dev) < 0)
                ftdi_error_return(-4, "usb_open() failed");

            if (description != NULL)
            {
                if (libusb_get_string_descriptor_ascii(ftdi->usb_dev, desc.iProduct, (unsigned char *)string, sizeof(string)) < 0)
                {
                    libusb_close (ftdi->usb_dev);
                    ftdi_error_return(-8, "unable to fetch product description");
                }
                if (strncmp(string, description, sizeof(string)) != 0)
                {
                    libusb_close (ftdi->usb_dev);
                    continue;
                }
            }
            if (serial != NULL)
            {
                if (libusb_get_string_descriptor_ascii(ftdi->usb_dev, desc.iSerialNumber, (unsigned char *)string, sizeof(string)) < 0)
                {
                    ftdi_usb_close_internal (ftdi);
                    ftdi_error_return(-9, "unable to fetch serial number");
                }
                if (strncmp(string, serial, sizeof(string)) != 0)
                {
                    ftdi_usb_close_internal (ftdi);
                    continue;
                }
            }

            ftdi_usb_close_internal (ftdi);

                if (index > 0)
                {
                    index--;
                    continue;
                }

            return ftdi_usb_open_dev(ftdi, dev);
        }
    }

    // device not found
    ftdi_error_return(-3, "device not found");
}

/**
    Opens the ftdi-device described by a description-string.
    Intended to be used for parsing a device-description given as commandline argument.

    \param ftdi pointer to ftdi_context
    \param description NULL-terminated description-string, using this format:
        \li <tt>d:\<devicenode></tt> path of bus and device-node (e.g. "003/001") within usb device tree (usually at /proc/bus/usb/)
        \li <tt>i:\<vendor>:\<product></tt> first device with given vendor and product id, ids can be decimal, octal (preceded by "0") or hex (preceded by "0x")
        \li <tt>i:\<vendor>:\<product>:\<index></tt> as above with index being the number of the device (starting with 0) if there are more than one
        \li <tt>s:\<vendor>:\<product>:\<serial></tt> first device with given vendor id, product id and serial string

    \note The description format may be extended in later versions.

    \retval  0: all fine
    \retval -1: libusb_init() failed
    \retval -2: libusb_get_device_list() failed
    \retval -3: usb device not found
    \retval -4: unable to open device
    \retval -5: unable to claim device
    \retval -6: reset failed
    \retval -7: set baudrate failed
    \retval -8: get product description failed
    \retval -9: get serial number failed
    \retval -10: unable to close device
    \retval -11: illegal description format
*/
int ftdi_usb_open_string(struct ftdi_context *ftdi, const char* description)
{
    if (description[0] == 0 || description[1] != ':')
        ftdi_error_return(-11, "illegal description format");

    if (description[0] == 'd')
    {
        libusb_device *dev;
        libusb_device **devs;
	unsigned int bus_number, device_address;
	int i = 0;

        if (libusb_init (NULL) < 0)
	    ftdi_error_return(-1, "libusb_init() failed");

	if (libusb_get_device_list(NULL, &devs) < 0)
	    ftdi_error_return(-2, "libusb_get_device_list() failed");

        /* XXX: This doesn't handle symlinks/odd paths/etc... */
        if (sscanf (description + 2, "%u/%u", &bus_number, &device_address) != 2)
	    ftdi_error_return(-11, "illegal description format");

	while ((dev = devs[i++]) != NULL)
        {
	    if (bus_number == libusb_get_bus_number (dev)
		&& device_address == libusb_get_device_address (dev))
                return ftdi_usb_open_dev(ftdi, dev);
        }

        // device not found
        ftdi_error_return(-3, "device not found");
    }
    else if (description[0] == 'i' || description[0] == 's')
    {
        unsigned int vendor;
        unsigned int product;
        unsigned int index=0;
        const char *serial=NULL;
        const char *startp, *endp;

        errno=0;
        startp=description+2;
        vendor=strtoul((char*)startp,(char**)&endp,0);
        if (*endp != ':' || endp == startp || errno != 0)
            ftdi_error_return(-11, "illegal description format");

        startp=endp+1;
        product=strtoul((char*)startp,(char**)&endp,0);
        if (endp == startp || errno != 0)
            ftdi_error_return(-11, "illegal description format");

        if (description[0] == 'i' && *endp != 0)
        {
            /* optional index field in i-mode */
            if (*endp != ':')
                ftdi_error_return(-11, "illegal description format");

            startp=endp+1;
            index=strtoul((char*)startp,(char**)&endp,0);
            if (*endp != 0 || endp == startp || errno != 0)
                ftdi_error_return(-11, "illegal description format");
        }
        if (description[0] == 's')
        {
            if (*endp != ':')
                ftdi_error_return(-11, "illegal description format");

            /* rest of the description is the serial */
            serial=endp+1;
        }

        return ftdi_usb_open_desc_index(ftdi, vendor, product, NULL, serial, index);
    }
    else
    {
        ftdi_error_return(-11, "illegal description format");
    }
}

/**
    Resets the ftdi device.

    \param ftdi pointer to ftdi_context

    \retval  0: all fine
    \retval -1: FTDI reset failed
*/
int ftdi_usb_reset(struct ftdi_context *ftdi)
{
    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                SIO_RESET_REQUEST, SIO_RESET_SIO,
                                ftdi->index, NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-1,"FTDI reset failed");

    // Invalidate data in the readbuffer
    ftdi->readbuffer_offset = 0;
    ftdi->readbuffer_remaining = 0;

    return 0;
}

/**
    Clears the read buffer on the chip and the internal read buffer.

    \param ftdi pointer to ftdi_context

    \retval  0: all fine
    \retval -1: read buffer purge failed
*/
int ftdi_usb_purge_rx_buffer(struct ftdi_context *ftdi)
{
    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                SIO_RESET_REQUEST, SIO_RESET_PURGE_RX,
                                ftdi->index, NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-1, "FTDI purge of RX buffer failed");

    // Invalidate data in the readbuffer
    ftdi->readbuffer_offset = 0;
    ftdi->readbuffer_remaining = 0;

    return 0;
}

/**
    Clears the write buffer on the chip.

    \param ftdi pointer to ftdi_context

    \retval  0: all fine
    \retval -1: write buffer purge failed
*/
int ftdi_usb_purge_tx_buffer(struct ftdi_context *ftdi)
{
    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                SIO_RESET_REQUEST, SIO_RESET_PURGE_TX,
                                ftdi->index, NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-1, "FTDI purge of TX buffer failed");

    return 0;
}

/**
    Clears the buffers on the chip and the internal read buffer.

    \param ftdi pointer to ftdi_context

    \retval  0: all fine
    \retval -1: read buffer purge failed
    \retval -2: write buffer purge failed
*/
int ftdi_usb_purge_buffers(struct ftdi_context *ftdi)
{
    int result;

    result = ftdi_usb_purge_rx_buffer(ftdi);
    if (result < 0)
        return -1;

    result = ftdi_usb_purge_tx_buffer(ftdi);
    if (result < 0)
        return -2;

    return 0;
}



/**
    Closes the ftdi device. Call ftdi_deinit() if you're cleaning up.

    \param ftdi pointer to ftdi_context

    \retval  0: all fine
    \retval -1: usb_release failed
*/
int ftdi_usb_close(struct ftdi_context *ftdi)
{
    int rtn = 0;

    if (ftdi->usb_dev != NULL)
        if (libusb_release_interface(ftdi->usb_dev, ftdi->interface) < 0)
            rtn = -1;

    ftdi_usb_close_internal (ftdi);

    return rtn;
}

/**
    ftdi_convert_baudrate returns nearest supported baud rate to that requested.
    Function is only used internally
    \internal
*/
static int ftdi_convert_baudrate(int baudrate, struct ftdi_context *ftdi,
                                 unsigned short *value, unsigned short *index)
{
    static const char am_adjust_up[8] = {0, 0, 0, 1, 0, 3, 2, 1};
    static const char am_adjust_dn[8] = {0, 0, 0, 1, 0, 1, 2, 3};
    static const char frac_code[8] = {0, 3, 2, 4, 1, 5, 6, 7};
    int divisor, best_divisor, best_baud, best_baud_diff;
    unsigned long encoded_divisor;
    int i;

    if (baudrate <= 0)
    {
        // Return error
        return -1;
    }

    divisor = 24000000 / baudrate;

    if (ftdi->type == TYPE_AM)
    {
        // Round down to supported fraction (AM only)
        divisor -= am_adjust_dn[divisor & 7];
    }

    // Try this divisor and the one above it (because division rounds down)
    best_divisor = 0;
    best_baud = 0;
    best_baud_diff = 0;
    for (i = 0; i < 2; i++)
    {
        int try_divisor = divisor + i;
        int baud_estimate;
        int baud_diff;

        // Round up to supported divisor value
        if (try_divisor <= 8)
        {
            // Round up to minimum supported divisor
            try_divisor = 8;
        }
        else if (ftdi->type != TYPE_AM && try_divisor < 12)
        {
            // BM doesn't support divisors 9 through 11 inclusive
            try_divisor = 12;
        }
        else if (divisor < 16)
        {
            // AM doesn't support divisors 9 through 15 inclusive
            try_divisor = 16;
        }
        else
        {
            if (ftdi->type == TYPE_AM)
            {
                // Round up to supported fraction (AM only)
                try_divisor += am_adjust_up[try_divisor & 7];
                if (try_divisor > 0x1FFF8)
                {
                    // Round down to maximum supported divisor value (for AM)
                    try_divisor = 0x1FFF8;
                }
            }
            else
            {
                if (try_divisor > 0x1FFFF)
                {
                    // Round down to maximum supported divisor value (for BM)
                    try_divisor = 0x1FFFF;
                }
            }
        }
        // Get estimated baud rate (to nearest integer)
        baud_estimate = (24000000 + (try_divisor / 2)) / try_divisor;
        // Get absolute difference from requested baud rate
        if (baud_estimate < baudrate)
        {
            baud_diff = baudrate - baud_estimate;
        }
        else
        {
            baud_diff = baud_estimate - baudrate;
        }
        if (i == 0 || baud_diff < best_baud_diff)
        {
            // Closest to requested baud rate so far
            best_divisor = try_divisor;
            best_baud = baud_estimate;
            best_baud_diff = baud_diff;
            if (baud_diff == 0)
            {
                // Spot on! No point trying
                break;
            }
        }
    }
    // Encode the best divisor value
    encoded_divisor = (best_divisor >> 3) | (frac_code[best_divisor & 7] << 14);
    // Deal with special cases for encoded value
    if (encoded_divisor == 1)
    {
        encoded_divisor = 0;    // 3000000 baud
    }
    else if (encoded_divisor == 0x4001)
    {
        encoded_divisor = 1;    // 2000000 baud (BM only)
    }
    // Split into "value" and "index" values
    *value = (unsigned short)(encoded_divisor & 0xFFFF);
    if (ftdi->type == TYPE_2232C || ftdi->type == TYPE_2232H || ftdi->type == TYPE_4232H)
    {
        *index = (unsigned short)(encoded_divisor >> 8);
        *index &= 0xFF00;
        *index |= ftdi->index;
    }
    else
        *index = (unsigned short)(encoded_divisor >> 16);

    // Return the nearest baud rate
    return best_baud;
}

/**
    Sets the chip baud rate

    \param ftdi pointer to ftdi_context
    \param baudrate baud rate to set

    \retval  0: all fine
    \retval -1: invalid baudrate
    \retval -2: setting baudrate failed
*/
int ftdi_set_baudrate(struct ftdi_context *ftdi, int baudrate)
{
    unsigned short value, index;
    int actual_baudrate;

    if (ftdi->bitbang_enabled)
    {
        baudrate = baudrate*4;
    }

    actual_baudrate = ftdi_convert_baudrate(baudrate, ftdi, &value, &index);
    if (actual_baudrate <= 0)
        ftdi_error_return (-1, "Silly baudrate <= 0.");

    // Check within tolerance (about 5%)
    if ((actual_baudrate * 2 < baudrate /* Catch overflows */ )
            || ((actual_baudrate < baudrate)
                ? (actual_baudrate * 21 < baudrate * 20)
                : (baudrate * 21 < actual_baudrate * 20)))
        ftdi_error_return (-1, "Unsupported baudrate. Note: bitbang baudrates are automatically multiplied by 4");

    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_BAUDRATE_REQUEST, value,
                                index, NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return (-2, "Setting new baudrate failed");

    ftdi->baudrate = baudrate;
    return 0;
}

/**
    Set (RS232) line characteristics.
    The break type can only be set via ftdi_set_line_property2()
    and defaults to "off".

    \param ftdi pointer to ftdi_context
    \param bits Number of bits
    \param sbit Number of stop bits
    \param parity Parity mode

    \retval  0: all fine
    \retval -1: Setting line property failed
*/
int ftdi_set_line_property(struct ftdi_context *ftdi, enum ftdi_bits_type bits,
                           enum ftdi_stopbits_type sbit, enum ftdi_parity_type parity)
{
    return ftdi_set_line_property2(ftdi, bits, sbit, parity, BREAK_OFF);
}

/**
    Set (RS232) line characteristics

    \param ftdi pointer to ftdi_context
    \param bits Number of bits
    \param sbit Number of stop bits
    \param parity Parity mode
    \param break_type Break type

    \retval  0: all fine
    \retval -1: Setting line property failed
*/
int ftdi_set_line_property2(struct ftdi_context *ftdi, enum ftdi_bits_type bits,
                            enum ftdi_stopbits_type sbit, enum ftdi_parity_type parity,
                            enum ftdi_break_type break_type)
{
    unsigned short value = bits;

    switch (parity)
    {
        case NONE:
            value |= (0x00 << 8);
            break;
        case ODD:
            value |= (0x01 << 8);
            break;
        case EVEN:
            value |= (0x02 << 8);
            break;
        case MARK:
            value |= (0x03 << 8);
            break;
        case SPACE:
            value |= (0x04 << 8);
            break;
    }

    switch (sbit)
    {
        case STOP_BIT_1:
            value |= (0x00 << 11);
            break;
        case STOP_BIT_15:
            value |= (0x01 << 11);
            break;
        case STOP_BIT_2:
            value |= (0x02 << 11);
            break;
    }

    switch (break_type)
    {
        case BREAK_OFF:
            value |= (0x00 << 14);
            break;
        case BREAK_ON:
            value |= (0x01 << 14);
            break;
    }

    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_DATA_REQUEST, value,
                                ftdi->index, NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return (-1, "Setting new line property failed");

    return 0;
}

/**
    Writes data in chunks (see ftdi_write_data_set_chunksize()) to the chip

    \param ftdi pointer to ftdi_context
    \param buf Buffer with the data
    \param size Size of the buffer

    \retval <0: error code from usb_bulk_write()
    \retval >0: number of bytes written
*/
int ftdi_write_data(struct ftdi_context *ftdi, unsigned char *buf, int size)
{
    int offset = 0;
    int actual_length;

    while (offset < size)
    {
        int write_size = ftdi->writebuffer_chunksize;

        if (offset+write_size > size)
            write_size = size-offset;

        if (libusb_bulk_transfer(ftdi->usb_dev, ftdi->in_ep, buf+offset, write_size, &actual_length, ftdi->usb_write_timeout) < 0)
            ftdi_error_return(-1, "usb bulk write failed");

        offset += actual_length;
    }

    return offset;
}

#ifdef LIBFTDI_LINUX_ASYNC_MODE
#ifdef USB_CLASS_PTP
#error LIBFTDI_LINUX_ASYNC_MODE is not compatible with libusb-compat-0.1!
#endif
static void LIBUSB_CALL ftdi_read_data_cb(struct libusb_transfer *transfer)
{
    struct ftdi_transfer_control *tc = (struct ftdi_transfer_control *) transfer->user_data;
    struct ftdi_context *ftdi = tc->ftdi;
    int packet_size, actual_length, num_of_chunks, chunk_remains, i, ret;

    // New hi-speed devices from FTDI use a packet size of 512 bytes
    if (ftdi->type == TYPE_2232H || ftdi->type == TYPE_4232H)
        packet_size = 512;
    else
        packet_size = 64;

    actual_length = transfer->actual_length;

    if (actual_length > 2)
    {
        // skip FTDI status bytes.
        // Maybe stored in the future to enable modem use
        num_of_chunks = actual_length / packet_size;
        chunk_remains = actual_length % packet_size;
        //printf("actual_length = %X, num_of_chunks = %X, chunk_remains = %X, readbuffer_offset = %X\n", actual_length, num_of_chunks, chunk_remains, ftdi->readbuffer_offset);

        ftdi->readbuffer_offset += 2;
        actual_length -= 2;

        if (actual_length > packet_size - 2)
        {
            for (i = 1; i < num_of_chunks; i++)
              memmove (ftdi->readbuffer+ftdi->readbuffer_offset+(packet_size - 2)*i,
                       ftdi->readbuffer+ftdi->readbuffer_offset+packet_size*i,
                       packet_size - 2);
            if (chunk_remains > 2)
            {
                memmove (ftdi->readbuffer+ftdi->readbuffer_offset+(packet_size - 2)*i,
                         ftdi->readbuffer+ftdi->readbuffer_offset+packet_size*i,
                         chunk_remains-2);
                actual_length -= 2*num_of_chunks;
            }
            else
              actual_length -= 2*(num_of_chunks-1)+chunk_remains;
        }

        if (actual_length > 0)
        {
            // data still fits in buf?
            if (tc->offset + actual_length <= tc->size)
            {
                memcpy (tc->buf + tc->offset, ftdi->readbuffer + ftdi->readbuffer_offset, actual_length);
                //printf("buf[0] = %X, buf[1] = %X\n", buf[0], buf[1]);
                tc->offset += actual_length;

                ftdi->readbuffer_offset = 0;
                ftdi->readbuffer_remaining = 0;

                /* Did we read exactly the right amount of bytes? */
                if (tc->offset == tc->size)
                {
                    //printf("read_data exact rem %d offset %d\n",
                    //ftdi->readbuffer_remaining, offset);
                    tc->completed = 1;
                    return;
                }
            }
            else
            {
                // only copy part of the data or size <= readbuffer_chunksize
                int part_size = tc->size - tc->offset;
                memcpy (tc->buf + tc->offset, ftdi->readbuffer + ftdi->readbuffer_offset, part_size);
                tc->offset += part_size;

                ftdi->readbuffer_offset += part_size;
                ftdi->readbuffer_remaining = actual_length - part_size;

                /* printf("Returning part: %d - size: %d - offset: %d - actual_length: %d - remaining: %d\n",
                part_size, size, offset, actual_length, ftdi->readbuffer_remaining); */
                tc->completed = 1;
                return;
            }
        }
    }
    ret = libusb_submit_transfer (transfer);
    if (ret < 0)
        tc->completed = 1;
}


static void LIBUSB_CALL ftdi_write_data_cb(struct libusb_transfer *transfer)
{
    struct ftdi_transfer_control *tc = (struct ftdi_transfer_control *) transfer->user_data;
    struct ftdi_context *ftdi = tc->ftdi;

    tc->offset = transfer->actual_length;

    if (tc->offset == tc->size)
    {
        tc->completed = 1;
    }
    else
    {
        int write_size = ftdi->writebuffer_chunksize;
        int ret;

        if (tc->offset + write_size > tc->size)
            write_size = tc->size - tc->offset;

        transfer->length = write_size;
        transfer->buffer = tc->buf + tc->offset;
        ret = libusb_submit_transfer (transfer);
        if (ret < 0)
            tc->completed = 1;
    }
}


/**
    Writes data to the chip. Does not wait for completion of the transfer
    nor does it make sure that the transfer was successful.

    Use libusb 1.0 Asynchronous API.
    Only available if compiled with --with-async-mode.

    \param ftdi pointer to ftdi_context
    \param buf Buffer with the data
    \param size Size of the buffer

    \retval NULL: Some error happens when submit transfer
    \retval !NULL: Pointer to a ftdi_transfer_control
*/

struct ftdi_transfer_control *ftdi_write_data_submit(struct ftdi_context *ftdi, unsigned char *buf, int size)
{
    struct ftdi_transfer_control *tc;
    struct libusb_transfer *transfer = libusb_alloc_transfer(0);
    int write_size, ret;

    tc = (struct ftdi_transfer_control *) malloc (sizeof (*tc));

    if (!tc || !transfer)
        return NULL;

    tc->ftdi = ftdi;
    tc->completed = 0;
    tc->buf = buf;
    tc->size = size;
    tc->offset = 0;

    if (size < ftdi->writebuffer_chunksize)
      write_size = size;
    else
      write_size = ftdi->writebuffer_chunksize;

    libusb_fill_bulk_transfer(transfer, ftdi->usb_dev, ftdi->in_ep, buf, write_size, ftdi_write_data_cb, tc, ftdi->usb_write_timeout);
    transfer->type = LIBUSB_TRANSFER_TYPE_BULK;

    ret = libusb_submit_transfer(transfer);
    if (ret < 0)
    {
        libusb_free_transfer(transfer);
        tc->completed = 1;
        tc->transfer = NULL;
        return NULL;
    }
    tc->transfer = transfer;

    return tc;
}

/**
    Reads data from the chip. Does not wait for completion of the transfer
    nor does it make sure that the transfer was successful.

    Use libusb 1.0 Asynchronous API.
    Only available if compiled with --with-async-mode.

    \param ftdi pointer to ftdi_context
    \param buf Buffer with the data
    \param size Size of the buffer

    \retval NULL: Some error happens when submit transfer
    \retval !NULL: Pointer to a ftdi_transfer_control
*/

struct ftdi_transfer_control *ftdi_read_data_submit(struct ftdi_context *ftdi, unsigned char *buf, int size)
{
    struct ftdi_transfer_control *tc;
    struct libusb_transfer *transfer;
    int ret;

    tc = (struct ftdi_transfer_control *) malloc (sizeof (*tc));
    if (!tc)
        return NULL;

    tc->ftdi = ftdi;
    tc->buf = buf;
    tc->size = size;

    if (size <= ftdi->readbuffer_remaining)
    {
        memcpy (buf, ftdi->readbuffer+ftdi->readbuffer_offset, size);

        // Fix offsets
        ftdi->readbuffer_remaining -= size;
        ftdi->readbuffer_offset += size;

        /* printf("Returning bytes from buffer: %d - remaining: %d\n", size, ftdi->readbuffer_remaining); */

        tc->completed = 1;
        tc->offset = size;
        tc->transfer = NULL;
        return tc;
    }

    tc->completed = 0;
    if (ftdi->readbuffer_remaining != 0)
    {
        memcpy (buf, ftdi->readbuffer+ftdi->readbuffer_offset, ftdi->readbuffer_remaining);

        tc->offset = ftdi->readbuffer_remaining;
    }
    else
        tc->offset = 0;

    transfer = libusb_alloc_transfer(0);
    if (!transfer)
    {
        free (tc);
        return NULL;
    }

    ftdi->readbuffer_remaining = 0;
    ftdi->readbuffer_offset = 0;

    libusb_fill_bulk_transfer(transfer, ftdi->usb_dev, ftdi->out_ep, ftdi->readbuffer, ftdi->readbuffer_chunksize, ftdi_read_data_cb, tc, ftdi->usb_read_timeout);
    transfer->type = LIBUSB_TRANSFER_TYPE_BULK;

    ret = libusb_submit_transfer(transfer);
    if (ret < 0)
    {
        libusb_free_transfer(transfer);
        free (tc);
        return NULL;
    }
    tc->transfer = transfer;

    return tc;
}

/**
    Wait for completion of the transfer.

    Use libusb 1.0 Asynchronous API.
    Only available if compiled with --with-async-mode.

    \param tc pointer to ftdi_transfer_control

    \retval < 0: Some error happens
    \retval >= 0: Data size transferred
*/

int ftdi_transfer_data_done(struct ftdi_transfer_control *tc)
{
    int ret;

    while (!tc->completed)
    {
        ret = libusb_handle_events(NULL);
        if (ret < 0)
        {
            if (ret == LIBUSB_ERROR_INTERRUPTED)
                continue;
            libusb_cancel_transfer(tc->transfer);
            while (!tc->completed)
                if (libusb_handle_events(NULL) < 0)
                    break;
            libusb_free_transfer(tc->transfer);
            free (tc);
            tc = NULL;
            return ret;
        }
    }

    if (tc->transfer->status == LIBUSB_TRANSFER_COMPLETED)
        ret = tc->offset;
    else
        ret = -1;

    libusb_free_transfer(tc->transfer);
    free(tc);
    return ret;
}

#endif // LIBFTDI_LINUX_ASYNC_MODE

/**
    Configure write buffer chunk size.
    Default is 4096.

    \param ftdi pointer to ftdi_context
    \param chunksize Chunk size

    \retval 0: all fine
*/
int ftdi_write_data_set_chunksize(struct ftdi_context *ftdi, unsigned int chunksize)
{
    ftdi->writebuffer_chunksize = chunksize;
    return 0;
}

/**
    Get write buffer chunk size.

    \param ftdi pointer to ftdi_context
    \param chunksize Pointer to store chunk size in

    \retval 0: all fine
*/
int ftdi_write_data_get_chunksize(struct ftdi_context *ftdi, unsigned int *chunksize)
{
    *chunksize = ftdi->writebuffer_chunksize;
    return 0;
}

/**
    Reads data in chunks (see ftdi_read_data_set_chunksize()) from the chip.

    Automatically strips the two modem status bytes transfered during every read.

    \param ftdi pointer to ftdi_context
    \param buf Buffer to store data in
    \param size Size of the buffer

    \retval <0: error code from libusb_bulk_transfer()
    \retval  0: no data was available
    \retval >0: number of bytes read

*/
int ftdi_read_data(struct ftdi_context *ftdi, unsigned char *buf, int size)
{
    int offset = 0, ret, i, num_of_chunks, chunk_remains;
    int packet_size = ftdi->max_packet_size;
    int actual_length = 1;

    // Packet size sanity check (avoid division by zero)
    if (packet_size == 0)
        ftdi_error_return(-1, "max_packet_size is bogus (zero)");

    // everything we want is still in the readbuffer?
    if (size <= ftdi->readbuffer_remaining)
    {
        memcpy (buf, ftdi->readbuffer+ftdi->readbuffer_offset, size);

        // Fix offsets
        ftdi->readbuffer_remaining -= size;
        ftdi->readbuffer_offset += size;

        /* printf("Returning bytes from buffer: %d - remaining: %d\n", size, ftdi->readbuffer_remaining); */

        return size;
    }
    // something still in the readbuffer, but not enough to satisfy 'size'?
    if (ftdi->readbuffer_remaining != 0)
    {
        memcpy (buf, ftdi->readbuffer+ftdi->readbuffer_offset, ftdi->readbuffer_remaining);

        // Fix offset
        offset += ftdi->readbuffer_remaining;
    }
    // do the actual USB read
    while (offset < size && actual_length > 0)
    {
        ftdi->readbuffer_remaining = 0;
        ftdi->readbuffer_offset = 0;
        /* returns how much received */
        ret = libusb_bulk_transfer (ftdi->usb_dev, ftdi->out_ep, ftdi->readbuffer, ftdi->readbuffer_chunksize, &actual_length, ftdi->usb_read_timeout);
        if (ret < 0)
            ftdi_error_return(ret, "usb bulk read failed");

        if (actual_length > 2)
        {
            // skip FTDI status bytes.
            // Maybe stored in the future to enable modem use
            num_of_chunks = actual_length / packet_size;
            chunk_remains = actual_length % packet_size;
            //printf("actual_length = %X, num_of_chunks = %X, chunk_remains = %X, readbuffer_offset = %X\n", actual_length, num_of_chunks, chunk_remains, ftdi->readbuffer_offset);

            ftdi->readbuffer_offset += 2;
            actual_length -= 2;

            if (actual_length > packet_size - 2)
            {
                for (i = 1; i < num_of_chunks; i++)
                    memmove (ftdi->readbuffer+ftdi->readbuffer_offset+(packet_size - 2)*i,
                             ftdi->readbuffer+ftdi->readbuffer_offset+packet_size*i,
                             packet_size - 2);
                if (chunk_remains > 2)
                {
                    memmove (ftdi->readbuffer+ftdi->readbuffer_offset+(packet_size - 2)*i,
                             ftdi->readbuffer+ftdi->readbuffer_offset+packet_size*i,
                             chunk_remains-2);
                    actual_length -= 2*num_of_chunks;
                }
                else
                    actual_length -= 2*(num_of_chunks-1)+chunk_remains;
            }
        }
        else if (actual_length <= 2)
        {
            // no more data to read?
            return offset;
        }
        if (actual_length > 0)
        {
            // data still fits in buf?
            if (offset+actual_length <= size)
            {
                memcpy (buf+offset, ftdi->readbuffer+ftdi->readbuffer_offset, actual_length);
                //printf("buf[0] = %X, buf[1] = %X\n", buf[0], buf[1]);
                offset += actual_length;

                /* Did we read exactly the right amount of bytes? */
                if (offset == size)
                    //printf("read_data exact rem %d offset %d\n",
                    //ftdi->readbuffer_remaining, offset);
                    return offset;
            }
            else
            {
                // only copy part of the data or size <= readbuffer_chunksize
                int part_size = size-offset;
                memcpy (buf+offset, ftdi->readbuffer+ftdi->readbuffer_offset, part_size);

                ftdi->readbuffer_offset += part_size;
                ftdi->readbuffer_remaining = actual_length-part_size;
                offset += part_size;

                /* printf("Returning part: %d - size: %d - offset: %d - actual_length: %d - remaining: %d\n",
                part_size, size, offset, actual_length, ftdi->readbuffer_remaining); */

                return offset;
            }
        }
    }
    // never reached
    return -127;
}

/**
    Configure read buffer chunk size.
    Default is 4096.

    Automatically reallocates the buffer.

    \param ftdi pointer to ftdi_context
    \param chunksize Chunk size

    \retval 0: all fine
*/
int ftdi_read_data_set_chunksize(struct ftdi_context *ftdi, unsigned int chunksize)
{
    unsigned char *new_buf;

    // Invalidate all remaining data
    ftdi->readbuffer_offset = 0;
    ftdi->readbuffer_remaining = 0;
#ifdef __linux__
    /* We can't set readbuffer_chunksize larger than MAX_BULK_BUFFER_LENGTH,
       which is defined in libusb-1.0.  Otherwise, each USB read request will
       be divided into multiple URBs.  This will cause issues on Linux kernel
       older than 2.6.32.  */
    if (chunksize > 16384)
        chunksize = 16384;
#endif

    if ((new_buf = (unsigned char *)realloc(ftdi->readbuffer, chunksize)) == NULL)
        ftdi_error_return(-1, "out of memory for readbuffer");

    ftdi->readbuffer = new_buf;
    ftdi->readbuffer_chunksize = chunksize;

    return 0;
}

/**
    Get read buffer chunk size.

    \param ftdi pointer to ftdi_context
    \param chunksize Pointer to store chunk size in

    \retval 0: all fine
*/
int ftdi_read_data_get_chunksize(struct ftdi_context *ftdi, unsigned int *chunksize)
{
    *chunksize = ftdi->readbuffer_chunksize;
    return 0;
}


/**
    Enable bitbang mode.

    \deprecated use \ref ftdi_set_bitmode with mode BITMODE_BITBANG instead

    \param ftdi pointer to ftdi_context
    \param bitmask Bitmask to configure lines.
           HIGH/ON value configures a line as output.

    \retval  0: all fine
    \retval -1: can't enable bitbang mode
*/
int ftdi_enable_bitbang(struct ftdi_context *ftdi, unsigned char bitmask)
{
    unsigned short usb_val;

    usb_val = bitmask; // low byte: bitmask
    /* FT2232C: Set bitbang_mode to 2 to enable SPI */
    usb_val |= (ftdi->bitbang_mode << 8);

    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_BITMODE_REQUEST, usb_val, ftdi->index,
                                NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-1, "unable to enter bitbang mode. Perhaps not a BM type chip?");

    ftdi->bitbang_enabled = 1;
    return 0;
}

/**
    Disable bitbang mode.

    \param ftdi pointer to ftdi_context

    \retval  0: all fine
    \retval -1: can't disable bitbang mode
*/
int ftdi_disable_bitbang(struct ftdi_context *ftdi)
{
    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE, SIO_SET_BITMODE_REQUEST, 0, ftdi->index, NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-1, "unable to leave bitbang mode. Perhaps not a BM type chip?");

    ftdi->bitbang_enabled = 0;
    return 0;
}

/**
    Enable/disable bitbang modes.

    \param ftdi pointer to ftdi_context
    \param bitmask Bitmask to configure lines.
           HIGH/ON value configures a line as output.
    \param mode Bitbang mode: use the values defined in \ref ftdi_mpsse_mode

    \retval  0: all fine
    \retval -1: can't enable bitbang mode
*/
int ftdi_set_bitmode(struct ftdi_context *ftdi, unsigned char bitmask, unsigned char mode)
{
    unsigned short usb_val;

    usb_val = bitmask; // low byte: bitmask
    usb_val |= (mode << 8);
    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE, SIO_SET_BITMODE_REQUEST, usb_val, ftdi->index, NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-1, "unable to configure bitbang mode. Perhaps not a 2232C type chip?");

    ftdi->bitbang_mode = mode;
    ftdi->bitbang_enabled = (mode == BITMODE_RESET) ? 0 : 1;
    return 0;
}

/**
    Directly read pin state, circumventing the read buffer. Useful for bitbang mode.

    \param ftdi pointer to ftdi_context
    \param pins Pointer to store pins into

    \retval  0: all fine
    \retval -1: read pins failed
*/
int ftdi_read_pins(struct ftdi_context *ftdi, unsigned char *pins)
{
    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_IN_REQTYPE, SIO_READ_PINS_REQUEST, 0, ftdi->index, (unsigned char *)pins, 1, ftdi->usb_read_timeout) != 1)
        ftdi_error_return(-1, "read pins failed");

    return 0;
}

/**
    Set latency timer

    The FTDI chip keeps data in the internal buffer for a specific
    amount of time if the buffer is not full yet to decrease
    load on the usb bus.

    \param ftdi pointer to ftdi_context
    \param latency Value between 1 and 255

    \retval  0: all fine
    \retval -1: latency out of range
    \retval -2: unable to set latency timer
*/
int ftdi_set_latency_timer(struct ftdi_context *ftdi, unsigned char latency)
{
    unsigned short usb_val;

    if (latency < 1)
        ftdi_error_return(-1, "latency out of range. Only valid for 1-255");

    usb_val = latency;
    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE, SIO_SET_LATENCY_TIMER_REQUEST, usb_val, ftdi->index, NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-2, "unable to set latency timer");

    return 0;
}

/**
    Get latency timer

    \param ftdi pointer to ftdi_context
    \param latency Pointer to store latency value in

    \retval  0: all fine
    \retval -1: unable to get latency timer
*/
int ftdi_get_latency_timer(struct ftdi_context *ftdi, unsigned char *latency)
{
    unsigned short usb_val;
    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_IN_REQTYPE, SIO_GET_LATENCY_TIMER_REQUEST, 0, ftdi->index, (unsigned char *)&usb_val, 1, ftdi->usb_read_timeout) != 1)
        ftdi_error_return(-1, "reading latency timer failed");

    *latency = (unsigned char)usb_val;
    return 0;
}

/**
    Poll modem status information

    This function allows the retrieve the two status bytes of the device.
    The device sends these bytes also as a header for each read access
    where they are discarded by ftdi_read_data(). The chip generates
    the two stripped status bytes in the absence of data every 40 ms.

    Layout of the first byte:
    - B0..B3 - must be 0
    - B4       Clear to send (CTS)
                 0 = inactive
                 1 = active
    - B5       Data set ready (DTS)
                 0 = inactive
                 1 = active
    - B6       Ring indicator (RI)
                 0 = inactive
                 1 = active
    - B7       Receive line signal detect (RLSD)
                 0 = inactive
                 1 = active

    Layout of the second byte:
    - B0       Data ready (DR)
    - B1       Overrun error (OE)
    - B2       Parity error (PE)
    - B3       Framing error (FE)
    - B4       Break interrupt (BI)
    - B5       Transmitter holding register (THRE)
    - B6       Transmitter empty (TEMT)
    - B7       Error in RCVR FIFO

    \param ftdi pointer to ftdi_context
    \param status Pointer to store status information in. Must be two bytes.

    \retval  0: all fine
    \retval -1: unable to retrieve status information
*/
int ftdi_poll_modem_status(struct ftdi_context *ftdi, unsigned short *status)
{
    char usb_val[2];

    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_IN_REQTYPE, SIO_POLL_MODEM_STATUS_REQUEST, 0, ftdi->index, (unsigned char *)usb_val, 2, ftdi->usb_read_timeout) != 2)
        ftdi_error_return(-1, "getting modem status failed");

    *status = (usb_val[1] << 8) | usb_val[0];

    return 0;
}

/**
    Set flowcontrol for ftdi chip

    \param ftdi pointer to ftdi_context
    \param flowctrl flow control to use. should be
           SIO_DISABLE_FLOW_CTRL, SIO_RTS_CTS_HS, SIO_DTR_DSR_HS or SIO_XON_XOFF_HS

    \retval  0: all fine
    \retval -1: set flow control failed
*/
int ftdi_setflowctrl(struct ftdi_context *ftdi, int flowctrl)
{
    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_FLOW_CTRL_REQUEST, 0, (flowctrl | ftdi->index),
                                NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-1, "set flow control failed");

    return 0;
}

/**
    Set dtr line

    \param ftdi pointer to ftdi_context
    \param state state to set line to (1 or 0)

    \retval  0: all fine
    \retval -1: set dtr failed
*/
int ftdi_setdtr(struct ftdi_context *ftdi, int state)
{
    unsigned short usb_val;

    if (state)
        usb_val = SIO_SET_DTR_HIGH;
    else
        usb_val = SIO_SET_DTR_LOW;

    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_MODEM_CTRL_REQUEST, usb_val, ftdi->index,
                                NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-1, "set dtr failed");

    return 0;
}

/**
    Set rts line

    \param ftdi pointer to ftdi_context
    \param state state to set line to (1 or 0)

    \retval  0: all fine
    \retval -1 set rts failed
*/
int ftdi_setrts(struct ftdi_context *ftdi, int state)
{
    unsigned short usb_val;

    if (state)
        usb_val = SIO_SET_RTS_HIGH;
    else
        usb_val = SIO_SET_RTS_LOW;

    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_MODEM_CTRL_REQUEST, usb_val, ftdi->index,
                                NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-1, "set of rts failed");

    return 0;
}

/**
 Set dtr and rts line in one pass

 \param ftdi pointer to ftdi_context
 \param dtr  DTR state to set line to (1 or 0)
 \param rts  RTS state to set line to (1 or 0)

 \retval  0: all fine
 \retval -1 set dtr/rts failed
 */
int ftdi_setdtr_rts(struct ftdi_context *ftdi, int dtr, int rts)
{
    unsigned short usb_val;

    if (dtr)
        usb_val = SIO_SET_DTR_HIGH;
    else
        usb_val = SIO_SET_DTR_LOW;

    if (rts)
        usb_val |= SIO_SET_RTS_HIGH;
    else
        usb_val |= SIO_SET_RTS_LOW;

    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                SIO_SET_MODEM_CTRL_REQUEST, usb_val, ftdi->index,
                                NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-1, "set of rts/dtr failed");

    return 0;
}

/**
    Set the special event character

    \param ftdi pointer to ftdi_context
    \param eventch Event character
    \param enable 0 to disable the event character, non-zero otherwise

    \retval  0: all fine
    \retval -1: unable to set event character
*/
int ftdi_set_event_char(struct ftdi_context *ftdi,
                        unsigned char eventch, unsigned char enable)
{
    unsigned short usb_val;

    usb_val = eventch;
    if (enable)
        usb_val |= 1 << 8;

    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE, SIO_SET_EVENT_CHAR_REQUEST, usb_val, ftdi->index, NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-1, "setting event character failed");

    return 0;
}

/**
    Set error character

    \param ftdi pointer to ftdi_context
    \param errorch Error character
    \param enable 0 to disable the error character, non-zero otherwise

    \retval  0: all fine
    \retval -1: unable to set error character
*/
int ftdi_set_error_char(struct ftdi_context *ftdi,
                        unsigned char errorch, unsigned char enable)
{
    unsigned short usb_val;

    usb_val = errorch;
    if (enable)
        usb_val |= 1 << 8;

    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE, SIO_SET_ERROR_CHAR_REQUEST, usb_val, ftdi->index, NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-1, "setting error character failed");

    return 0;
}

/**
   Set the eeprom size

   \param ftdi pointer to ftdi_context
   \param eeprom Pointer to ftdi_eeprom
   \param size

*/
void ftdi_eeprom_setsize(struct ftdi_context *ftdi, struct ftdi_eeprom *eeprom, int size)
{
    ftdi->eeprom_size=size;
    eeprom->size=size;
}

/**
    Init eeprom with default values.

    \param eeprom Pointer to ftdi_eeprom
*/
void ftdi_eeprom_initdefaults(struct ftdi_eeprom *eeprom)
{
    eeprom->vendor_id = 0x0403;
    eeprom->product_id = 0x6001;

    eeprom->self_powered = 1;
    eeprom->remote_wakeup = 1;
    eeprom->BM_type_chip = 1;

    eeprom->in_is_isochronous = 0;
    eeprom->out_is_isochronous = 0;
    eeprom->suspend_pull_downs = 0;

    eeprom->use_serial = 0;
    eeprom->change_usb_version = 0;
    eeprom->usb_version = 0x0200;
    eeprom->max_power = 0;

    eeprom->manufacturer = NULL;
    eeprom->product = NULL;
    eeprom->serial = NULL;

    eeprom->size = FTDI_DEFAULT_EEPROM_SIZE;
}

/**
   Build binary output from ftdi_eeprom structure.
   Output is suitable for ftdi_write_eeprom().

   \param eeprom Pointer to ftdi_eeprom
   \param output Buffer of 128 bytes to store eeprom image to

   \retval >0: used eeprom size
   \retval -1: eeprom size (128 bytes) exceeded by custom strings
*/
int ftdi_eeprom_build(struct ftdi_eeprom *eeprom, unsigned char *output)
{
    unsigned char i, j;
    unsigned short checksum, value;
    unsigned char manufacturer_size = 0, product_size = 0, serial_size = 0;
    int size_check;

    if (eeprom->manufacturer != NULL)
        manufacturer_size = strlen(eeprom->manufacturer);
    if (eeprom->product != NULL)
        product_size = strlen(eeprom->product);
    if (eeprom->serial != NULL)
        serial_size = strlen(eeprom->serial);

    size_check = eeprom->size;
    size_check -= 28; // 28 are always in use (fixed)

    // Top half of a 256byte eeprom is used just for strings and checksum
    // it seems that the FTDI chip will not read these strings from the lower half
    // Each string starts with two bytes; offset and type (0x03 for string)
    // the checksum needs two bytes, so without the string data that 8 bytes from the top half
    if (eeprom->size>=256)size_check = 120;
    size_check -= manufacturer_size*2;
    size_check -= product_size*2;
    size_check -= serial_size*2;

    // eeprom size exceeded?
    if (size_check < 0)
        return (-1);

    // empty eeprom
    memset (output, 0, eeprom->size);

    // Addr 00: Stay 00 00
    // Addr 02: Vendor ID
    output[0x02] = eeprom->vendor_id;
    output[0x03] = eeprom->vendor_id >> 8;

    // Addr 04: Product ID
    output[0x04] = eeprom->product_id;
    output[0x05] = eeprom->product_id >> 8;

    // Addr 06: Device release number (0400h for BM features)
    output[0x06] = 0x00;

    if (eeprom->BM_type_chip == 1)
        output[0x07] = 0x04;
    else
        output[0x07] = 0x02;

    // Addr 08: Config descriptor
    // Bit 7: always 1
    // Bit 6: 1 if this device is self powered, 0 if bus powered
    // Bit 5: 1 if this device uses remote wakeup
    // Bit 4: 1 if this device is battery powered
    j = 0x80;
    if (eeprom->self_powered == 1)
        j |= 0x40;
    if (eeprom->remote_wakeup == 1)
        j |= 0x20;
    output[0x08] = j;

    // Addr 09: Max power consumption: max power = value * 2 mA
    output[0x09] = eeprom->max_power;

    // Addr 0A: Chip configuration
    // Bit 7: 0 - reserved
    // Bit 6: 0 - reserved
    // Bit 5: 0 - reserved
    // Bit 4: 1 - Change USB version
    // Bit 3: 1 - Use the serial number string
    // Bit 2: 1 - Enable suspend pull downs for lower power
    // Bit 1: 1 - Out EndPoint is Isochronous
    // Bit 0: 1 - In EndPoint is Isochronous
    //
    j = 0;
    if (eeprom->in_is_isochronous == 1)
        j = j | 1;
    if (eeprom->out_is_isochronous == 1)
        j = j | 2;
    if (eeprom->suspend_pull_downs == 1)
        j = j | 4;
    if (eeprom->use_serial == 1)
        j = j | 8;
    if (eeprom->change_usb_version == 1)
        j = j | 16;
    output[0x0A] = j;

    // Addr 0B: reserved
    output[0x0B] = 0x00;

    // Addr 0C: USB version low byte when 0x0A bit 4 is set
    // Addr 0D: USB version high byte when 0x0A bit 4 is set
    if (eeprom->change_usb_version == 1)
    {
        output[0x0C] = eeprom->usb_version;
        output[0x0D] = eeprom->usb_version >> 8;
    }


    // Addr 0E: Offset of the manufacturer string + 0x80, calculated later
    // Addr 0F: Length of manufacturer string
    output[0x0F] = manufacturer_size*2 + 2;

    // Addr 10: Offset of the product string + 0x80, calculated later
    // Addr 11: Length of product string
    output[0x11] = product_size*2 + 2;

    // Addr 12: Offset of the serial string + 0x80, calculated later
    // Addr 13: Length of serial string
    output[0x13] = serial_size*2 + 2;

    // Dynamic content
    i=0x14;
    if (eeprom->size>=256) i = 0x80;


    // Output manufacturer
    output[0x0E] = i | 0x80;  // calculate offset
    output[i++] = manufacturer_size*2 + 2;
    output[i++] = 0x03; // type: string
    for (j = 0; j < manufacturer_size; j++)
    {
        output[i] = eeprom->manufacturer[j], i++;
        output[i] = 0x00, i++;
    }

    // Output product name
    output[0x10] = i | 0x80;  // calculate offset
    output[i] = product_size*2 + 2, i++;
    output[i] = 0x03, i++;
    for (j = 0; j < product_size; j++)
    {
        output[i] = eeprom->product[j], i++;
        output[i] = 0x00, i++;
    }

    // Output serial
    output[0x12] = i | 0x80; // calculate offset
    output[i] = serial_size*2 + 2, i++;
    output[i] = 0x03, i++;
    for (j = 0; j < serial_size; j++)
    {
        output[i] = eeprom->serial[j], i++;
        output[i] = 0x00, i++;
    }

    // calculate checksum
    checksum = 0xAAAA;

    for (i = 0; i < eeprom->size/2-1; i++)
    {
        value = output[i*2];
        value += output[(i*2)+1] << 8;

        checksum = value^checksum;
        checksum = (checksum << 1) | (checksum >> 15);
    }

    output[eeprom->size-2] = checksum;
    output[eeprom->size-1] = checksum >> 8;

    return size_check;
}

/**
   Decode binary EEPROM image into an ftdi_eeprom structure.

   \param eeprom Pointer to ftdi_eeprom which will be filled in.
   \param buf Buffer of \a size bytes of raw eeprom data
   \param size size size of eeprom data in bytes

   \retval 0: all fine
   \retval -1: something went wrong

   FIXME: How to pass size? How to handle size field in ftdi_eeprom?
   FIXME: Strings are malloc'ed here and should be freed somewhere
*/
int ftdi_eeprom_decode(struct ftdi_eeprom *eeprom, unsigned char *buf, int size)
{
    unsigned char i, j;
    unsigned short checksum, eeprom_checksum, value;
    unsigned char manufacturer_size = 0, product_size = 0, serial_size = 0;
    int eeprom_size = 128;
#if 0
    size_check = eeprom->size;
    size_check -= 28; // 28 are always in use (fixed)

    // Top half of a 256byte eeprom is used just for strings and checksum
    // it seems that the FTDI chip will not read these strings from the lower half
    // Each string starts with two bytes; offset and type (0x03 for string)
    // the checksum needs two bytes, so without the string data that 8 bytes from the top half
    if (eeprom->size>=256)size_check = 120;
    size_check -= manufacturer_size*2;
    size_check -= product_size*2;
    size_check -= serial_size*2;

    // eeprom size exceeded?
    if (size_check < 0)
        return (-1);
#endif

    // empty eeprom struct
    memset(eeprom, 0, sizeof(struct ftdi_eeprom));

    // Addr 00: Stay 00 00

    // Addr 02: Vendor ID
    eeprom->vendor_id = buf[0x02] + (buf[0x03] << 8);

    // Addr 04: Product ID
    eeprom->product_id = buf[0x04] + (buf[0x05] << 8);

    value = buf[0x06] + (buf[0x07]<<8);
    switch (value)
    {
        case 0x0400:
            eeprom->BM_type_chip = 1;
            break;
        case 0x0200:
            eeprom->BM_type_chip = 0;
            break;
        default: // Unknown device
            eeprom->BM_type_chip = 0;
            break;
    }

    // Addr 08: Config descriptor
    // Bit 7: always 1
    // Bit 6: 1 if this device is self powered, 0 if bus powered
    // Bit 5: 1 if this device uses remote wakeup
    // Bit 4: 1 if this device is battery powered
    j = buf[0x08];
    if (j&0x40) eeprom->self_powered = 1;
    if (j&0x20) eeprom->remote_wakeup = 1;

    // Addr 09: Max power consumption: max power = value * 2 mA
    eeprom->max_power = buf[0x09];

    // Addr 0A: Chip configuration
    // Bit 7: 0 - reserved
    // Bit 6: 0 - reserved
    // Bit 5: 0 - reserved
    // Bit 4: 1 - Change USB version
    // Bit 3: 1 - Use the serial number string
    // Bit 2: 1 - Enable suspend pull downs for lower power
    // Bit 1: 1 - Out EndPoint is Isochronous
    // Bit 0: 1 - In EndPoint is Isochronous
    //
    j = buf[0x0A];
    if (j&0x01) eeprom->in_is_isochronous = 1;
    if (j&0x02) eeprom->out_is_isochronous = 1;
    if (j&0x04) eeprom->suspend_pull_downs = 1;
    if (j&0x08) eeprom->use_serial = 1;
    if (j&0x10) eeprom->change_usb_version = 1;

    // Addr 0B: reserved

    // Addr 0C: USB version low byte when 0x0A bit 4 is set
    // Addr 0D: USB version high byte when 0x0A bit 4 is set
    if (eeprom->change_usb_version == 1)
    {
        eeprom->usb_version = buf[0x0C] + (buf[0x0D] << 8);
    }

    // Addr 0E: Offset of the manufacturer string + 0x80, calculated later
    // Addr 0F: Length of manufacturer string
    manufacturer_size = buf[0x0F]/2;
    if (manufacturer_size > 0) eeprom->manufacturer = malloc(manufacturer_size);
    else eeprom->manufacturer = NULL;

    // Addr 10: Offset of the product string + 0x80, calculated later
    // Addr 11: Length of product string
    product_size = buf[0x11]/2;
    if (product_size > 0) eeprom->product = malloc(product_size);
    else eeprom->product = NULL;

    // Addr 12: Offset of the serial string + 0x80, calculated later
    // Addr 13: Length of serial string
    serial_size = buf[0x13]/2;
    if (serial_size > 0) eeprom->serial = malloc(serial_size);
    else eeprom->serial = NULL;

    // Decode manufacturer
    i = buf[0x0E] & 0x7f; // offset
    for (j=0;j<manufacturer_size-1;j++)
    {
        eeprom->manufacturer[j] = buf[2*j+i+2];
    }
    eeprom->manufacturer[j] = '\0';

    // Decode product name
    i = buf[0x10] & 0x7f; // offset
    for (j=0;j<product_size-1;j++)
    {
        eeprom->product[j] = buf[2*j+i+2];
    }
    eeprom->product[j] = '\0';

    // Decode serial
    i = buf[0x12] & 0x7f; // offset
    for (j=0;j<serial_size-1;j++)
    {
        eeprom->serial[j] = buf[2*j+i+2];
    }
    eeprom->serial[j] = '\0';

    // verify checksum
    checksum = 0xAAAA;

    for (i = 0; i < eeprom_size/2-1; i++)
    {
        value = buf[i*2];
        value += buf[(i*2)+1] << 8;

        checksum = value^checksum;
        checksum = (checksum << 1) | (checksum >> 15);
    }

    eeprom_checksum = buf[eeprom_size-2] + (buf[eeprom_size-1] << 8);

    if (eeprom_checksum != checksum)
    {
        fprintf(stderr, "Checksum Error: %04x %04x\n", checksum, eeprom_checksum);
        return -1;
    }

    return 0;
}

/**
    Read eeprom location

    \param ftdi pointer to ftdi_context
    \param eeprom_addr Address of eeprom location to be read
    \param eeprom_val Pointer to store read eeprom location

    \retval  0: all fine
    \retval -1: read failed
*/
int ftdi_read_eeprom_location (struct ftdi_context *ftdi, int eeprom_addr, unsigned short *eeprom_val)
{
    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_IN_REQTYPE, SIO_READ_EEPROM_REQUEST, 0, eeprom_addr, (char *)eeprom_val, 2, ftdi->usb_read_timeout) != 2)
        ftdi_error_return(-1, "reading eeprom failed");

    return 0;
}

/**
    Read eeprom

    \param ftdi pointer to ftdi_context
    \param eeprom Pointer to store eeprom into

    \retval  0: all fine
    \retval -1: read failed
*/
int ftdi_read_eeprom(struct ftdi_context *ftdi, unsigned char *eeprom)
{
    int i;

    for (i = 0; i < ftdi->eeprom_size/2; i++)
    {
        if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_IN_REQTYPE, SIO_READ_EEPROM_REQUEST, 0, i, eeprom+(i*2), 2, ftdi->usb_read_timeout) != 2)
            ftdi_error_return(-1, "reading eeprom failed");
    }

    return 0;
}

/*
    ftdi_read_chipid_shift does the bitshift operation needed for the FTDIChip-ID
    Function is only used internally
    \internal
*/
static unsigned char ftdi_read_chipid_shift(unsigned char value)
{
    return ((value & 1) << 1) |
           ((value & 2) << 5) |
           ((value & 4) >> 2) |
           ((value & 8) << 4) |
           ((value & 16) >> 1) |
           ((value & 32) >> 1) |
           ((value & 64) >> 4) |
           ((value & 128) >> 2);
}

/**
    Read the FTDIChip-ID from R-type devices

    \param ftdi pointer to ftdi_context
    \param chipid Pointer to store FTDIChip-ID

    \retval  0: all fine
    \retval -1: read failed
*/
int ftdi_read_chipid(struct ftdi_context *ftdi, unsigned int *chipid)
{
    unsigned int a = 0, b = 0;

    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_IN_REQTYPE, SIO_READ_EEPROM_REQUEST, 0, 0x43, (unsigned char *)&a, 2, ftdi->usb_read_timeout) == 2)
    {
        a = a << 8 | a >> 8;
        if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_IN_REQTYPE, SIO_READ_EEPROM_REQUEST, 0, 0x44, (unsigned char *)&b, 2, ftdi->usb_read_timeout) == 2)
        {
            b = b << 8 | b >> 8;
            a = (a << 16) | (b & 0xFFFF);
            a = ftdi_read_chipid_shift(a) | ftdi_read_chipid_shift(a>>8)<<8
                | ftdi_read_chipid_shift(a>>16)<<16 | ftdi_read_chipid_shift(a>>24)<<24;
            *chipid = a ^ 0xa5f0f7d1;
            return 0;
        }
    }

    ftdi_error_return(-1, "read of FTDIChip-ID failed");
}

/**
   Guesses size of eeprom by reading eeprom and comparing halves - will not work with blank eeprom
   Call this function then do a write then call again to see if size changes, if so write again.

   \param ftdi pointer to ftdi_context
   \param eeprom Pointer to store eeprom into
   \param maxsize the size of the buffer to read into

   \retval size of eeprom
*/
int ftdi_read_eeprom_getsize(struct ftdi_context *ftdi, unsigned char *eeprom, int maxsize)
{
    int i=0,j,minsize=32;
    int size=minsize;

    do
    {
        for (j = 0; i < maxsize/2 && j<size; j++)
        {
            if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_IN_REQTYPE,
                                        SIO_READ_EEPROM_REQUEST, 0, i,
                                        eeprom+(i*2), 2, ftdi->usb_read_timeout) != 2)
                ftdi_error_return(-1, "reading eeprom failed");
            i++;
        }
        size*=2;
    }
    while (size<=maxsize && memcmp(eeprom,&eeprom[size/2],size/2)!=0);

    return size/2;
}

/**
    Write eeprom location

    \param ftdi pointer to ftdi_context
    \param eeprom_addr Address of eeprom location to be written
    \param eeprom_val Value to be written

    \retval  0: all fine
    \retval -1: read failed
*/
int ftdi_write_eeprom_location(struct ftdi_context *ftdi, int eeprom_addr, unsigned short eeprom_val)
{
    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                    SIO_WRITE_EEPROM_REQUEST, eeprom_val, eeprom_addr,
                                    NULL, 0, ftdi->usb_write_timeout) != 0)
        ftdi_error_return(-1, "unable to write eeprom");

    return 0;
}

/**
    Write eeprom

    \param ftdi pointer to ftdi_context
    \param eeprom Pointer to read eeprom from

    \retval  0: all fine
    \retval -1: read failed
*/
int ftdi_write_eeprom(struct ftdi_context *ftdi, unsigned char *eeprom)
{
    unsigned short usb_val, status;
    int i, ret;

    /* These commands were traced while running MProg */
    if ((ret = ftdi_usb_reset(ftdi)) != 0)
        return ret;
    if ((ret = ftdi_poll_modem_status(ftdi, &status)) != 0)
        return ret;
    if ((ret = ftdi_set_latency_timer(ftdi, 0x77)) != 0)
        return ret;

    for (i = 0; i < ftdi->eeprom_size/2; i++)
    {
        usb_val = eeprom[i*2];
        usb_val += eeprom[(i*2)+1] << 8;
        if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                    SIO_WRITE_EEPROM_REQUEST, usb_val, i,
                                    NULL, 0, ftdi->usb_write_timeout) < 0)
            ftdi_error_return(-1, "unable to write eeprom");
    }

    return 0;
}

/**
    Erase eeprom

    This is not supported on FT232R/FT245R according to the MProg manual from FTDI.

    \param ftdi pointer to ftdi_context

    \retval  0: all fine
    \retval -1: erase failed
*/
int ftdi_erase_eeprom(struct ftdi_context *ftdi)
{
    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE, SIO_ERASE_EEPROM_REQUEST, 0, 0, NULL, 0, ftdi->usb_write_timeout) < 0)
        ftdi_error_return(-1, "unable to erase eeprom");

    return 0;
}

/**
    Get string representation for last error code

    \param ftdi pointer to ftdi_context

    \retval Pointer to error string
*/
char *ftdi_get_error_string (struct ftdi_context *ftdi)
{
    return ftdi->error_str;
}

/* @} end of doxygen libftdi group */
