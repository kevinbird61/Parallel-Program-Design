#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>
#define MAX_POP 20

/* Global variable*/
int city_number;
int comm_sz;
int **path_cost;
/* User define function*/
/* Use to allocate a 2-D dimension*/
int **allocate_dimension(int row,int col);
/* Use to make a random generation 2D*/
int **random_population_generation(int my_rank,int row_size , int col_size);
/* Calculate the weight */
void weight_calculation(int **Local_p,int *pop_w);
/* Toss and setting the rate to determine whether it should do or not do */
int toss(int index,float rate);
/* Crossover interface */
void Crossover(int **Local_p,int *pop_weight,float cross_rate);
/* Crossover Implementation */
void Implement_Crossover(int **Local_p , int **temp,int parent1,int parent2, int t_index);
/* Select whether if the suitable parent */
int Select_parent(int *w,int index,int avg);
/* Mutation implement */
void Mutation(int **Local_p,float muta_rate);
/* Switch the population among the population */
void Switch_Select(int my_rank,int **Local_p,int switching_num);

int main(int argc , char *argv[]){
	/* Variable */
	int my_rank;
	double startTime=0.0 , totalTime=0.0;
	int max_generation,switching_num;
	float crossover_rate,mutation_rate;
	int **Local_population;
	int *pop_weight = malloc(MAX_POP*sizeof(int));
	int i,j,k;
	/* Initialize MPI */
	MPI_Init(&argc,&argv);
	/* Set MPI  */
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&comm_sz);
	/* Start time (rank = 0)*/
	if(my_rank == 0){
		startTime = MPI_Wtime();
		/* Read from the file */
	}
	char *filename = argv[1];
	FILE *fp = fopen(filename , "r");
	/* Read from the parameter file */
   	 char *para_file = argv[2];
   	 FILE *fpara = fopen(para_file,"r");
   	 /* Format : max generation , Crossover rate , Mutation rate , Switching number => for every process !*/
   	 fscanf(fpara , "%d %f %f %d" , &max_generation , &crossover_rate , &mutation_rate , &switching_num);
   	 //printf("Max : %d ; cross rate : %f ; mutation rate : %f ; switching_num : %d \n", max_generation , crossover_rate , mutation_rate , switching_num);
	/* Read in the size of matrix => Get "city number" */
	city_number = (filename[strlen(filename)-7] - 48) + 10*(filename[strlen(filename)-8]-48);
    	path_cost = allocate_dimension(city_number,city_number);
    	/* Read in the matrix */
    	for(i=0;i<city_number;i++){
        	for(j=0;j<city_number;j++){
        		fscanf(fp , "%5d" , &path_cost[i][j]);
        	}
   	}
   	// Initialize
   	for(i = 0 ; i < comm_sz ; i++){
   		if(my_rank == i){
	   	 /* Build Population , taking sample from a line of data , number 20~50 sequences (define on the top)*/ 
	   	Local_population = random_population_generation(i,MAX_POP,city_number);
	   	 /* Setting Every  Weight of each sequence of array*/
	   	weight_calculation(Local_population,pop_weight);
		}
   	 }
	// check result => OK ; check every population is different => OK
	// check for every weight of population sequence => OK
   	 if(my_rank == 0){
   	 	for(j=0;j<MAX_POP;j++){
   	 		for(k=0;k<city_number;k++){
   	 			printf("%d " , Local_population[j][k]);
   	 		}
   	 		printf("\n");
   	 	}
   	}
   	int **sendswitching = allocate_dimension(switching_num , city_number);
	int **recvswitching = allocate_dimension(switching_num , city_number);
   	 /* Get the generation count */
   	for(i = 0 ; i < max_generation ; i++){
   		/* Recompute the pop_weight */
   	 	weight_calculation(Local_population,pop_weight);
	   	// Calculate "Crossover"
	   	Crossover(Local_population,pop_weight,crossover_rate);
		// Calculate "Mutation"
		Mutation(Local_population,mutation_rate);
		// Calculate "switching_num" and add to population
		// Select the "MAX_POP" number and make it to next generation population
		// MPI_SEND , MPI_RECV
		Switch_Select(my_rank,Local_population,switching_num);
   	 }
   	 if(my_rank == 0){
   	 	printf("\n");
   	 	for(j=0;j<MAX_POP;j++){
   	 		for(k=0;k<city_number;k++){
   	 			printf("%d " , Local_population[j][k]);
   	 		}
   	 		printf("\n");
   	 	}
   	 }
   	 // Choosing the best result of each process
   	 //if(my_rank == 0){
   	 weight_calculation(Local_population,pop_weight); 
   	 int minimal = 20000 , result;
   	 for(i = 0; i < MAX_POP ; i++){
   	 	if(minimal > pop_weight[i]){
 			minimal = pop_weight[i];
 	 		result = i;
   		}
   	 }
   	printf("My rank %d The Best answer is %d\n",my_rank, minimal);
   	printf("\n");
   	
   	int *compare = malloc(city_number*sizeof(int));
   	 // And choosing the best result among every process's best result , ALL Send to my_rank = 0
   	 if(my_rank != 0){
   	 	MPI_Send(Local_population[result] , city_number , MPI_INT , 0 , my_rank , MPI_COMM_WORLD);
   	 }
   	 if(my_rank == 0){
   	 	for(i = 1;i<comm_sz;i++){
   	 		MPI_Recv(compare,city_number , MPI_INT , i , i , MPI_COMM_WORLD,MPI_STATUS_IGNORE);
   	 		int weight=0;
   	 		for(j = 0; j <city_number-1 ; j++){
   	 			weight += path_cost[compare[j]][compare[j+1]];
   	 		}
   	 		if(minimal > weight){
   	 			minimal = weight;
   	 		}
   	 	}
   	 	printf("The Best answer among process is %d\n",minimal);
   	 }
   	 
   	 /* End time (rank = 0) */
   	 if(my_rank == 0){
   	 	totalTime = MPI_Wtime() - startTime;
   	 	printf("Total Time is %f \n" , totalTime);
   	 }
   	 /* Terminate MPI */
   	MPI_Finalize();
    	return 0;
}

