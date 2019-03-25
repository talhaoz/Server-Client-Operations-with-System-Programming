
/* 
*/

/* Written by Talha OZ, on 20 march 2017*/
/* Student ID:131044003 ---  HW3 ---- */



/******  Includes  ********/
#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* to generate rendom numbers */
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <dirent.h>
#include "restart.h" /* taken from aliyasineser github */


/******  Macros  ********/
#define FOUR_ARGS 4
#define WORD_SIZE 10
#define FILE_NAME_MAX 20
#define FIFO_NAME_MAX 20
#define WRITE_FLAGS (O_WRONLY | O_APPEND | O_CREAT)
#define WRITE_PERMS (S_IRUSR | S_IWUSR)
#define FIFO_PERM (S_IRUSR | S_IWUSR | S_IWGRP | S_IWOTH | S_IROTH)
#define TRUE 1

/***** Variables ********/
int mainFifofd=0;
int serverToClientFifofd=0;
int sendParentPidfd=0;
int isRequest=-2; /* to check coming request signal from client*/
int fdLogFileWrite=0;
long tempServerPid=0;

char mainFifoName[FIFO_NAME_MAX]="";
char matrixTxtFile[FIFO_NAME_MAX]="";
char serverToClientFifo[FIFO_NAME_MAX]="";
char sendParentPidFifo[FIFO_NAME_MAX]="";
char stringForWrite[1024]="";
char stringForLogFile[2048]="";
char writeFileName[FILE_NAME_MAX]="";

int sleepTime=0;
int dimension=0;
pid_t clientPid=0;
pid_t serverPid=0;
pid_t newChild=-2;
pid_t parentPid=0;
size_t bytesread=0;
time_t timeMatrix=0;
int totalLoop=0;
struct sigaction newSig;

FILE *sendMatrix,*sendServerPid,*serverPidToShow;

/* these variables for the matrix */
int i,j;
double detMain=0,det1=0,det2=0,det3=0,det4=0;
double matrix1[20][20],matrix2[20][20],matrix3[20][20],matrix4[20][20];
double mainMatrix[20][20];

/***** Helper Functions  *****/
void invertibleMatrix(int n);
double determinant(double a[20][20], int n);
void createMatrix(int n,double tempMatrix[20][20],int randParam);
void signalHandler(int signal);
void doSleep(int sleepTime);

int main(int argc, char const *argv[])
{
	newSig.sa_handler=signalHandler;
	newSig.sa_flags=0;
	

	if ((sigemptyset(&newSig.sa_mask) == -1) ||
		(sigaction(SIGINT,&newSig,NULL) == -1) ||
		(isRequest=sigaction(SIGUSR1,&newSig,NULL)) ==-1) 
	{
       	fprintf(stderr,"Failed to caught signal !\n");
        exit(1);
    }

	/* block all coming signals */

	/* if there is not 3 arguments in the command line print an error and exit*/
	if(argc != FOUR_ARGS)
	{
		fprintf(stderr,"Usage: Command line format must be => %s <ticks in miliSeconds>  <n>  <mainPipeName>\n", argv[0]);
		return 1;
	}

	sleepTime=atoi(argv[1]);
	dimension=atoi(argv[2]);
	strcat(mainFifoName,argv[3]); /* setting writing log file name*/

	sleepTime=sleepTime/1000;
	
	if(mkdir("log",S_IRWXU | S_IRWXG)==-1)
	{
		fprintf(stderr, "Failed to making log directory!\n");
		exit(1);
	}

	sprintf(writeFileName,"log/timeServer.log");
	/* opening write log file *************************************************************/
	while((fdLogFileWrite = open(writeFileName, WRITE_FLAGS,WRITE_PERMS)) == -1 && errno == EINTR);
	
	/* if any error occurs while opening the file ,print an error message then exit */
	if (fdLogFileWrite == -1)
	{
		fprintf(stderr,"Failed to open %s\n", writeFileName);
		return 1;
	}
	/***********************************************************************************/

	/*/////////////////////// After here server things begin ////////////*/
	/* making fifo (taken from course book ) ***********************************/
	if (mkfifo(mainFifoName, FIFO_PERM) == -1) 
	{
		/* create a named pipe */
		if (errno != EEXIST) 
		{
			fprintf(stderr, "Failed to create named pipe %s: %s\n", mainFifoName, strerror(errno));
			exit(1);
		}
	}

	/* Opening main fifo*/
	if((mainFifofd = open(mainFifoName, O_RDWR)) == -1) 
	{
        fprintf(stderr,"Failed to open %s\n", mainFifoName);
		exit(1);
    }

    parentPid=getpid();
    /* the serverpid will send to the showRes prog */
    serverPidToShow=fopen("serverPidToShow.txt","w");
    fprintf(serverPidToShow, "%ld\n", (long)parentPid);
    fclose(serverPidToShow);
	printf("Server has started..\n");
	


    while(TRUE)
	{   
		

	    if ((bytesread = r_read(mainFifofd,&clientPid,sizeof(clientPid)))!=sizeof(clientPid))
		{
			fprintf(stderr,"Failed to read client pid ");
			exit(1);
		}
		printf("Client pid read successfully..\n");
		
		sprintf(sendParentPidFifo,"%s.parentPid",mainFifoName);
		/* making fifo (taken from course book ) ***********************************/
		if (mkfifo(sendParentPidFifo, FIFO_PERM) == -1) 
		{
			/* create a named pipe */
			if (errno != EEXIST) 
			{
				fprintf(stderr, "Failed to create named pipe %s: %s\n", mainFifoName, strerror(errno));
				exit(1);
			}
		}

		/* Opening main fifo*/
		if((sendParentPidfd = open(sendParentPidFifo, O_RDWR)) == -1) 
		{
	        fprintf(stderr,"Failed to open %s\n", mainFifoName);
			exit(1);
	    }

		r_write(sendParentPidfd,&parentPid,sizeof(pid_t));
		


		/* fork for directory search*/
    	if(bytesread > 0)
    	{
    		newChild=fork();
    	}

    	if (newChild == -1) 
		{
			fprintf(stderr,"Failed to fork for Client : %ld\n", (long) clientPid);
			exit(1);
		}

		/* child takes the current client, then send it selfpid*/
    	if(newChild==0 && bytesread > 0)
    	{
    		printf("Server pid sent to the client..\n");
    		serverPid=getpid();
			sprintf(serverToClientFifo,"%ld.serverPid.txt", (long) clientPid);

			/* opening txt file to send serverpid*/
			sendServerPid=fopen(serverToClientFifo,"w");
    		/* sending pid of server to the client */
    		fprintf(sendServerPid, "%ld\n", (long)serverPid);

    		while(TRUE)
    		{
		    	/* if isRequest is zero we have a request signal from client then we should send the matrix*/
		    	if(isRequest==0)
		    	{
		    		printf("Request signal received..\n");
		    		sprintf(matrixTxtFile,"%ld.txt", (long) serverPid);
					sendMatrix=fopen(matrixTxtFile,"a");
					invertibleMatrix(dimension);
					timeMatrix=time(NULL);

					fprintf(sendMatrix, "%d\n", dimension);
					for(i=0; i<2*dimension; i++)
					{
						for(j=0; j<2*dimension; j++)
						{	
							fprintf(sendMatrix, "%f ", mainMatrix[i][j]);
						}
	
					}
					printf("Matrix sent successfully !\n");
					sprintf(stringForLogFile,"Matrix create time: %ld\nClient Pid: %ld\nDeterminant of Matrix: %f\n\n",(long)timeMatrix,(long)clientPid,determinant(mainMatrix,dimension));

					write(fdLogFileWrite, stringForLogFile, strlen(stringForLogFile));

				   	fclose(sendMatrix);
				   	isRequest=-2;
				    break;
				}

				doSleep(sleepTime);
			}

			close(serverToClientFifofd);
			fclose(sendServerPid);

    	}
    	totalLoop++;
    	if(newChild==0)
    		break;
	
		doSleep(5);
		
	}
    


	return 0;
}

