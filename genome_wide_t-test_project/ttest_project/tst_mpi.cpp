#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "mpi.h"

/*
 Adeniyi CIS 677 HW 12
*/

int malloc2D(int ***array, int n, int m)
{
    int i;
    /* allocate the n*m contiguous items */
    int *p = (int *)malloc(n * m * sizeof(int));
    if (!p)
        return -1;

    /* allocate the row pointers into the memory */
    (*array) = (int **)malloc(n * sizeof(int *));
    if (!(*array))
    {
        free(p);
        return -1;
    }

    /* set up the pointers into the contiguous memory */
    for (i = 0; i < n; i++)
        (*array)[i] = &(p[i * m]);
    return 0;
}

int free2D(int ***array)
{
    /* free the memory - the first element of the array is at the start */
    free(&((*array)[0][0]));

    /* free the pointers into the memory */
    free(*array);

    return 0;
}

int main(int argc, char **argv)
{
    int **global, **local;
    const int gridsize = 4;     // size of grid
    const int procgridsize = 4; // size of process ranks
    int rank, size;             // rank of current process and no. of processes
    int i, j, p, v;
    int displs[procgridsize];
    int elementsperproc = 4;
    int subRows = 1;
    int colSize = 4;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size != procgridsize)
    {
        fprintf(stderr, "%s: Only works with np=%d for now\n", argv[0], procgridsize);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0)
    {
        /* fill in the array, and print it */
        malloc2D(&global, gridsize, colSize);
        int counter = 0;
        for (i = 0; i < gridsize; i++)
        {
            for (j = 0; j < colSize; j++)
                global[i][j] = i;
        }

        printf("Global array is:\n");
        for (i = 0; i < gridsize; i++)
        {
            for (j = 0; j < colSize; j++)
            {
                printf("%2d ", global[i][j]);
            }
            printf("\n");
        }

        for (v = 0; v < procgridsize; v++)
        {
            displs[v] = v * elementsperproc;
            //printf("displs %d \n", displs[v]);
        }
    }

    /* create the local array which we'll process */
    malloc2D(&local, subRows, colSize); //Rows, Columns

    /* create a datatype to describe the subarrays of the global array */
    int sizes[2] = {gridsize, colSize};   /* global size --> Rows, Columns*/
    int subsizes[2] = {subRows, colSize}; /* local size --> Rows, Columns*/
    int starts[2] = {0, 0};               /* where this one starts */
    MPI_Datatype type, subarrtype;
    MPI_Type_create_subarray(2, sizes, subsizes, starts, MPI_ORDER_C, MPI_INT, &type);
    MPI_Type_create_resized(type, 0, sizeof(int), &subarrtype); //Number of Rows
    MPI_Type_commit(&subarrtype);

    int *globalptr = NULL;
    if (rank == 0)
        globalptr = &(global[0][0]);

    /* scatter the array to all processors */
    int sendcounts[procgridsize];
    int subSize = subRows * colSize;

    if (rank == 0)
    {
        for (i = 0; i < procgridsize; i++)
            sendcounts[i] = 1;
    }

    MPI_Scatterv(globalptr, sendcounts, displs, subarrtype, &(local[0][0]),
                 elementsperproc, MPI_INT,
                 0, MPI_COMM_WORLD);

    /* now all processors print their local data: */

    for (p = 0; p < size; p++)
    {
        if (rank == p)
        {
            printf("Local process on rank %d is:\n", rank);
            for (i = 0; i < subRows; i++)
            {
                int sum = 0;
                putchar('|');
                for (j = 0; j < colSize; j++)
                {
                    printf("%2d ", local[i][j]);
                    sum += local[i][j];
                }
                printf("|\n");
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    /* now each processor has its local array, and can process it */
    for (i = 0; i < subRows; i++)
    {
        for (j = 0; j < colSize; j++)
        {
            local[i][j] = j;
        }
    }

    /* it all goes back to process 0 */
    MPI_Gatherv(&(local[0][0]), elementsperproc, MPI_INT,
                globalptr, sendcounts, displs, subarrtype,
                0, MPI_COMM_WORLD);

    /* don't need the local data anymore */
    free2D(&local);

    /* or the MPI data type */
    MPI_Type_free(&subarrtype);

    if (rank == 0)
    {
        printf("Processed grid:\n");
        for (i = 0; i < gridsize; i++)
        {
            for (j = 0; j < colSize; j++)
            {
                printf("%2d ", global[i][j]);
            }
            printf("\n");
        }

        free2D(&global);
    }

    MPI_Finalize();

    return 0;
}