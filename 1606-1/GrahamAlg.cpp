#include "stdafx.h"
#include <iostream>
#include "mpi.h"
#include <vector>
#include <ctime>


using namespace std;

struct pt
{
	int x; 
	int y;
};

class Point
{
private:
	int x;
	int y;

public:

	int getY()
	{
		return y;
	}

	void setY(int y)
	{
		this->y = y;
	}

	int getX()
	{
		return x;
	}

	void setX(int x) 
	{
		this->x = x;
	}

	Point(int x, int y)
	{
		this->setX(x);
		this->setY(y);
	}

	Point()
	{

	}

	static bool moreOrEqualThan(Point a, Point c, Point b)
	{
		return a.getX() * b.getY() - a.getY() * b.getX() + b.getX() * c.getY() - b.getY() * c.getX() + c.getX() * a.getY() - c.getY() * a.getX() <= 0;
	}

	static bool notEquals(Point a, Point b)
	{
		if (a.getX() != b.getX())
			return true;
		else if (a.getY() != b.getY())
			return true;
		else return false;
	}

	static int determinant(Point a, Point b, Point c)
	{
		return (b.getX() - a.getX()) * (c.getY() - a.getY()) - (c.getX() - a.getX())*(b.getY() - a.getY());
	}
};

int partition(vector<Point> &a, int low, int high)
{
	Point pivot = a[high];
	int i = (low - 1);
	for (int j = low; j < high; j++)
	{
		if (Point::moreOrEqualThan(pivot, a[j], a[0]))
		{
			i++;
			Point tmp = a[i];
			a[i] = a[j];
			a[j] = tmp;
		}
	}
	Point tmp = a[i + 1];
	a[i + 1] = a[high];
	a[high] = tmp;

	return i + 1;
}

void quickSort(vector<Point> &a, int low, int high)
{
	if (low < high)
	{
		int partIndex = partition(a, low, high);
		quickSort(a, low, partIndex - 1);
		quickSort(a, partIndex + 1, high);

	}
}

int grahamScan(vector<Point>& a)
{
	Point c = a[0];
	int m = 0;
	for (int i = 1; i < a.size(); i++)
	{
		if (a[i].getX() < c.getX())
		{
			c = a[i];
			m = i;
		}
		else if (a[i].getX() == c.getX())
		{
			if (a[i].getY() < c.getY())
			{
				c = a[i];
				m = i;
			}
		}
	}

	Point tmp = a[0];
	a[0] = a[m];
	a[m] = tmp;
	m = 1;
	quickSort(a, 0, a.size() - 1);

	for (int i = 1; i < a.size(); i++)
	{
		if (Point::notEquals(a[i], a[m]))
		{
			if (m >= 1)
			{
				while (m >= 1 && Point::determinant(a[m - 1], a[m], a[i]) >= 0)
					m = m - 1;
			}
			m = m + 1;
			Point tmp = a[m];
			a[m] = a[i];
			a[i] = tmp;
		}
	}
	return m + 1;
}


int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);
	int worldSize;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	int count = 2;
	int arrOfBlocklengths[] = { 1, 1 };
	MPI_Aint arrOfDisps[] = { offsetof(pt, x), offsetof(pt, y) };
	MPI_Datatype arrOfTypes[] = { MPI_INT, MPI_INT };
	MPI_Datatype tmpType, pointType;
	MPI_Aint lb, extent;

	MPI_Type_create_struct(count, arrOfBlocklengths, arrOfDisps, arrOfTypes, &tmpType);
	MPI_Type_get_extent(tmpType, &lb, &extent);
	MPI_Type_create_resized(tmpType, lb, extent, &pointType);
	MPI_Type_commit(&pointType);

	if (rank == 0)
	{
		int numOfPoints = 0;
		int min = 0;
		int max = 0;
		cout << "Please, enter number of points: ";
		cin >> numOfPoints;
		cout << "Please, enter min value: ";
		cin >> min;
		cout << "Please, enter max value: ";
		cin >> max;
		srand(time(nullptr));
		vector<pt> data;
		for (int i = 0; i < numOfPoints; i++)
		{
			pt p;
			p.x = min + rand() % (max - min + 1);
			p.y = min + rand() % (max - min + 1);
			data.push_back(p);
			cout << "( " << data[i].x << ", " << data[i].y << ") ";
		}
		cout << endl;
		MPI_Bcast(&numOfPoints, 1, MPI_INT, 0, MPI_COMM_WORLD);
		for (int i = 1; i < worldSize; i++)
		{
			pt* pointsToSend = new pt [data.size() / (worldSize - 1)];
			//cout << "Sending points to " << i << " thread: ";
			for (int j = 0; j < data.size() / (worldSize - 1); j++)
			{
				pt p = data[(i - 1) *data.size()/(worldSize - 1) + j];
				pointsToSend[j] = p;
				//cout << "( " << p.x << ", " << p.y << ") ";
			}
			//cout << endl;
			
			MPI_Send(&pointsToSend[0], data.size() / (worldSize - 1), pointType, i, 0, MPI_COMM_WORLD);
		}

		cout << "left points: ";
		vector<Point> leftPoints;

		for (int i = (worldSize - 1) * numOfPoints / (worldSize - 1); i < numOfPoints; i++)
		{
			Point p(data[i].x, data[i].y);
			leftPoints.push_back(p);
			cout << "( " << p.getX() << ", " << p.getY() << ") ";
		}

		vector<Point> receivedHulls;

		int newSize = data.size() / (worldSize - 1) + 1;
		pt* pointsToReceive = new pt[newSize];

		for (int i = 0; i < worldSize - 1; i++)
		{
			MPI_Recv(&pointsToReceive[0], newSize, pointType, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
			int sizeOfHull = pointsToReceive[0].x;
			for (int j = 1; j < sizeOfHull + 1; j++)
			{
				Point receivedPoint(pointsToReceive[j].x, pointsToReceive[j].y);
				receivedHulls.push_back(receivedPoint);
			}
		}

		int hullSize = grahamScan(receivedHulls);
		receivedHulls.resize(hullSize);
		cout << endl << "Convex hull made by Graham scan: ";
		for (int i = 0; i < hullSize; i++)
		{
			cout << "( " << receivedHulls[i].getX() << ", " << receivedHulls[i].getY() << ") ";
		}
	}

	else
	{
		int size = 0;
		MPI_Bcast(&size, 1, MPI_INT, 0, MPI_COMM_WORLD);
		size = size / (worldSize - 1);
		pt* pts = new pt [size];
		MPI_Recv(&pts[0], size, pointType, 0, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
		vector<Point> points;
		//cout << endl << "got points at " << rank << " ";
		for (int i = 0; i < size; i++)
		{
			//cout << "( " << pts[i].x << ", " << pts[i].y << ") ";
			Point point(pts[i].x, pts[i].y);
			points.push_back(point);
			
		}
		int newSize = grahamScan(points);
		cout << endl << "calculated hull: " << newSize << " ";
		pts = new pt[size + 1];
		pts[0].x = newSize;
		pts[0].y = 0;
		for (int i = 1; i < size + 1; i++)
		{
			pt p = { points[i - 1].getX(), points[i - 1].getY() };
			pts[i] = p;
			cout << "( " << pts[i].x << ", " << pts[i].y << ") ";
		}
		cout << endl;
		MPI_Send(&pts[0], size + 1, pointType, 0, 0, MPI_COMM_WORLD);
	}

	MPI_Finalize();

}
