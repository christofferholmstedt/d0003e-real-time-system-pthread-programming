/*****************************************************************************
* Original from:
* FILE: hello.c
* DESCRIPTION:
*   A "hello world" Pthreads program.  Demonstrates thread creation and
*   termination.
* AUTHOR: Blaise Barney
* LAST REVISED: 08/09/11
******************************************************************************/

/***
 * Heavily modified by Christoffer Holmstedt
 * 2011-10-16
 **/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define NUM_THREADS 20	

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

/*
void *PrintHello(void *threadid)
{
   long tid;
   tid = (long)threadid;
   while (1) {
        printf("Hello World! It's me, thread #%ld!\n", tid);
   }
   pthread_exit(NULL);
}
*/
/*
int main(int argc, char *argv[])
{
   pthread_t threads[NUM_THREADS];
   int rc;
   long t;
   for(t=0;t<NUM_THREADS;t++){
     printf("In main: creating thread %ld\n", t);
     rc = pthread_create(&threads[t], NULL, PrintHello, (void *)t);
     if (rc){
       printf("ERROR; return code from pthread_create() is %d\n", rc);
       exit(-1);
       }
     }

   // Last thing that main() should do
   printf("Last call...shutting down...\n");
   pthread_exit(NULL);
}
*/
