// ParallelProgramming.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "mpi.h"
#include <iostream>
#include <ctime>

using namespace std;


int main(int argc, char *argv [])
{
	
	MPI_Init(&argc,&argv);
	int worldSize;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if (rank == 0) {
		int col = 0;
		int row = 0;
		cout << "Please, enter number of rows ";
		cin >> row;
		cout << endl << "Please, enter number of columns ";
		cin >> col;
		cout << endl;
		int size[] = {row,col};
		MPI_Bcast(size, 2, MPI_INT, 0, MPI_COMM_WORLD);
		srand(time(nullptr));
		int ** data = new int *[row];
		for (int i = 0; i < row; i++) {
			data[i] = new int[col];
			for (int j = 0; j < col; j++) {
				data[i][j] = rand() % 100;
				cout << data[i][j] << " ";
			}
			cout << endl;
		}
		

		if (worldSize != 1) {
			int min = data[0][0];
			cout << min << " ";
			for (int j = 1; j < row; j++) {
				cout << data[j][0] << " ";
				if (data[j][0] < min) min = data[j][0];
			}
			cout << " min element is " << min << endl;
			
			for (int k = 0; k < col / (worldSize - 1); k++) {
				int thread = 1;
				for (int i = 1; i < worldSize; i++) {
					int* request = new int[row];
					for (int j = 0; j < row; j++) {
						request[j] = data[j][i + (worldSize - 1) * k];
					}
					MPI_Send(request, row, MPI_INT, thread, 0, MPI_COMM_WORLD);
					thread++;
				}
			}
			int s = 1;
			for (int i = (col / (worldSize - 1)) * (worldSize - 1); i < col; i++) {
				int* request = new int[row];
				for (int j = 0; j < row; j++) {
					request[j] = data[j][i];
				}
				MPI_Send(request, row, MPI_INT, s, 0, MPI_COMM_WORLD);
				s++;
			}
			for (int i = 1; i < col; i++) {
				int *resp1 = new int[row + 1];
				MPI_Recv(resp1, row + 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
				for (int j = 0; j < row; j++) {
					cout << resp1[j] << " ";
				}
				cout << "  min element is " << resp1[row] << endl;
			}
		}

	}
	else {
		int size [2];
		MPI_Bcast(size, 2, MPI_INT, 0, MPI_COMM_WORLD);
		int row = size[0];
		int col = size[1];
		int* resp = new int[row];
		for (int i = 0; i < col / (worldSize - 1); i++) {
			MPI_Recv(resp, row, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
			int minElem = resp[0];
			int * answer = new int[row + 1];
			answer[0] = minElem;
			for (int j = 1; j < row; j++) {
				answer[j] = resp[j];
				if (minElem > resp[j])
					minElem = resp[j];
			}
			answer[row] = minElem;
			MPI_Send(answer, row + 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		}
		if (rank <= col % (worldSize - 1)) {
			int * resp2 = new int[row];
			MPI_Recv(resp2, row, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
			int minElem = resp[0];
			int * resp3 = new int[row + 1];
			resp3[0] = minElem;
			for (int j = 1; j < row; j++) {
				resp3[j] = resp[j];
				if (minElem > resp[j])
					minElem = resp[j];
			}
			resp3[row] = minElem;			
			MPI_Send(resp3, row + 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		}
	}
	MPI_Finalize();
	return 0;
}