/* makes an invertible n by n matrix */
void invertibleMatrix(int n)
{
	srand(time(NULL));
	
	if(n<=0)
	{
		fprintf(stderr,"Matrix dimension cannot be less than 0!");
		exit(1);
	}
	
	while(detMain==0)
	{
		while(det1==0)
		{
			createMatrix(n,matrix1,rand());
			det1=determinant(matrix1,n);	
		}

		while(det2==0)
		{
			createMatrix(n,matrix2,rand());
			det2=determinant(matrix2,n);
		}
	
		while(det3==0)
		{
			createMatrix(n,matrix3,rand());
			det3=determinant(matrix3,n);
		}

		while(det4==0)
		{
			createMatrix(n,matrix4,rand());
			det4=determinant(matrix4,n);
		}

		for(i=0; i<n; i++)
		{
			for(j=0; j<n; j++)
			{
				mainMatrix[i][j]=matrix1[i][j];
				mainMatrix[i][j+n]=matrix2[i][j];
				mainMatrix[i+n][j]=matrix3[i][j];
				mainMatrix[i+n][j+n]=matrix4[i][j];

			}
		}
		detMain=determinant(mainMatrix,2*n);

	}	
}

/* create a matrix n*n dimension */
void createMatrix(int n,double tempMatrix[20][20],int randParam)
{
	int row,col;
	double randNum;

	srand(randParam);

	for(row=0;row<n;row++)
    {
        for(col=0;col<n;col++)
        {
            randNum=rand()%10;

            tempMatrix[row][col] =randNum;
        }
    }
}

/* This code Taken from  => http://www.sanfoundry.com/c-program-find-inverse-matrix/  ***/
double determinant(double a[20][20], int n)
{
	int i, j, row, col, flag=0;
	double cofMatrix=0,b[20][20];
    double res=0;


    /**/
	if(n == 1) { return a[0][0]; }

	if(n == 2) { return (a[0][0] * a[1][1] - a[0][1] * a[1][0]); }


	for(flag=0; flag<n; flag++)
	{
		row = 0;
		for(i=1; i<n; ++i)
		{			
			col = 0;
			for(j = 0; j < n; j++)
				if(j != flag)
				{
					b[row][col] = a[i][j];
					col++;
				}

			row++;
		}
        cofMatrix= pow(-1,flag)* a[0][flag];
		res+=cofMatrix*determinant(b, n-1);	
	}
        
	return res;
}


void signalHandler(int caughtSignal)
{
	/*char tempStr[1024]="";*/
	if(caughtSignal==SIGUSR1)
	{
		/* send matrix to client (delibaretely empty) */
	}
	else if(caughtSignal==SIGINT)
	{
		printf("SIGINT caught !\n");
		sprintf(stringForLogFile,"***KILL SIGNAL CAUGHT at time: %ld ***",(long)time(NULL));
		write(fdLogFileWrite, stringForLogFile, strlen(stringForLogFile));
		kill(clientPid,SIGINT);
		kill(getpid(),SIGINT);
		remove(sendParentPidFifo);
		remove(mainFifoName);
		exit(0);
	}
}

/* difftime func takes two times and return the differences */
void doSleep(int sleepTime)
{
	time_t sleepStart,currentTime;
	sleepStart=time(NULL);

	do
	{	
		currentTime=time(NULL);
		if(difftime(currentTime,sleepStart) > (double)sleepTime)
			break;
	}while(TRUE);

}