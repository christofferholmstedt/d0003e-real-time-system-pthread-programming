/***
 * Written by Christoffer Holmstedt
 * 2011-10-16
 **/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <termios.h> // COMport communication - http://uw714doc.sco.com/en/man/html.3C/termios.3C.html

#include <sys/types.h> // File access - http://uw714doc.sco.com/en/man/html.2/open.2.html
#include <sys/stat.h>
#include <fcntl.h>

#define NORTHBOUND_CAR_ARRIVAL 0x01 // Received
#define NORTHBOUND_BRIDGE_SENSOR 0x02
#define SOUTHBOUND_CAR_ARRIVAL 0x04 
#define SOUTHBOUND_BRIDGE_SENSOR 0x08

#define NORTHBOUND_GREEN_LIGHT 0x01 // Transmission
#define NORTHBOUND_RED_LIGHT 0x02 
#define SOUTHBOUND_GREEN_LIGHT 0x04 
#define SOUTHBOUND_RED_LIGHT 0x08 

#define TRUE 1
#define FALSE 0

unsigned int Com1;

void write_data(int arg);

/*************************************************
 * Author: Ib Lundgren
 *************************************************/
// Flush data to the avr, avr_fd is global so use mutex to be safe
void write_data(int data) {
    write(Com1, &data, 1);  
}
/*************************************************
 * End Author: Ib Lundgren
 *************************************************/
 
/*************************************************
 * Main thread.
 *************************************************/
int main(int argc, char *argv[])
{
	unsigned char characterInput = 0;
	unsigned int c, trafficLightSignal, readFromCom1 = FALSE, readFromKeyboard = FALSE;	
	struct termios Com1Config;

	// Original code/structure from lecture 14 presentation images
	Com1 = open("/dev/ttyS0", O_RDWR | O_NONBLOCK); 	// open comport

	// Com1 Com1Configuration with termios - http://en.wikibooks.org/wiki/Serial_Programming/termios
	if(tcgetattr(Com1, &Com1Config) < 0)
	{
		printf("Com1 has no Com1Configuration, is it really a Com device?\n");
	}
	
	// Modified configuration example below from http://tldp.org/HOWTO/Serial-Programming-HOWTO/x115.html "Canonical Input Processing"
	bzero(&Com1Config, sizeof(Com1Config)); /* clear struct for new port settings */
	Com1Config.c_cflag = CS8 | CSTOPB | CLOCAL | CREAD;
	Com1Config.c_oflag = 0;	
	Com1Config.c_iflag = 0;
	Com1Config.c_lflag = 0;
	Com1Config.c_cc[VMIN]  = 1;
	Com1Config.c_cc[VTIME] = 5;

	if(cfsetispeed(&Com1Config, B9600) < 0 || cfsetospeed(&Com1Config, B9600) < 0) {
		// ... error handling ...
		printf("Com1 speed settings doesn't work.\n");
	}
	
	if(tcsetattr(Com1, TCSANOW, &Com1Config) < 0)
	{ 
		// ... error handling ...
		printf("Com1 settings hasn't been set.\n");
	}
	
	// Get going with several file descriptors waiting for input.
	fd_set setOfFDs;					// More information about SELECT - http://www.delorie.com/gnu/docs/glibc/libc_248.html
	FD_ZERO(&setOfFDs); 				// Initialize
	FD_SET(0, &setOfFDs); 				// include keyboard
	FD_SET(Com1, &setOfFDs); 			// include com1:
	
	// How select functions http://linux.die.net/man/2/select_tut
	// Read about "Arguments". Short version: select removes thoose filedescriptors
	// that do _not_ get any input from "setOfFDs" therfore you need to add them back
	// to &setOfFDs before the while loop runs another loop.
	while (select(4,&setOfFDs,NULL,NULL,NULL))
	{
		// Check which file descriptors returned true from select().
		// Only thoose that have something of value will be left in &setOfFDs
		if(FD_ISSET(0,&setOfFDs)) {
			 readFromKeyboard = TRUE;
			
		} else if (FD_ISSET(Com1,&setOfFDs)) {
			 readFromCom1 = TRUE;
		}
		
		// Take care of keyboard input
		if(readFromKeyboard) {
			c = getchar();
			if (c == 10)
			{
				// Do nothing when entered is pressed.
			} else if (c == 110)
			{
				// Northbound car arrival sensor activated
				printf("You pressed n\n");
				write_data(NORTHBOUND_CAR_ARRIVAL);
			} else if (c == 115)
			{
				// Southbound car arrival sensor activated
				printf("You pressed s\n");
				write_data(SOUTHBOUND_CAR_ARRIVAL);
			/***
			 * TESTING TRANSMISSION
			 */
			} else if (c == 109)
			{
				// Northbound bridge entry sensor activated
				printf("You pressed m\n");
				write_data(NORTHBOUND_BRIDGE_SENSOR);
			} else if (c == 100)
			{
				// Southbound bridge entry sensor activated
				printf("You pressed d\n");
				write_data(SOUTHBOUND_BRIDGE_SENSOR);
			/***
			 * END OF TESTING
			 */
			} else if (c == 113)
			{
				// Quit if q is pressed
				break;
			} else {
				printf("Press \"n\" to add a northbound car\n");
				printf("Press \"s\" to add a southbound car\n");
				printf("Press \"q\" to quit\n");
			}
			// Include keyboard again after it has been removed by select() function when select() returned.
			FD_SET(Com1, &setOfFDs);
			readFromKeyboard = FALSE; // Reset
		}
		
		// Take care of Com1 input
		if(readFromCom1)
		{
			trafficLightSignal = read(Com1, &characterInput , 1);
			if (characterInput == 1)
			{
				printf("Northbound has now green light.\n");
			} else if (characterInput == 2)
			{
				printf("Northbound has now red light.\n");
			} else if (characterInput == 4)
			{
				printf("Southbound has now green light.\n");
			} else if (characterInput == 8)
			{
				printf("Southbound has now red light.\n");
			} else {
				printf("I didn't understand input from COM1 port (Decimal value): %d\n", characterInput);
			}
			// Include keyboard again after it has been removed by select() function when select() returned.
			FD_SET(0, &setOfFDs);
			readFromCom1 = FALSE; // Reset
		}
	}

   // Last thing that main() should do
   printf("Last call...shutting down...\n");
   pthread_exit(NULL);
}