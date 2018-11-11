// Task21PP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "mpi.h"
#include <iostream>
#include <stack>
#include <ctime>

using namespace std;

#define PROXY 0
#define CONSUMER 2
#define PRODUCER 1
#define NEED_APPROVAL -4
#define TERMINATING -5
#define BUFFER_STATUS_FULL -3
#define BUFFER_STATUS_OK -6
#define BUFFER_STATUS_EMPTY -7

struct properties {
	int role;
	int rank;
	int res = 0;
};

class Consumer {
private:
	int resourcesToConsume = 0;
	stack<int> recievedRes;
	properties prop;

private:
	void sendRequest() {
		prop.res = NEED_APPROVAL;
		MPI_Send(&prop, 3, MPI_INT, PROXY, 0, MPI_COMM_WORLD);
	}

	void getResources() {
		int res = 0;
		MPI_Recv(&res, 1, MPI_INT, PROXY, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
		if (res == BUFFER_STATUS_EMPTY) {
			return;
		}
		else {
			resourcesToConsume--;
			recievedRes.push(res);
			if (resourcesToConsume == 0) {
				prop.res = TERMINATING;
				cout << "All resources at process " << prop.rank << " received. Terminating" << endl;
				MPI_Send(&prop, 3, MPI_INT, PROXY, 0, MPI_COMM_WORLD);
				MPI_Finalize();
			}
		}
	}

public:
	Consumer(int rank, int _resourcesToConsume) {
		prop.rank = rank;
		prop.role = CONSUMER;
		resourcesToConsume = _resourcesToConsume;
	}

	void process() {
		while (true) {
			sendRequest();
			getResources();
		}
	}
};

class Producer {
private:
	int resourcesToProduce = 0;
	stack <int> producedRes;
	properties prop;

private:
	void sendRequest() {
		prop.res = NEED_APPROVAL;
		MPI_Send(&prop, 3, MPI_INT, PROXY, 0, MPI_COMM_WORLD);
	}

	void sendResources() {
		int bufferStatus;
		MPI_Recv(&bufferStatus, 1, MPI_INT, PROXY, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
		if (bufferStatus == BUFFER_STATUS_FULL)
			return;
		else if (!producedRes.empty() && bufferStatus == BUFFER_STATUS_OK){
			prop.res = producedRes.top();
			MPI_Send(&prop, 3, MPI_INT, PROXY, 0, MPI_COMM_WORLD);
			producedRes.pop();
			if (producedRes.empty()) {
				cout << "All resources at " << prop.rank << " successfully sent. Terminating" << endl;
				prop.res = TERMINATING;
				MPI_Send(&prop, 3, MPI_INT, PROXY, 0, MPI_COMM_WORLD);
				MPI_Finalize();
			}
		}
	}

public:
	Producer(int _rank, int _resourcesToProduce) {
		resourcesToProduce = _resourcesToProduce;
		prop.rank = _rank;
		prop.role = PRODUCER;
		srand(time(nullptr));
		for (int i = 0; i < resourcesToProduce; i++) {
			producedRes.push(rand() % 20);
		}
	}

	void process() {
		while (true) {
			sendRequest();
			sendResources();
		}
	}
};

class Proxy {
private:
	stack<int>  buffer;
	int bufferSize;
	int processNum;
	properties props;

public:
	Proxy(int _bufferSize, int _processNum) {
		bufferSize = _bufferSize;
		processNum = _processNum;
	}

	void process() {
		while (processNum != 0) {
			MPI_Recv(&props, 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
			
			switch (props.role) {
			case CONSUMER:
				if (!buffer.empty() && props.res == NEED_APPROVAL) {
					int res = buffer.top();
					MPI_Send(&res, 1, MPI_INT, props.rank, 0, MPI_COMM_WORLD);
					cout << "Received resource " << res << " at process " << props.rank << endl;
					buffer.pop();
				}
				else if (props.res == TERMINATING) {
					processNum--;
				}
				else {
					int res = BUFFER_STATUS_EMPTY;
					cout << "Buffer is empty! " << endl;
					MPI_Send(&res, 1, MPI_INT, props.rank, 0, MPI_COMM_WORLD);
				}
				break;
				
			case PRODUCER:
				if (buffer.size() != bufferSize && props.res == NEED_APPROVAL) {
					int res = BUFFER_STATUS_OK;
					MPI_Send(&res, 1, MPI_INT, props.rank, 0, MPI_COMM_WORLD);
					int currRank = props.rank;
					MPI_Recv(&props, 3, MPI_INT, currRank, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
					cout << "Process " << props.rank << " sent resource " << props.res << " to proxy. " << endl;
					buffer.push(props.res);
					bufferSize++;
				}
				else if (props.res == TERMINATING) {
					processNum--;
				}
				else if (buffer.size() == bufferSize && props.res == NEED_APPROVAL) {
					cout << "Buffer is full! " << endl;
					int res = BUFFER_STATUS_FULL;
					MPI_Send(&res, 1, MPI_INT, props.rank, 0, MPI_COMM_WORLD);
				}
				break;
			}
		}
	}
	
};

int main(int argc, char *argv[]){
	MPI_Init(&argc, &argv);
	int prodSize;
	int worldSize;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if (rank == 0) {
		int buffSize;
		/*cout << "Please, enter number of elements to produce: ";
		cin >> prodSize;*/
		cout << "Please, enter size of the buffer: ";
		cin >> buffSize;
		MPI_Bcast(&prodSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
		Proxy proxy(buffSize, worldSize - 1);
		proxy.process();
	}
	/*else if (rank % 2 == 0) {*/
	else if (rank == 1 || rank == 2) {
		MPI_Bcast(&prodSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

		cout << "Created consumer at process " << rank << endl;
		Consumer consumer(rank, 8);
		consumer.process();
	}
	else {
		MPI_Bcast(&prodSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
		cout << "Created producer at process " << rank << endl;
		Producer producer(rank, 4);
		producer.process();
	}

	MPI_Finalize();
    return 0;
}

