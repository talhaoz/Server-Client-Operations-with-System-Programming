
/* 
*/

/* Written by Talha OZ, on 20 march 2017*/
/* Student ID:131044003 ---  HW3 ---- */



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


/***** Variables ********/
int mainFifofd=0;
int clientFifofd=0;
int matrixTxtfd=0;
int sendParentPidfd=0;
int serverToClientFifofd=0;
int sleepTime=0;
int totalLoop=0;
long serverPidInt=0;
long checkServerPid=0;
time_t res1TimeStart=0;
time_t res1TimeEnd=0;
time_t res2TimeStart=0;
time_t res2TimeEnd=0;
double res1Diff=0.0;
double res2Diff=0.0;

char mainFifoName[FILE_NAME_MAX]="";
char serverToClientFifo[FIFO_NAME_MAX]="";
char matrixTxtFile[FILE_NAME_MAX]="";
char sendParentPidFifo[FILE_NAME_MAX]="";
char *clientFifoName="";
char stringForLogFile[2048]="";
char stringForShowResult[2048]="";
char stringForShowResult2[2048]="";

pid_t clientPid=0;
pid_t serverPid=0;
pid_t comingPid=0;
pid_t parentPid=0;
long showPid=0;

FILE *comingMatrix,*comingServerPid,*writeLogFile,*forScreen,*forLogFile,*clientPidToShow,*showProgPid;

/* these variables for the matrix */
int i,j,n=0;
double detMain=0,det1=0,det2=0,det3=0,det4=0;
double matrix1[20][20],matrix2[20][20],matrix3[20][20],matrix4[20][20];
double inverseMatrix1[20][20],inverseMatrix2[20][20],inverseMatrix3[20][20],inverseMatrix4[20][20];
double mainMatrix[20][20];
double twoDconvulotionMatrix[20][20];
double shiftedInverseMatrix[20][20];
double res1=0.0; /* result1 (det(org matris) - det(shifted inverse matris) )  */
double res2=0.0;
struct sigaction newSig;


/*Helper functions  */
void doSleep(int sleepTime);
double result1Calculater(int n);
double result2Calculater(int n);
void twoDConvolution(double in[20][20],double out[20][20],int n);
void cofactor(double num[20][20],int f,double inverseMatrix[20][20]);
void transpose(double num[2][20], double fac[20][20], int r,double inverseMatrix[20][20]);
double determinant(double f[20][20],int x);
void printLogFile(double matrix[20][20],int n);

