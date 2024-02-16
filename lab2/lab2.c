/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Vasileios Panousopoulos (vp2518)
 */
#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* arthur.cs.columbia.edu */
#define SERVER_HOST "128.59.19.114"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;		// Thread to run the network operations in parallel
void *network_thread_f(void *);

int main()
{
  int err, col;

  struct sockaddr_in serv_addr;

  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[12];

  // Open the FrameBuffer
  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }



  /* Draw rows of asterisks across the top and bottom of the screen */
  /*for (col = 0 ; col < 64 ; col++) {
    fbputchar('*', 0, col);
    fbputchar('*', 23, col);
  }

  fbputs("Hello CSEE 4840 World!", 4, 10);*/


  for (row = 0; row < 24; row++) {
  	for (col = 0 ; col < 64 ; col++) {
		if (row == 21) fbputchar('-', row, col);
		else fbputchar(' ', row, col);
  }

  

  /* Open the keyboard */
  // Find the device and return a handle on it and store its control endpoint in memory 
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }
    
  /* Create a TCP communications socket */  
  // Creates an endpoint for communication - returns a file descriptor
  // Communication Domain (Protocol used for communication): IPv4 Internet Protocol
  // Communication Semantics (socket type): sequenced, reliable, two-way, connection based byte stream
	// Must be in connected state to transfer data (connection to another socket with "connect")
	// Data transferred with "read" and "write" syscalls
	// Data are not lost or duplicated
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));	// Initialize memory addresses with 0	

  // Bundle in a single structure the remote host's protocol, IP address and port
  // This structure describes an IPv4 Internet domain socket
  // Port and Address are stored in network byte order
  serv_addr.sin_family = AF_INET;		
  serv_addr.sin_port = htons(SERVER_PORT);	// Convert port value from host to network byte order
  // Convert IP address (SERVER_HOST) from character string into a network address structure (binary) (network byte order)
  if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  // Tries to connect this socket with the socket of the remote host
  if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  // New thread will start execution by invoking "network_thread_f" routine
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  /* Look for and handle keypresses (continuously)*/ 
 /* for (;;) {
    // Control endpoint shows the direction of a transfer (input)
    // Data will be stored in a struct of 8 bytes (1: ASCII code of pressed key, 2: Unused, 3-8: others)
    libusb_interrupt_transfer(keyboard, endpoint_address, (unsigned char *) &packet, sizeof(packet), &transferred, 0);

    // If a complete packet has been received
    if (transferred == sizeof(packet)) {
      // Store in keystate 3 bytes in hex format (6 digits)
      sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0], packet.keycode[1]);
      printf("%s\n", keystate);		// Print the 3 bytes in terminal
      fbputs(keystate, 6, 0);		// Print the 3 bytes in the screen
      if (packet.keycode[0] == 0x29) { 
	break;
      }
    }
  }
*/

  char recvkey[BUFFER_SIZE];
  int keycount = 0;
  recvkey[0] = '_';
  for (;;) {

    libusb_interrupt_transfer(keyboard, endpoint_address, (unsigned char *) &packet, sizeof(packet), &transferred, 0);
    if (transferred == sizeof(packet)) {

	if (packet.keycode[0] == 0x2a && keycount > 0) recvkey[--keycount] = '_';	// Backspace
	// else if for arrows
	else if (packet.keycode[0] == 0x2c) recvkey[keycount++] = ' ';	// Space character
	
	else if ((packet.modifiers == 0x02 || packet.modifiers == 0x20) && packet.keycode[0] != 0) {	// Use of shift
		recvkey[keycount++] = HDI_to_ASCII(packet.keycode[0]-4) - 32;
		recvkey[keycount] = '_';
	}
	else {
		recvkey[keycount++] = HDI_to_ASCII(packet.keycode[0]-4);
		recvkey[keycount] = '_';
	}


      //sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0], packet.keycode[1]);


      printf("%s\n", recvkey);		// Print the 3 bytes in terminal
      fbputs(recvkey, 22, 0);		// Print the 3 bytes in the screen
      if (packet.keycode[0] == 0x29) { /* ESC pressed? */
	break;
      }
    }
  }

  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

void *network_thread_f(void *ignored)
{
  char recvBuf[BUFFER_SIZE];
  // Use two new buffers if we write in 2 lines (handle wrap around)
  // First buffer takes the first line + 1 NULL character to be terminated
  char recvBuf1[65], recvBuf2[64];
  int n;
  int counter = 0;
  /* Receive data */
  while ( (n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
    recvBuf[n-1] = '\0';
    recvBuf[n-2] = ' ';		// Replace Shift-Out character with blank
    printf("%s\n", recvBuf);	// Use of '\n' is necessary to unblock this printf (we have another one from keyboard)
    //fbputs(recvBuf, 8, 0);
 
    if (counter == 21) counter = 0; // Wrap back to first line if all the lines are written
    if (n > 64) {
        memcpy(recvBuf1, recvBuf, 64);
        recvBuf1[64] = '\0';
        memcpy(recvBuf2, recvBuf+64, 64);
        fbputs(recvBuf1, counter++, 0);
        if (counter == 21) counter = 0;
        fbputs(recvBuf2, counter++, 0);
        }
    else
    fbuts(recvBuf, counter++, 0);
  }

  return NULL;
}

