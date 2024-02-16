#include "usbkeyboard.h"

#include <stdio.h>
#include <stdlib.h> 

/* References on libusb 1.0 and the USB HID/keyboard protocol
 *
 * http://libusb.org
 * http://www.dreamincode.net/forums/topic/148707-introduction-to-using-libusb-10/
 * http://www.usb.org/developers/devclass_docs/HID1_11.pdf
 * http://www.usb.org/developers/devclass_docs/Hut1_11.pdf
 */

/*
 * Find and return a USB keyboard device or NULL if not found
 * The argument con
 * 
 */
struct libusb_device_handle *openkeyboard(uint8_t *endpoint_address) {


  // Structure representing a USB device detected on the system (pointer to the list of devices)
  libusb_device **devs;

  // Structure representing a handle on a USB device (pointer to the device handle)
  struct libusb_device_handle *keyboard = NULL;

  // Structure representing the device descriptor (stores the descriptor data)
  struct libusb_device_descriptor desc;
  ssize_t num_devs, d;
  uint8_t i, k;
  
  /* Start the library */
  if ( libusb_init(NULL) < 0 ) {
    fprintf(stderr, "Error: libusb_init failed\n");
    exit(1);
  }

  /* Enumerate all the attached USB devices */
  // Returns a list of USB devices found, return value indicates the number of devices (+ 1 element larger)
  if ( (num_devs = libusb_get_device_list(NULL, &devs)) < 0 ) {
    fprintf(stderr, "Error: libusb_get_device_list failed\n");
    exit(1);
  }

  /* Look at each device, remembering the first HID device that speaks
     the keyboard protocol */

  for (d = 0 ; d < num_devs ; d++) {
    libusb_device *dev = devs[d];
    // For each device get the device descriptor and store it in "desc"
    if ( libusb_get_device_descriptor(dev, &desc) < 0 ) {
      fprintf(stderr, "Error: libusb_get_device_descriptor failed\n");
      exit(1);
    }

    if (desc.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE) {			// Based on the device's class
      struct libusb_config_descriptor *config;					// Structure representing the configuration descriptor (store device's config descriptor)
      libusb_get_config_descriptor(dev, 0, &config);				
      for (i = 0 ; i < config->bNumInterfaces ; i++)	       			// For each interface supported by this configuration
	for ( k = 0 ; k < config->interface[i].num_altsetting ; k++ ) {		// For each one of the alternate settings supported by this interface
	  const struct libusb_interface_descriptor *inter =			
	    config->interface[i].altsetting + k ;
	  if ( inter->bInterfaceClass == LIBUSB_CLASS_HID &&			// If this setting of the interface belongs to the HID class
	       inter->bInterfaceProtocol == USB_HID_KEYBOARD_PROTOCOL) {	// and speaks the keyboard protocol
	    int r;
	    if ((r = libusb_open(dev, &keyboard)) != 0) {			// Open the device and obtain a handle (add a reference and make it available)
	      fprintf(stderr, "Error: libusb_open failed: %d\n", r);
	      exit(1);
	    }
	    if (libusb_kernel_driver_active(keyboard,i))			// Check if a kernel driver has "claimed" the interface
	      libusb_detach_kernel_driver(keyboard, i);				// if it is, detach the kernel driver to be able to claim the interface
	    libusb_set_auto_detach_kernel_driver(keyboard, i);			// Always detach kernel drive when the app wants access and attach it after
	    if ((r = libusb_claim_interface(keyboard, i)) != 0) {		// Claim the interface
	      fprintf(stderr, "Error: libusb_claim_interface failed: %d\n", r);
	      exit(1);
	    }
	    // Function takes as argument a pointer to a byte in memory -> the control endpoint of the keyboard is stored there
	    // Endpoint descriptor specifies the settings of the endpoint (transfer type, direction etc)
	    // The interface has a number of different endpoints - endpoint[0] is a control endpoint
	    *endpoint_address = inter->endpoint[0].bEndpointAddress;		
	    goto found;	// Once a valid device is found the loop breaks
	  }
	}
    }
  }

 found:
  libusb_free_device_list(devs, 1);

  return keyboard;
}
