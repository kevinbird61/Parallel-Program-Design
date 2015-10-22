/* circuitSatifiability.c solves the Circuit Satisfiability
 *  Problem using a brute-force sequential solution.
 *
 *   The particular circuit being tested is "wired" into the
 *   logic of function 'checkCircuit'. All combinations of
 *   inputs that satisfy the circuit are printed.
 *
 *   16-bit version by Michael J. Quinn, Sept 2002.
 *   Extended to 32 bits by Joel C. Adams, Sept 2013.
 */

#include <stdio.h>     // printf()
#include <limits.h>    // UINT_MAX
#include <mpi.h>

#define TEST_MAX 100000000

int checkCircuit (int, long);

int main (int argc, char *argv[]) {
   long i;               /* loop variable (64 bits) */
   int id = 0,k=0;           /* process id */
   int count = 0 ,count_e = 0, total = 0;        /* number of solutions */
	   int my_rank,comm_sz,procNum,start,end;
	   char counting[100],timeing[100];
	   //FILE *fp = fopen("analysis_hw1.txt","w");
	   //fprintf(fp,"Process:		Consume Time:\n");
	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);	
	MPI_Comm_size(MPI_COMM_WORLD,&comm_sz);
	//printf("The rank is : %d\n",my_rank);
	//printf("The communate size is :%d\n\n",comm_sz);
	double startTime = 0.0 , totalTime = 0.0;
	if(comm_sz>1){
	   start=0+(UINT_MAX/comm_sz)*my_rank;
	   end=start+UINT_MAX/comm_sz;
	   startTime = MPI_Wtime();
	   for (i = start; i <= end; i++) {
		  count += checkCircuit (my_rank, i);
	   }
	   totalTime = MPI_Wtime() - startTime;
   }
   else if(comm_sz==1){
	   startTime = MPI_Wtime();
	   for (i = 0; i <= UINT_MAX; i++) {
		  count += checkCircuit (my_rank, i);
	   }
	   totalTime = MPI_Wtime() - startTime;
   }
   procNum = comm_sz;
   sprintf(timeing,"process %d finished in time %f sec\n",my_rank,totalTime);
	//fprintf(fp,"%d		%f\n",my_rank,totalTime);
   sprintf(counting,"process %d finished in count %d \n",my_rank,count);
   	//fprintf(fp,"%d		%d\n",my_rank,count);
   // Orginal use UNIT_MAX 
   while(procNum>1){
        if(my_rank>=(procNum/2) && my_rank<procNum){
            MPI_Send(timeing,strlen(timeing)+1,MPI_CHAR,0,1,MPI_COMM_WORLD);
            MPI_Send(counting,strlen(counting)+1,MPI_CHAR,0,2,MPI_COMM_WORLD);
            MPI_Send(&count,1,MPI_INT,my_rank-(procNum/2),3,MPI_COMM_WORLD);
        }
        else if(my_rank<(procNum/2) && my_rank<procNum){
                count_e=count;
                MPI_Recv(&count,1,MPI_INT,my_rank+(procNum/2),3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                count=count_e+count;
        }
        procNum=procNum/2;
   }
   if(my_rank==0&&comm_sz!=1){
        printf("\nprocess %d finished in time %f sec\n\n",my_rank,totalTime);
        //fprintf(fp,"%d		%f\n",my_rank,totalTime);
        int q;
            for(q=1;q<comm_sz;q++){
                MPI_Recv(counting,1000,MPI_CHAR,q,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                printf("%s\n",counting);
                MPI_Recv(timeing,1000,MPI_CHAR,q,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                printf("%s\n",timeing);
                fflush (stdout);
            }
        printf("\n");
        printf("total count %d\n",count);
        //fprintf(fp,"total count %d\n",count);
   }
   else if(my_rank==0&&comm_sz==1){
   		printf("\nprocess %d finished in time %f sec\n\n",my_rank,totalTime);
   		printf("%s\n",counting);
   		printf("%s\n",timeing);
   		printf("\n");
        printf("total count %d\n",count);
   }
   
   MPI_Finalize();
   
   return 0;
}

/* EXTRACT_BIT is a macro that extracts the ith bit of number n.
 *
 * parameters: n, a number;
 *             i, the position of the bit we want to know.
 *
 * return: 1 if 'i'th bit of 'n' is 1; 0 otherwise 
 */

#define EXTRACT_BIT(n,i) ( (n & (1<<i) ) ? 1 : 0)


/* checkCircuit() checks the circuit for a given input.
 * parameters: id, the id of the process checking;
 *             bits, the (long) rep. of the input being checked.
 *
 * output: the binary rep. of bits if the circuit outputs 1
 * return: 1 if the circuit outputs 1; 0 otherwise.
 */

#define SIZE 32

int checkCircuit (int id, long bits) {
   int v[SIZE];        /* Each element is a bit of bits */
   int i;

   for (i = 0; i < SIZE; i++)
     v[i] = EXTRACT_BIT(bits,i);

   if ( ( (v[0] || v[1]) && (!v[1] || !v[3]) && (v[2] || v[3])
       && (!v[3] || !v[4]) && (v[4] || !v[5])
       && (v[5] || !v[6]) && (v[5] || v[6])
       && (v[6] || !v[15]) && (v[7] || !v[8])
       && (!v[7] || !v[13]) && (v[8] || v[9])
       && (v[8] || !v[9]) && (!v[9] || !v[10])
       && (v[9] || v[11]) && (v[10] || v[11])
       && (v[12] || v[13]) && (v[13] || !v[14])
       && (v[14] || v[15]) )
       ||
          ( (v[16] || v[17]) && (!v[17] || !v[19]) && (v[18] || v[19])
       && (!v[19] || !v[20]) && (v[20] || !v[21])
       && (v[21] || !v[22]) && (v[21] || v[22])
       && (v[22] || !v[31]) && (v[23] || !v[24])
       && (!v[23] || !v[29]) && (v[24] || v[25])
       && (v[24] || !v[25]) && (!v[25] || !v[26])
       && (v[25] || v[27]) && (v[26] || v[27])
       && (v[28] || v[29]) && (v[29] || !v[30])
       && (v[30] || v[31]) ) )
   {
     /* printf ("%d) %d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d \n", id,
         v[31],v[30],v[29],v[28],v[27],v[26],v[25],v[24],v[23],v[22],
         v[21],v[20],v[19],v[18],v[17],v[16],v[15],v[14],v[13],v[12],
         v[11],v[10],v[9],v[8],v[7],v[6],v[5],v[4],v[3],v[2],v[1],v[0]);*/
      fflush (stdout);
      return 1;
   } else {
      return 0;
   }
}