/*
About Major function part:
*/

void Switch_Select(int my_rank,int **Local_p,int switching_num){
	// Allocate " switching_num " array to recv from the parter and send buf to the other partner
	int i,j;
	// Store the selected population to switching , do send first .
	for(i = 0; i < switching_num ; i++){
		for(j = 0; j < city_number ; j++){
			//sendswitching[i][j] = Local_p[i][j];
			Local_p[i][j] = j;
		}
	}
	// Send Implement : Recv : Tag is yourself 
	//MPI_Send(switching , switching_num*city_number, MPI_INT , (my_rank==comm_sz-1? 0 : my_rank+1), my_rank+1 , MPI_COMM_WORLD);
	// Recv Implement : 
	//MPI_Recv(switching , switching_num*city_number , MPI_INT , (my_rank==0? comm_sz-1 : my_rank-1) , my_rank , MPI_COMM_WORLD , MPI_STATUS_IGNORE );
	// Check out recv , compare the "switching num" 's weight
	//MPI_Barrier(MPI_COMM_WORLD);
	//MPI_Sendrecv(sendswitching, switching_num*city_number , MPI_INT ,(my_rank==comm_sz-1? 0 : my_rank+1) , my_rank+1 ,recvswitching, switching_num*city_number , MPI_INT , (my_rank==0? comm_sz-1 : my_rank-1) , my_rank ,MPI_COMM_WORLD , MPI_STATUS_IGNORE);
	
	//printf("In Switching , my_rank is %d\n",my_rank);
	/*for(i = 0; i < switching_num ; i++){
		for(j = 0; j < city_number ; j++){
			printf("%d ",recvswitching[i][j]);
		}
		printf("\n");
	}
	
	free(sendswitching);
	free(recvswitching);*/
}

void Mutation(int **Local_p,float muta_rate){
	// Pass through the MAX_POP , flip if it will mutation or not
	int i,status;
	for(i = 0; i<MAX_POP;i++){
		status = toss(i,muta_rate);
		if(status == 1){
			// muta occur
			int temp = Local_p[i][city_number/3] ;
			Local_p[i][city_number/3] = Local_p[i][city_number/4];
			Local_p[i][city_number/4] = temp;
		}
	}
}

void Crossover(int **Local_p,int *weight,float cross_rate){
	// choosing the center : (city_number / 2) to (3*city_number / 4) to do crossover
	// choosing 2 for a pair , if chosen , store to the temp(next generation) , if not , store those parent into next generation  
	int i,j,k,status,parent1,parent2,average;
	int **temp_pop = allocate_dimension(MAX_POP , city_number);
	int temp_index = 0,now_index=0;
	/*
		When Temp_index = MAX_POP => End
	*/
	/* First , get the average weight */
	for(i=0;i<MAX_POP;i++){
		average += weight[i];
	}
	average = average / MAX_POP;
	
	while(1){
		// Step 1 : Select 2 parent ( from Local_p ) , by pop_weight , more small more better
		/* Second , Select the weight < average 's population*/
		parent1 = Select_parent(weight,now_index,average);
		now_index = parent1+1; 
		parent2 = Select_parent(weight,now_index,average);
		now_index = parent2+1;
		//printf("Average %d ; Parent1 is %d , Parent2 is %d\n",average,parent1,parent2);
		// Step 2 : Decide do or not do crossover
		status = toss(now_index,cross_rate);
		if(status == 1){
			// do crossover and store into temp
			Implement_Crossover(Local_p,temp_pop,parent1,parent2,temp_index);
			temp_index += 2;
		}
		else{
			// directly store the file into temp
			for(i=0;i<city_number;i++){
				temp_pop[temp_index][i] = Local_p[parent1][i];
				temp_pop[temp_index+1][i] = Local_p[parent2][i];
			}
			temp_index += 2;
		}
		// temp_index += 2 , if temp_index > MAX_POP , then break.
		if(temp_index >= MAX_POP){
			break;
		}
	}
	// switch matrix
	for(i = 0 ; i < MAX_POP ; i++){
		for(j = 0 ; j < city_number ; j++){
			Local_p[i][j] = temp_pop[i][j];
		}
	}
	free(temp_pop);
}

