#define row1 250
#define col1 250
#define row2 250
#define col2 250
#include <time.h>
#include <stdlib.h>
#include <iostream>
#include "mpi.h"
#include <stdio.h>
#include <math.h>
#include <ctime>
using namespace std;

int** generate_matrix(int row, int col, bool empty){

    int **matrix = new int*[row];
    for (int i = 0; i<row; i++){
        for (int j = 0; j<col; j++){
            matrix[i] = new int [col];
        }
    }
    
    if (empty){
        return matrix;
    }

    for (int i = 0; i<row; i++){
        for (int j = 0; j<col; j++){
            matrix[i][j] = int(rand() % 10 + 1);           
        }
    }
    return matrix;
}

void print_matrix(int** matrix, int row, int col){
    for (int i = 0; i<row; i++){
        for (int j = 0; j<col; j++){
            cout << matrix[i][j] << " ";
        }
        cout << endl;
    }
}

bool serial_mm(int** matrix1, int** matrix2, int** matrix3){
    if (col1 != row2){
        return false;
    }

    for (int i =0; i<row1; i++){
        for (int j=0; j<col2; j++){
            for (int k=0; k<col1; k++){
                matrix3[i][j] += matrix1[i][k] * matrix2[k][j];
            }
        }
    }
    return true;
}

int* transform(int** matrix, int offset, int length){
    int* buff = new int[length];
    for (int i =0; i < length; i++){
        buff[i] = matrix[i][offset];
    }
    return buff;
}

