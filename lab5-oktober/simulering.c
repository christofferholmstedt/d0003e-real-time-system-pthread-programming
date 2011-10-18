/***
 * Written by Christoffer Holmstedt
 * 2011-10-16
 **/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <termios.h> // COMport communication - http://uw714doc.sco.com/en/man/html.3C/termios.3C.html

#include <sys/types.h> // File access - http://uw714doc.sco.com/en/man/html.2/open.2.html
#include <sys/stat.h>
#include <fcntl.h>


#define NORTHBOUND_CAR_ARRIVAL 0x01 // Transmission
#define NORTHBOUND_BRIDGE_SENSOR 0x02
#define SOUTHBOUND_CAR_ARRIVAL 0x04
#define SOUTHBOUND_BRIDGE_SENSOR 0x08
#define WIPEOUT_ALL_CARS_IN_BOTH_QUEUES 0xCC

#define NORTHBOUND_GREEN_LIGHT 0x01 // Received
#define NORTHBOUND_RED_LIGHT 0x02 
#define SOUTHBOUND_GREEN_LIGHT 0x04 
#define SOUTHBOUND_RED_LIGHT 0x08 

#define TRUE 1
#define FALSE 0

#define GREEN 1
#define RED 0

struct bridgeStatus{
   unsigned int southQ;
   unsigned int northQ;
   unsigned int northboundTrafficlight;
   unsigned int southboundTrafficlight;
   unsigned int stopViewer;
};

struct bridgeStatus bridgeStatus;
pthread_mutex_t mutexBridgeStatus;

unsigned int Com1;
/*************************************************
 * viewer thread/function
 *************************************************/
 void *viewer(void *bridgeStatusInput)
{
	// Bridgestatus variables
	struct bridgeStatus *bridgeStatusIO;
	bridgeStatusIO = (struct bridgeStatus *) bridgeStatusInput;
	
	// Date and time information
	// Source: http://www.java2s.com/Code/C/Development/PrintlocalandUTCtime.htm
	struct tm *local;
	time_t t;
	
	while (bridgeStatus.stopViewer == FALSE)
	{
		pthread_mutex_lock (&mutexBridgeStatus);
		t = time(NULL);
		local = localtime(&t);
		printf("\n\n\n\n\n/------------------------------\\\n");
		printf("| %s", asctime(local));
		printf("| Press \"n\" for new northbound car\n");
		printf("| Press \"s\" for new southbound car\n");
		printf("| Press \"q\" to quit\n");
		// printf("| Press \"c\" to wipe both queues (testing purpose)\n");
		printf("|\n| ----- Bridgestatus -----\n");
		printf("| Northbound cars in queue: %d\n", bridgeStatus.northQ);
		printf("| Southbound cars in queue: %d\n", bridgeStatus.southQ);
		
		// Northbound traffic lights
		if (bridgeStatus.northboundTrafficlight == GREEN) {
			printf("| Northbound has green light.\n");
		} else {
			printf("| Northbound has red light.\n");
		}
		
		// Southbound traffic lights
		if (bridgeStatus.southboundTrafficlight == GREEN) {
			printf("| Southbound has green light.\n");
		} else {
			printf("| Southbound has red light.\n");
		}
		printf("\\------------------------------/\n");
		pthread_mutex_unlock (&mutexBridgeStatus);
		sleep(1);
	}
	pthread_exit(NULL);
}

/*************************************************
 * io thread/function
 *************************************************/