void signalHandler(int caughtSignal)
{
	char tempStr[1024]="";
	if(caughtSignal==SIGINT)
	{
		printf("SIGINT caught !\n");
		sprintf(tempStr,"\n\n***KILL SIGNAL CAUGHT at time: %ld ***",(long)time(NULL));
		fprintf(writeLogFile,"%s",tempStr);
		showProgPid=fopen("showProgPid.txt","r");
		fscanf(showProgPid,"%ld",&showPid);
		remove("showProgPid.txt");
		kill(showPid,SIGINT);
		kill(parentPid,SIGINT);
		kill(getpid(),SIGINT);
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

	/* if there is not 3 arguments in the command line print an error and exit*/
	if(argc != TWO_ARGS)
	{
		fprintf(stderr,"Usage: Command line format must be => %s <mainPipeName>\n", argv[0]);
		return 1;
	}
	printf("Client has started..\n");
	strcat(mainFifoName,argv[1]); /* setting writing log file name*/


	/* opening fifo file with write perms  */
	/* if any error occurs while opening the file ,print an error message then exit */
	if ((mainFifofd = open(mainFifoName, O_RDWR) ) == -1)
	{
		fprintf(stderr,"Failed to open %s(Client),please make sure that server is working!\n", mainFifoName);
		return 1;
	}

	clientPid=getpid();

	/*client pid is gonna send to the show res prog*/
	clientPidToShow=fopen("clientPidToShow.txt","a");
    fprintf(clientPidToShow, "%ld\n", (long)clientPid);
    fclose(clientPidToShow);
	

	while(TRUE)
	{

		/*write pid to the mainFifo*/
		r_write(mainFifofd,&clientPid,sizeof(clientPid));

		if(totalLoop==0)
		{
			sprintf(sendParentPidFifo,"%s.parentPid", mainFifoName);
			while((sendParentPidfd= open(sendParentPidFifo, O_RDWR) ) == -1);
			/*{
				fprintf(stderr,"Failed to open %s(Client) to read parentPid from server! \n", sendParentPidFifo);
				return 1;
			} */

			if (r_read(sendParentPidfd,&parentPid,sizeof(pid_t))!=sizeof(pid_t))
			{
				fprintf(stderr,"Failed to read client pid ");
				exit(1);
			}
		}

		printf("Client pid sent..\n");
		sprintf(serverToClientFifo,"%ld.serverPid.txt", (long) clientPid);

		printf("Server pid reading..\n");
		while(TRUE)
		{
			comingServerPid=fopen(serverToClientFifo,"r");

			if(comingServerPid!=NULL)
				break;
		}
		
		/* reading serverid from txt file */
		for(;;)
		{	
			checkServerPid=serverPidInt;
			fscanf(comingServerPid,"%ld", &serverPidInt);
			if(checkServerPid!=serverPidInt)
				break;

			doSleep(1);
		}
		

		serverPid=(pid_t)serverPidInt;
		
		kill(serverPid,SIGUSR1);
	    
		sprintf(matrixTxtFile,"%ld.txt", (long) serverPid);

		while(TRUE)
		{
			comingMatrix=fopen(matrixTxtFile,"r");

			if(comingMatrix!=NULL)
				break;
		}

		printf("Matrix Reading...\n");
		fscanf(comingMatrix,"%d", &n);
		for(i=0; i<2*n; i++)
		{
			for(j=0; j<2*n; j++)
			{	
				fscanf(comingMatrix,"%lf", &mainMatrix[i][j]);
			}
		}

		sprintf(stringForLogFile,"log/%ld.seeWhat(%d).log", (long)clientPid,totalLoop+1);
		writeLogFile=fopen(stringForLogFile,"w");
		sprintf(stringForShowResult,"toPrintScreen.txt");
		forScreen=fopen(stringForShowResult,"a");
		sprintf(stringForShowResult2,"toPrintLogFile.txt");
		forLogFile=fopen(stringForShowResult2,"a");

		if(fork()==0)
		{
			res1TimeStart=time(NULL);
			res1=result1Calculater(n);
			res1TimeEnd=time(NULL);
			res1Diff=difftime(res1TimeEnd,res1TimeStart);

			if(fork()==0)
			{	
				time(&res2TimeStart);
				res2=result2Calculater(n);
				time(&res2TimeEnd);
				res2Diff=difftime(res2TimeEnd,res2TimeStart);

				/*Writing matrices to log file for seeWhat in matlab format*/
				fprintf(writeLogFile,"Original Matris:\n");
				printLogFile(mainMatrix,n);
				fprintf(writeLogFile,"\n\nShifted Inverse Matris:\n");
				printLogFile(shiftedInverseMatrix,n);
				fprintf(writeLogFile, "\n\n2d Convulotion Matris(with kernel):\n");
				printLogFile(twoDconvulotionMatrix,n);
				/*End of writing  */
				fprintf(forScreen,"%ld %f %f\n", (long)clientPid,res1,res2);
				fprintf(forLogFile,"%ld %f %f\n", (long)clientPid,res1Diff,res2Diff);
	
			}

			exit(0);
		}
		printf("Matrix read,result1 and result2 calculated successfully!\n");
		totalLoop++;
		remove(matrixTxtFile);
		remove(serverToClientFifo);
		close(sendParentPidfd);

		doSleep(4);

	}


	return 0;
}

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


double result1Calculater(int n)
{
	for(i=0; i<n; i++)
	{
		for(j=0; j<n; j++)
		{
			matrix1[i][j]=mainMatrix[i][j];
			matrix2[i][j]=mainMatrix[i][j+n];
			matrix3[i][j]=mainMatrix[i+n][j];
			matrix4[i][j]=mainMatrix[i+n][j+n];

		}
	}
	cofactor(matrix1,n,inverseMatrix1);
	cofactor(matrix2,n,inverseMatrix2);
	cofactor(matrix3,n,inverseMatrix3);
	cofactor(matrix4,n,inverseMatrix4);

	for(i=0; i<n; i++)
	{
		for(j=0; j<n; j++)
		{
			shiftedInverseMatrix[i][j]=inverseMatrix1[i][j];
			shiftedInverseMatrix[i][j+n]=inverseMatrix2[i][j];
			shiftedInverseMatrix[i+n][j]=inverseMatrix3[i][j];
			shiftedInverseMatrix[i+n][j+n]=inverseMatrix4[i][j];

		}
	}

	return determinant(mainMatrix,2*n)-determinant(shiftedInverseMatrix,2*n);
}

double result2Calculater(int n)
{
	twoDConvolution(mainMatrix,twoDconvulotionMatrix,2*n);

	return determinant(mainMatrix,2*n)-determinant(twoDconvulotionMatrix,2*n);
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

/* This code Taken from  => http://www.sanfoundry.com/c-program-find-inverse-matrix/  ***/
void cofactor(double num[20][20],int f,double inverseMatrix[20][20])
{
 	double b[20][20],fac[20][20];
 	int p, q, m, n, i, j;
 	for (q = 0;q < f; q++)
 	{
   		for (p = 0;p < f; p++)
    	{
    		m = 0;
     		n = 0;
     		for (i = 0;i < f; i++)
     		{
       			for (j = 0;j < f; j++)
        		{
          			if (i != q && j != p)
          			{
            			b[m][n] = num[i][j];
            			if (n < (f - 2))
             				n++;
            			else
             			{
               				n = 0;
               				m++;
               			}
            		}
        		}
      		}

      		fac[q][p] = pow(-1, q + p) * determinant(b, f - 1);
    	}
  	}

  transpose(num, fac, f,inverseMatrix);
}

/* This code Taken from  => http://www.sanfoundry.com/c-program-find-inverse-matrix/  ***/
void transpose(double num[20][20], double fac[20][20], int r,double inverseMatrix[20][20])
{
  	int i, j;
  	double b[20][20],d;
 
  	for (i = 0;i < r; i++)
    {
     	for (j = 0;j < r; j++)
       	{
         	b[i][j] = fac[j][i];
        }
    }

  	d = determinant(num, r);

  	for (i = 0;i < r; i++)
    {
     	for (j = 0;j < r; j++)
       	{
        	inverseMatrix[i][j] = b[i][j] / d;
        }
    }

  
}

/* This code Taken from  => http://stackoverflow.com/questions/3982439/fast-2d-convolution-for-dsp  ***/
void twoDConvolution(double in[20][20],double out[20][20],int n)
{
	int k=3,i,j,ii,jj;
	int data,coeff,sum;

	int coeffs[3][3]={{0,0,0},{0,1,0},{0,0,0}};


	for (i = k / 2; i < n - k / 2; ++i) /* iterate through image */
	{
	  	for (j = k / 2; j < n - k / 2; ++j)
	  	{
	    	sum = 0; /* sum will be the sum of input data * coeff terms */

	   	 	for (ii = - k / 2; ii <= k / 2; ++ii) /* iterate over kernel */
	    	{
	      		for (jj = - k / 2; jj <= k / 2; ++jj)
	      		{
	        		data = in[i + ii][j +jj];
	        		coeff = coeffs[ii + k / 2][jj + k / 2];

	        		sum += data * coeff;
	      		}
	    	}

	    	out[i][j] = sum; /* scale sum of convolution products and store in output */
	  	}
	}

}

void printLogFile(double matrix[20][20],int n)
{
	fprintf(writeLogFile,"[");
	for(i=0; i<2*n; i++)
	{
		for(j=0; j<2*n; j++)
		{	
			fprintf(writeLogFile, "%f ", matrix[i][j]);
		}

		if(i==2*n-1)
			fprintf(writeLogFile,"]");

		if(i!=2*n-1)
			fprintf(writeLogFile,";");
	}
}

















