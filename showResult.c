

/******  Includes  ********/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include "restart.h" /* taken from aliyasineser github */


/******  Macros  ********/
#define TWO_ARGS 2
#define WORD_SIZE 10
#define FILE_NAME_MAX 20
#define FIFO_NAME_MAX 20
#define WRITE_FLAGS (O_WRONLY | O_APPEND | O_CREAT)
#define WRITE_PERMS (S_IRUSR | S_IWUSR)
#define FIFO_PERM (S_IRUSR | S_IWUSR | S_IWGRP | S_IWOTH | S_IROTH)
#define TRUE 1

/* Variables */
int count=1;
int fdWrite=0;
int numOfClients=0;
long clientPid=0;
long clientPid2=0;
long clientPids[100];
long serverPid=0;
pid_t selfPid=0;
double res1=0.0;
double res2=0.0;
double res1Time=0.0;
double res2Time=0.0;
char stringForShowResult[2048]="";
char stringForShowResult2[2048]="";
char stringForLogFile[2048]="";
char strTempForWrite[4096]="";

FILE *forScreen,*forLogFile,*showRes,*serverPidToShow,*clientPidToShow,*showProgPid;

struct sigaction newSig;

/* this signal handler handle SIGUSR1 signal and SIGINT signal*/
void signalHandler(int caughtSignal)
{
	char tempStr[1024]="";
	int i=0;

	if(caughtSignal==SIGINT)
	{
		/* killing all processes*/
		kill(serverPid,SIGINT);
		for(i=0; i<numOfClients; i++)
		{
			kill(clientPids[i],SIGINT);
		}

		printf("SIGINT caught !\n");
		sprintf(tempStr,"\n\n***KILL SIGNAL CAUGHT at time: %ld ***",(long)time(NULL));
		
		fclose(serverPidToShow);
		fclose(clientPidToShow);
		remove("serverPidToShow.txt");
		remove("clientPidToShow.txt");
		remove("toPrintScreen.txt");
		remove("toPrintLogFile.txt");

		write(fdWrite,tempStr, strlen(tempStr));
		
		exit(0);
	}
	else
	{
		fprintf(stderr,"Caught wrong signall!\n");
		exit(1);
	}
}



int main(int argc, char const *argv[])
{
	newSig.sa_handler=signalHandler;
	newSig.sa_flags=0;
	

	if ((sigemptyset(&newSig.sa_mask) == -1) ||
		(sigaction(SIGINT,&newSig,NULL) == -1)) 
	{
       	fprintf(stderr,"Failed to caught signal !\n");
        exit(1);
    }

    /*Usage*/
	if(argc!=1)
	{
		fprintf(stderr,"Usage: Command line format must be => %s (no arguments)\n", argv[0]);
	}

	sprintf(stringForShowResult,"toPrintScreen.txt");
	sprintf(stringForShowResult2,"toPrintLogFile.txt");
	sprintf(stringForLogFile,"log/showResult.log");
	
	/*if((showRes=fopen(stringForLogFile,"a"))==NULL)
	{
		fprintf(stderr,"Failed to open showResult.log,please make sure that log directory exist!");
		exit(1);
	} */

	/* opening file Read_Only mood */
	while((fdWrite = open(stringForLogFile,  WRITE_FLAGS,WRITE_PERMS)) == -1 && errno == EINTR);
	
	/* if any error occurs while opening the file ,print an error message then exit */
	if (fdWrite == -1)
	{
		fprintf(stderr,"Failed to open %s,  %s\n", stringForLogFile, strerror(errno));
		return 1;
	}
	/***********************************************************************************/

	/*serverPid is gonna read*/
	serverPidToShow=fopen("serverPidToShow.txt","r");
    fscanf(serverPidToShow, "%ld\n", &serverPid);
    
	clientPidToShow=fopen("clientPidToShow.txt","r");
    
    selfPid=getpid();
    showProgPid=fopen("showProgPid.txt","w");
    fprintf(showProgPid, "%ld\n", (long)selfPid);
    fclose(showProgPid);


	while(TRUE)
	{
		forScreen=fopen(stringForShowResult,"r");

		if(forScreen!=NULL)
			break;
	}

	while(TRUE)
	{
		forLogFile=fopen(stringForShowResult2,"r");

		if(forLogFile!=NULL)
			break;
	}
		

	

	/* Make easier to understand print what we re gonna print on the screen*/
	printf(" Pid        Result1            Result2\n");
	printf("-----      ---------          ---------\n");
	while(TRUE)
	{
		
		/* values for the screen */
		if(fscanf(clientPidToShow, "%ld\n", &clientPids[numOfClients])==1)
		{
			numOfClients++;
		}

		
		if(fscanf(forScreen,"%ld%lf%lf", &clientPid,&res1,&res2)==3)
		{
			printf("%ld    %f    %f\n",  clientPid,res1,res2);
		}

		if(fscanf(forLogFile,"%ld%lf%lf", &clientPid2,&res1Time,&res2Time)==3)
		{
			sprintf(strTempForWrite,"m%d\t\tpid=%ld\nresult1Time,\t%f\nresult2Time,\t%f\n\n",count,clientPid2,res1Time,res2Time);
			write(fdWrite,strTempForWrite, strlen(strTempForWrite));
			count++;
		}


	}



	return 0;
}