bool parallel_1d_mm(int** matrix1, int** matrix2, int** matrix3){
    int num_process, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &num_process);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Status status;
    if (num_process != row1 + 1 or num_process != col2 + 1){ 
        //cout<< "Num Processor Error" << num_process<< endl;
        return false;
    }
    if (rank==0){
        //print_matrix(matrix1, row1, col1);
        //cout<<endl;
        //print_matrix(matrix2, row2, col2);
        //cout<<endl;

        int offset=0;
        for (int spros = 1; spros < num_process; spros++){
            MPI_Send(&offset, 1, MPI_INT, spros, 1, MPI_COMM_WORLD);
            MPI_Send(matrix1[offset], col1, MPI_INT, spros, 1, MPI_COMM_WORLD);
            int* buff = transform(matrix2, offset, row2);
            MPI_Send(buff, row2, MPI_INT, spros, 1, MPI_COMM_WORLD);
            offset++;
        }
        //cout << "Main Processor Finished Sending" <<endl;

        for (int i =1; i< num_process; i++){
            int done;
            MPI_Recv(&done, 1, MPI_INT, i, 2, MPI_COMM_WORLD, &status);
        }
        //cout <<"Done signal recieved" <<endl;
        for (int i=1; i<num_process;i++){
            int* buffer = new int[row2];
            MPI_Recv(buffer , row2, MPI_INT, i, 4, MPI_COMM_WORLD, &status);
            matrix3[i] = buffer;
        }
        print_matrix(matrix3, row1, col2);
    }

    if (rank > 0){
        int* row = new int[col1];
        int* col = new int[row2];
        int offset; 
        
        MPI_Recv(&offset, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(row, col1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(col, row2, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        for (int i = 1; i<num_process; i++){
            if (i != rank){
                MPI_Send(col, row2, MPI_INT, i, 3, MPI_COMM_WORLD);
            }
        }
        //cout<< "Proc " << rank << " finished sending col"<<endl;

        for (int i =1; i<num_process; i++){

            int* temp_arr = new int[row2];
            if (i==rank){
                for (int i = 0; i< row2; i++){
                    matrix3[rank -1][i] += row[i] * col[i];
                }
            }
            else{
                int* temp_col = new int[row2];
                MPI_Recv(temp_col, row2, MPI_INT, i, 3, MPI_COMM_WORLD, &status);
                //cout << "Proce " << rank << " recieved col" << i << endl;
                for (int j =0; j<row2; j++){
                    temp_arr[i] += row[i] * temp_col[i]; 
                }
                int done = 1;
                MPI_Send(&done, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
                MPI_Send(temp_col, row2, MPI_INT, 0, 4, MPI_COMM_WORLD);
            }
        }
    }

    return true;
}

//bool parallel_2d_mm(int** matrix1, int** matrix2, int** matrix){
//    int num_process, rank;
//    MPI_Comm_size(MPI_COMM_WORLD, &num_process);
//    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//    MPI_Status status;
//
//    int dimx1;
//    int dimy1;
//    int dimx2;
//    int dimy2;
//
//    if (rank==0){
//        int offsetx=0;
//        int offsety = 0;
//        dimx1 = row1/sqrt(num_process);
//        dimy1 = col1/sqrt(num_process);
//        dimx2 = row2/sqrt(num_process);
//        dimy2 = col2/sqrt(num_process);
//        
//        for (int spros = 1; spros < num_process; spros++){
//            int **tempmatrix1 = new int*[dimx];
//            int **tempmatrix2 = new int*[dimx];
//            for (int i = 0; i<dimx1; i++){
//                for (int j = 0; j<dimy1; j++){
//                    tempmatrix1[i] = new int [col];
//                }
//            }
//            for (int i=offsetx*dimx1, i<(offsetx+1)*dimx1, i++){
//                for (int j=offsety*dimy1, i<(offsety+1)*dimy1, i++){
//                    tempmatrix1[i-offsetx*dimx1][j-offsety*dimy1] = matrix1[i][j]
//                }
//            }
//            for (int i = 0; i<dimx2; i++){
//                for (int j = 0; j<dimy2; j++){
//                    tempmatrix2[i] = new int [col];
//                }
//            }
//            for (int i=offsetx*dimx2, i<(offsetx+1)*dimx2, i++){
//                for (int j=offsety*dimy2, i<(offsety+1)*dimy2, i++){
//                    tempmatrix1[i-offsetx*dimx2][j-offsety*dimy2] = matrix2[i][j]
//                }
//            }
//
//            MPI_Send(&offset, 1, MPI_INT, spros, 1, MPI_COMM_WORLD);
//            MPI_Send(tempmatrix1, dimx*dimy, MPI_INT, spros, 1, MPI_COMM_WORLD);
//            MPI_Send(tempmatrix2, dimx*dimy, MPI_INT, spros, 1, MPI_COMM_WORLD);
//            offsetx++;
//            if (offsetx >= dimx1){
//                offsetx=0;
//                offsety++;
//            }
//        }
//
//        for (int i =1; i< num_process; i++){
//            int done;
//            MPI_Recv(&done, 1, MPI_INT, i, 2, MPI_COMM_WORLD, &status);
//        }
//        for (int i=1; i<num_process;i++){
//            int* buffer = new int[row2];
//            MPI_Recv(buffer, row2, MPI_INT, i, 4, MPI_COMM_WORLD, &status);
//            matrix3[i] = buffer;
//        }
//        print_matrix(matrix3, row1, col2);
//    }
//
//    if (rank > 0){
//        int** tempmatrix1 = new int[dimx1][dimy1];
//        int** tempmatrix2 = new int[dimx2][dimy2];
//        int offset; 
//        
//        MPI_Recv(&offset, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
//        MPI_Recv(tempmatrix1, dimx1*dimy1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
//        MPI_Recv(tempmatrix2, dimx2*dimy2, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
//
//        for (int i = 1; i<num_process; i++){
//            if (i != rank){
//                MPI_Send(col, row2, MPI_INT, i, 3, MPI_COMM_WORLD);
//            }
//        }
//
//        for (int i =1; i<num_process; i++){
//
//            int* temp_arr = new int[row2];
//            if (i==rank){
//                for (int i = 0; i< row2; i++){
//                    matrix3[rank -1][i] += row[i] * col[i];
//                }
//            }
//            else{
//                int* temp_col = new int[row2];
//                MPI_Recv(temp_col, row2, MPI_INT, i, 3, MPI_COMM_WORLD, &status);
//                for (int j =0; j<row2; j++){
//                    temp_arr[i] += row[i] * temp_col[i]; 
//                }
//                int done = 1;
//                MPI_Send(&done, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
//                MPI_Send(temp_col, row2, MPI_INT, 0, 4, MPI_COMM_WORLD);
//            }
//        }
//    }
//
//    return true;
//}

int main(){
    srand(time(NULL));

    int** matrix1 = generate_matrix(row1, col1, false);
    int** matrix2 = generate_matrix(row2, col2, false);
    int** matrix3 = generate_matrix(row1, col2, true);
    int** matrix4 = generate_matrix(row1, col2, true);
    int** matrix5 = generate_matrix(row1, col2, true);

    clock_t start;
    start = clock();
    
    serial_mm(matrix1, matrix2, matrix3);
    print_matrix(matrix3, row1, col2);
    cout<<endl;

    double elapsed_seconds = (clock() - start )/ (double) CLOCKS_PER_SEC; 
    cout << "Elapsed time: " << elapsed_seconds << " s"<<endl;

    double t1, t2;
    MPI_Init(0,0);
    t1 = MPI_Wtime();
    parallel_1d_mm(matrix1, matrix2, matrix4);
    t2 = MPI_Wtime();
    printf("Elapsed time is %f\n", t2-t1);
    MPI_Finalize();

    /**
    MPI_Init(0,0);
    t1 = MPI_Wtime();
    parallel_2d_mm(matrix1, matrix2, matrix5);
    t2 = MPI_Wtime();
    MPI_Finalize();
    **/

}


