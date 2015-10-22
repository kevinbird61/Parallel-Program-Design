#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>

#define TRY 1000000000
int throwDart();

int main(int argc,char *argv[])
{
	long i;
	long number_in_circle=0,total_number=0,inputNum;
	int my_rank,comm_sz;
	float pi,pi_temp;
	// For Parallel count
	long start,end;
	// For Store the msg of process 1~comm_sz
	char time[100],number[100];
	// Initialize MPI
	MPI_Init(&argc,&argv);
	
	// Set MPI 
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&comm_sz);
	inputNum = UINT_MAX;
	//printf("Please input the number of tosses:\n");
	//scanf("%d",&inputNum);
	MPI_Bcast(&inputNum,1,MPI_LONG,0,MPI_COMM_WORLD);
	// Separate
	start = (long)(0+(inputNum/comm_sz)*my_rank);
	end = (long)(start + (inputNum/comm_sz));
	
	double startTime=0.0 , totalTime=0.0;
	startTime = MPI_Wtime();
	for(i=start;i<=end;i++){
		number_in_circle += throwDart();
	}
	totalTime = MPI_Wtime()-startTime;
	/* Send to all process */
	pi_temp = (float)4*((float)number_in_circle)/((float)inputNum);
	printf("The process %d 's pi_temp is %f\n",my_rank,pi_temp);
	MPI_Reduce(&pi_temp,&pi,1,MPI_FLOAT,MPI_SUM,0,MPI_COMM_WORLD);	

	if(my_rank==0){
		printf("\nprocess %d finished in time %f sec\n\n",my_rank,totalTime);
		printf("Pi estimate: %f\n",pi);
	}
	
	MPI_Finalize();
	return 0;
}

int throwDart()
{
	float x,y,Hit;
	srand(time(NULL));
	// rand() : 0~ UINT_MAX-1
	x = ((float)rand()/(float)UINT_MAX)*(float)2 - (float)1;
	y = ((float)rand()/(float)UINT_MAX)*(float)2 - (float)1;
	Hit = (x)*(x) + (y)*(y);
	if(Hit<= (float)1.0){
		return 1;
	}
	return 0;
}