void *iothread(void *bridgeStatusInput)
{
	// Bridgestatus variables
	struct bridgeStatus *bridgeStatusIO;
	bridgeStatusIO = (struct bridgeStatus *) bridgeStatusInput;
	
	// Program specific variables
	unsigned char characterInput = 0;
	unsigned int c, trafficLightSignal, readFromCom1 = FALSE, readFromKeyboard = FALSE;	
	struct termios Com1Config;
	int transmitData;
	
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
				pthread_mutex_lock (&mutexBridgeStatus);
				bridgeStatus.northQ++;
				pthread_mutex_unlock (&mutexBridgeStatus);
				
				transmitData = NORTHBOUND_CAR_ARRIVAL;
				write(Com1, &transmitData, 1);
				// printf("Northbound cars: %d\n",bridgeStatus.northQ);
				
				
			} else if (c == 115)
			{
				// Southbound car arrival sensor activated
				pthread_mutex_lock (&mutexBridgeStatus);
				bridgeStatus.southQ++;
				pthread_mutex_unlock (&mutexBridgeStatus);
				
				transmitData = SOUTHBOUND_CAR_ARRIVAL;
				write(Com1, &transmitData, 1);
				// printf("Southbound cars: %d\n",bridgeStatus.southQ);
				
			/***
			 * TESTING TRANSMISSION
			 */
			/*
			} else if (c == 109)
			{
				// Northbound bridge entry sensor activated
				// printf("You pressed m\n");
				transmitData = NORTHBOUND_BRIDGE_SENSOR;
				write(Com1, &transmitData, 1);
			} else if (c == 100)
			{
				// Southbound bridge entry sensor activated
				// printf("You pressed d\n");
				transmitData = SOUTHBOUND_BRIDGE_SENSOR;
				write(Com1, &transmitData, 1);
			*/
			/***
			 * END OF TESTING
			 */
			} else if (c == 113)
			{
				// Quit if q is pressed
				pthread_mutex_lock (&mutexBridgeStatus);
				bridgeStatus.stopViewer = TRUE;
				pthread_mutex_unlock (&mutexBridgeStatus);
				break;
			} else if (c == 99)
			{
				// Press C and wipeout all cars in both queues with a big laser!
				pthread_mutex_lock (&mutexBridgeStatus);
				bridgeStatus.northQ = 0;
				bridgeStatus.southQ = 0;
				pthread_mutex_unlock (&mutexBridgeStatus);
				// printf("Northbound cars: %d\n",bridgeStatus.northQ);
				// printf("Southbound cars: %d\n",bridgeStatus.southQ);
				
				transmitData = WIPEOUT_ALL_CARS_IN_BOTH_QUEUES;
				write(Com1, &transmitData, 1);
			}/* else {
				printf("Press \"n\" to add a northbound car\n");
				printf("Press \"s\" to add a southbound car\n");
				printf("Press \"q\" to quit\n");
			}*/
			// Include keyboard again after it has been removed by select() function when select() returned.
			FD_SET(Com1, &setOfFDs);
			readFromKeyboard = FALSE; // Reset
			transmitData = 0; // Reset
		}
		
		// Take care of Com1 input
		if(readFromCom1)
		{
			trafficLightSignal = read(Com1, &characterInput , 1);
			if (characterInput == NORTHBOUND_GREEN_LIGHT)
			{
				pthread_mutex_lock (&mutexBridgeStatus);
				bridgeStatus.northboundTrafficlight = GREEN;
				if (bridgeStatus.northQ > 0) bridgeStatus.northQ--;
				pthread_mutex_unlock (&mutexBridgeStatus);
				
				transmitData = NORTHBOUND_BRIDGE_SENSOR;
				write(Com1, &transmitData, 1);
				
			} else if (characterInput == NORTHBOUND_RED_LIGHT)
			{
				pthread_mutex_lock (&mutexBridgeStatus);
				bridgeStatus.northboundTrafficlight = RED;
				pthread_mutex_unlock (&mutexBridgeStatus);
				
			} else if (characterInput == SOUTHBOUND_GREEN_LIGHT)
			{
				pthread_mutex_lock (&mutexBridgeStatus);
				bridgeStatus.southboundTrafficlight = GREEN;
				if (bridgeStatus.southQ > 0) bridgeStatus.southQ--;
				pthread_mutex_unlock (&mutexBridgeStatus);
				
				transmitData = SOUTHBOUND_BRIDGE_SENSOR;
				write(Com1, &transmitData, 1);
				
			} else if (characterInput == SOUTHBOUND_RED_LIGHT)
			{
				pthread_mutex_lock (&mutexBridgeStatus);
				bridgeStatus.southboundTrafficlight = RED;
				pthread_mutex_unlock (&mutexBridgeStatus);
				
			} else {
				printf("I didn't understand input from COM1 port (Decimal value): %d\n", characterInput);
			}
			// Include keyboard again after it has been removed by select() function when select() returned.
			FD_SET(0, &setOfFDs);
			readFromCom1 = FALSE; // Reset
		}
	}
	
   pthread_exit(NULL);
}

/*************************************************
 * Main function/thread.
 *************************************************/
int main(int argc, char *argv[])
{
	long t;
	pthread_t threads[2];
	
	bridgeStatus.northboundTrafficlight = RED; // Startup, default values
	bridgeStatus.southboundTrafficlight = RED;
	bridgeStatus.stopViewer = FALSE; 
	
	pthread_create(&threads[0], NULL, iothread, (void *) &bridgeStatus);
	pthread_create(&threads[1], NULL, viewer, (void *) &bridgeStatus);
	 
	// Last thing that main() should do
	pthread_exit(NULL);
}