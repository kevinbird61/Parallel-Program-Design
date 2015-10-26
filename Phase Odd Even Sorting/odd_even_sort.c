#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/********************************
Notice:
This version could only implement
on input number "n" > processor 
total number 
And n must be the k*comm_sz 
Created by Kevin Chiu
********************************/

int *sort(int array[],int size);
int checkSort(int array[],int size);

int main(int argc,char *argv[]){
	/* 
	Local variable declaration
	*/
	int i,n,size,phase;
	int *unsorted,*sorted,*concat,*rc,*disp;
	int my_rank,comm_sz,start,end;
	// Start MPI
	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&comm_sz);
	//確保程式取得同步的正確性
	MPI_Barrier(MPI_COMM_WORLD);
	// Get the input amount
	if(my_rank==0){
	printf("Please input the number of random unsorted array:");
	scanf("%d",&n);
	}
	// Allocated the size of array
	sorted = (int*)malloc(n*sizeof(int));
	unsorted = (int*)malloc((n)*sizeof(int));
	for(i=0;i<n;i++){
		sorted[i]=0;
	}
	rc = (int*)malloc(n*sizeof(int));
	disp = (int*)malloc(n*sizeof(int));
	// Broadcast the n to every processor
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);
	// Decide the start/end
	if(my_rank==comm_sz-1){
		start = 0 + (n/comm_sz)*my_rank;
		end=start + (n/comm_sz)+(n%comm_sz);
		size = end-start;
		rc[my_rank] = size;
		//unsorted = (int*)malloc((size)*sizeof(int));
	}
	else{
		start = 0 + (n/comm_sz)*my_rank;
		end = start + (n/comm_sz);
		size = end-start;
		rc[my_rank] = size;
		//unsorted = (int*)malloc((size)*sizeof(int));
	}
	for(i=0;i<comm_sz;i++){
		disp[i] = (n/comm_sz)*i;
	}
	//MPI_Scatterv(sorted,rc,disp,MPI_INT,unsorted,size,MPI_INT,0,MPI_COMM_WORLD);
	srand(my_rank+1);
	for(i=0;i<size;i++){
		unsorted[i] = rand()%100;
		printf("In rank %d, generated: %d\n",my_rank,unsorted[i]);
	}
	// Sorted it at once first
	unsorted = sort(unsorted,size);
	// Define the concat array!
	phase = 0;
	concat = (int*)malloc(n*sizeof(int));
	// Parallel odd-even sorted
	for(phase=0;phase< (n%comm_sz!=0?n:comm_sz) ;phase++){
		// Do Even - and then judge
		if(phase%2==0){
			if(my_rank%2==0){
			// Turn of even processor
			// Recv and Sort the concat 
			for(i=0;i<size;i++){
				concat[i] = unsorted[i];
			}
			if(my_rank==comm_sz-2){
			MPI_Recv((concat+size),size+n%comm_sz,MPI_INT,my_rank+1,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			}
			else{
			MPI_Recv((concat+size),size,MPI_INT,my_rank+1,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			}
			//printf("In Even Sort concat\n");
			if(my_rank==comm_sz-2){
			concat = sort(concat,size*2+n%comm_sz);
			}else{
			concat = sort(concat,size*2);}
			// Discard the concat to 2 processor
			for(i=0;i<size;i++){
				unsorted[i] = concat[i];
			}
			if(my_rank==comm_sz-2){
			MPI_Send((concat+size),size+n%comm_sz,MPI_INT,my_rank+1,2,MPI_COMM_WORLD);
			}else{
			MPI_Send((concat+size),size,MPI_INT,my_rank+1,2,MPI_COMM_WORLD);}
			}
			else{
			// Turn of odd processor
			// Send the unsorted to even
			if(my_rank==comm_sz-1){
			MPI_Send(unsorted,size,MPI_INT,my_rank-1,1,MPI_COMM_WORLD);
			MPI_Recv(unsorted,size,MPI_INT,my_rank-1,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			}else{
			MPI_Send(unsorted,size,MPI_INT,my_rank-1,1,MPI_COMM_WORLD);
			MPI_Recv(unsorted,size,MPI_INT,my_rank-1,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			}
			}
		}
		// Do Odd - and then judge
		else{
			if(my_rank%2==0&&my_rank!=0){
			// Turn of even processor
			// Send the unsorted to odd
			MPI_Send(unsorted,size,MPI_INT,my_rank-1,3,MPI_COMM_WORLD);
			MPI_Recv(unsorted,size,MPI_INT,my_rank-1,4,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			}
			else if(my_rank%2!=0&&my_rank!=comm_sz-1){
			// Turn of odd processor
			// Recv and Sort the concat 
			for(i=0;i<size;i++){
				concat[i] = unsorted[i];
			}
			MPI_Recv((concat+size),size,MPI_INT,my_rank+1,3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
			//printf("In Even Sort concat\n");
			concat = sort(concat,size*2);
			// Discard the concat to 2 processor
			for(i=0;i<size;i++){
				unsorted[i] = concat[i];
			}
			MPI_Send((concat+size),size,MPI_INT,my_rank+1,4,MPI_COMM_WORLD);
			}
			
		}
	}
	printf("Complete Merge!\n\n");
	// And then receive the total
	if(my_rank!=0){
		if(my_rank!=comm_sz-1){
		MPI_Send(unsorted,size,MPI_INT,0,5,MPI_COMM_WORLD);
		}
		else{
		MPI_Send(unsorted,size,MPI_INT,0,5,MPI_COMM_WORLD);
		}
	}
	else{
		for(i=0;i<size;i++){
			sorted[i] = unsorted[i];
		}
		for(i=1;i<comm_sz;i++){
			MPI_Recv(sorted+size*i,i==comm_sz-1?size+n%comm_sz:size,MPI_INT,i,5,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		}
	}
	
	if(my_rank==0){
		printf("Sorted:\n");
		for(i=0;i<n;i++){
			printf("%d\t",sorted[i]);
		}
		printf("\n");
	}
	// Free the memory
	free(sorted);
	free(unsorted);
	free(rc);
	free(disp);
	free(concat);
	MPI_Finalize();
	return 0;
}

int checkSort(int array[],int size){
	int i;
	for(i=0;i<size-1;i++){
		if(array[i]>array[i+1]){return 1;}
	}
	return 0;
}

int *sort(int array[],int size){
	int *result = (int*)malloc(size*sizeof(int));
	int n = 0,i,j;
	for(i=0;i<size;i++){
		result[i] = 0;
	}
	int smallest = 101,index;
	for(i=0;i<size;i++){
		smallest = 101;
		for(j=0;j<size;j++){
			if(array[j]<smallest){
				smallest = array[j];
				index = j;
			}
		}
		array[index] = 101;
		result[n] = smallest; 
		n++;
	}
	return result;
}
