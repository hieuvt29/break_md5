/* simple send and receive */ 
#include <stdio.h>
#include <mpi.h>
#include <math.h>
int main (int argc, char **argv) {
int myrank,i;
MPI_Status status;
double a[100],b[100];
MPI_Init(&argc, &argv); /* Initialize MPI */
MPI_Comm_rank(MPI_COMM_WORLD, &myrank); /* Get rank */
if( myrank == 0 ){	/* Send a message */
    for (i=0;i<100;++i) a[i]=sqrt(i);
    MPI_Send( a, 101, MPI_DOUBLE, 1, 17, MPI_COMM_WORLD );
}
else if( myrank == 1 ) /* Receive a message */
{
    MPI_Recv( b, 101, MPI_DOUBLE, 0, 17, MPI_COMM_WORLD, &status);
    int cnt;
    MPI_Get_count(&status, MPI_DOUBLE, &cnt);
    printf("status: count = %d, source = %d, tag = %d\n", cnt, status.MPI_SOURCE, status.MPI_TAG);
}
        
MPI_Finalize(); /* Terminate MPI */ 
}