/*
About Subfunction - List:
*/
void Implement_Crossover(int **Local_p , int **temp,int parent1,int parent2, int t_index){
	// Select Crossover index
	int i,j,k;
	int start_index = city_number / 2;
	int size = (3*city_number / 4) - ( city_number / 2);
	int find_index = start_index+size;
	int local_index = 0;
	int tmp_index = find_index;
	// Initialize
	for(i=start_index; i < start_index + size ; i++){
		// Do the parent1-  fixed Input 
		temp[t_index][i] = Local_p[parent1][i];
		// Do the parent2 - fixed Input
		temp[t_index+1][i] = Local_p[parent2][i];
	}
	// Find the value not in rest Local_p and store it back to temp
	// Parent1 first
	while(local_index != city_number){
		if(Local_p[parent1][local_index] == Local_p[parent2][find_index]){
			// Store back into temp
			temp[t_index][tmp_index] = Local_p[parent1][local_index];
			//printf("Temp is %d \n",temp[t_index][tmp_index]);
			tmp_index++;
			local_index++;
			find_index++;
			if(local_index == start_index){
				local_index = start_index+size;
			}
			if(tmp_index == city_number){
				tmp_index = 0;
			}
			if(find_index == city_number){
				find_index = 0;
			}
		}
		else{
			// Skip
			find_index++;
			if(find_index == city_number){
				find_index = 0;
			}
		}
	}
	// Parent2
	find_index = start_index+size;
	local_index = 0;
	tmp_index = find_index;
	while(local_index != city_number){
		if(Local_p[parent2][local_index] == Local_p[parent1][find_index]){
			// Store back into temp
			temp[t_index+1][tmp_index] = Local_p[parent2][local_index];
			tmp_index++;
			local_index++;
			find_index++;
			if(local_index == start_index){
				local_index = start_index+size;
			}
			if(tmp_index == city_number){
				tmp_index = 0;
			}
			if(find_index == city_number){
				find_index = 0;
			}
		}
		else{
			// Skip
			find_index++;
			if(find_index == city_number){
				find_index = 0;
			}
		}
	}
}

int Select_parent(int *w,int index,int avg){
	int i = index,result;
	while(1){
		if(i >= MAX_POP){ 
		i = 0;}
		if(w[i] <= avg){
			result = i;
			break;	
		}
		else{
			i++;
		}
	}
	return result;
}

int toss(int index,float rate){
	srand(index+time(NULL));
	int value = rand()%100;
	float result = (float)value / 100;
	//printf("Rate is %f , value is %d ; result is %f\n",rate ,value ,  result);
	if(result < rate){
		// cover in the range
		return 1;
	}
	return 0;
}

void weight_calculation(int **Local_p,int *pop_w){
	// Calculate every line of weight 
	int i , j;
	// Initialize
	for(j = 0 ; j < MAX_POP ; j++){
		pop_w[j] = 0; 
	}
	// Start
	for(i = 0; i < MAX_POP ; i++){
		for(j = 0 ; j < city_number -1 ; j++){
			pop_w[i] += path_cost[ Local_p[i][j] ][  Local_p[i][j+1] ];
		}
	}
}

int **allocate_dimension(int row , int col){
	int i;
	int **a = (int **)malloc(row*sizeof(int*) + (row*col*sizeof(int)));
	int *mem = (int*)(a+row);
	for(i=0;i<row;i++){
		a[i] = mem+(i*col);
	}
	return a;
}

int **random_population_generation(int my_rank , int row_size , int col_size){
	int i,j,filter = 1;
	int **result = allocate_dimension(row_size,col_size);
	int *calculation = malloc(col_size*sizeof(int));
	for(i=0;i<col_size;i++){
		calculation[i] = 0;
	}
	srand(my_rank+time(NULL));
	for(i=0;i<row_size;i++){
		for(j=0;j<col_size;j++){
			while(filter){
				int temp = rand()%col_size;
				if(calculation[temp] == i){
					calculation[temp] ++;
					result[i][j] = temp;
					filter = 0;
				}
			}
			filter = 1;
		}
	}
	return result;	
}
