// ParallelProgramming.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "mpi.h"
#include <iostream>
#include <ctime>

using namespace std;


int main(int argc, char *argv[])
{

	MPI_Init(&argc, &argv);
	int worldSize;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int* data;
	int* minsArr;
	int cols = 0;
	int rows= 0;
	double startTime = 0;
	double endTime = 0;
	if (rank == 0) {
		int min = 0;
		int max = 0;
		cout << "Please, enter number of rows ";
		cin >> rows;
		cout << endl << "Please, enter number of columns ";
		cin >> cols;
		cout << endl << "Please, enter min: ";
		cin >> min;
		cout << endl << "Please, enter max: ";
		cin >> max;
		cout << endl;
		startTime = MPI_Wtime();
		srand(time(nullptr));
		int sizeArr = rows*cols;
		cout << endl << sizeArr << endl;
		data = new int[sizeArr];
		for (int i = 0; i < sizeArr; i++) {
			data[i] = min + rand() % (max - min + 1);
		}
		
		for (int j = 0; j < rows; j++) {
			for (int i = 0; i < cols; i++) {
				cout << data[i * rows + j] << " ";
			}
			cout << endl;
		}

		minsArr = new int[cols];
	}
		int size[] = { rows,cols };
		MPI_Bcast(size, 2, MPI_INT, 0, MPI_COMM_WORLD);
		int row = size[0];
		int col = size[1];
		
		int * offset = new int[worldSize];
		int* sendCounts = new int[worldSize];
		for (int i = 0; i < worldSize; i++) {
			sendCounts[i] = (col / worldSize) * row;
		}
		for (int i = 0; i < col % worldSize; i++) {
			sendCounts[i] += row;
		}

		int displ = 0;
		for (int i = 0; i < worldSize; i++) {
			offset[i] = displ;
			displ += sendCounts[i];
		}

		int * recBuf = new int[sendCounts[rank]];

		MPI_Scatterv(data, sendCounts, offset , MPI_INT, recBuf, sendCounts[rank], MPI_INT, 0, MPI_COMM_WORLD);
		
		int * sendBuf = new int[sendCounts[rank] / row];
		int j = 0;
		int r = 1;
		int min = recBuf[0];
		for (int i = 0; i < sendCounts[rank]; i++) {
			if (min > recBuf[i]) min = recBuf[i];
			if (r == row) {
				sendBuf[j] = min;
				j++;
				min = recBuf[i + 1];
				r = 0;
			}
			r++;
		}
		
		int * offset1 = new int[worldSize];
		int d = 0;
		int * recvCounts = new int[worldSize];
		for (int i = 0; i < worldSize; i++) {
			recvCounts[i] = sendCounts[i] / row;
			offset1[i] = d;
			d += recvCounts[i];
		}

		MPI_Gatherv(sendBuf, sendCounts[rank] / row, MPI_INT, minsArr, recvCounts, offset1, MPI_INT, 0, MPI_COMM_WORLD);

		if (rank == 0) {
			cout << endl;
			for (int i = 0; i < cols; i++) {
				cout << "Min Element for " << i << " row is: " << minsArr[i] << endl;
			}
			endTime = MPI_Wtime();
			cout << "Execution time is: " << endTime - startTime << endl;
			delete[] minsArr, data, offset, offset1, sendBuf, sendCounts, recvCounts, recBuf;
		}
	MPI_Finalize();
	
	return 0;
}

