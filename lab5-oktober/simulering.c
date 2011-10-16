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

/*************************************************
 * Main thread.
 *************************************************/
int main(int argc, char *argv[])
{
    int c;

    while((c = getchar()) != EOF)
    {
        if (c == 10)
        {
            // Do nothing when entered is pressed.
        } else if (c == 110)
        {
            printf("You pressed n\n");
        } else if (c == 115)
        {
            printf("You pressed s\n");
        } else if (c == 113)
        {
            break;
        } else {
            printf("Press \"n\" to add a northbound car\n");
            printf("Press \"s\" to add a southbound car\n");
            printf("Press \"q\" to quit\n");
        }
    } 
    
   // Last thing that main() should do
   printf("Last call...shutting down...\n");
   pthread_exit(NULL);
}