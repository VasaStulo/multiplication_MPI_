#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#include <Windows.h>
#include "mpi.h"
#include <clocale>

//Функция простой инициализации матриц
void matrixInitialize(double* pAMatrix, double* pBMatrix, int N) {
	int i, j;
	for(i=0; i<N; i++)
		for (j = 0; j < N; j++) {
			pAMatrix[i * N + j] = 1;
			pBMatrix[i * N + j] = 1;
		}
}


//Функция рандома инициализации матриц
void randomInitialize(double* pAMatrix, double* pBMatrix, int N) {
	int i, j;
	srand(unsigned(clock()));
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++) {
			pAMatrix[i * N + j] = rand()/double(1000);
			pBMatrix[i * N + j] = rand()/double(1000);
		}
}
//Память, память
void processInitialize(double* &pAMatrix, double* &pBMatrix, double* &pCMatrix, double* &pCMatrixSum, int &N) {
	do {
		printf("Введи размер матрицы -> ");
		fflush(stdout);
		scanf_s("%d", &N);
	} while (N <= 0);

	pAMatrix = new double[N * N];
	pBMatrix = new double[N * N];
	pCMatrix = new double[N * N];
	pCMatrixSum = new double[N * N];

	randomInitialize(pAMatrix, pBMatrix, N);
	for (int i = 0; i < N * N; i++) {
		pCMatrix[i] = 0;
		pCMatrixSum[i] = 0;
	}
}

//вывод матриц
void printMatrix(double* pMatrix, int rowCount, int colCount) {
	int i, j;
	for (i = 0; i < rowCount; i++) {
		for (j = 0; j < colCount; j++)
			printf("%7.4f ", pMatrix[i * rowCount + j]);
		printf("\n");
	}
}

// умножение матриц
void serialResultCalc(double* pAMatrix, double* pBMatrix, double* pCMatrix, int N, int nCommRunk, int nCommSize) {
	int i, j, k;
	for (i = 0; i < N; i++) {
		for (j = 0 + nCommRunk; j < N; j += nCommSize) {
			for (k = 0; k < N; k++) {
				pCMatrix[i * N + j] += pAMatrix[i * N + k] * pBMatrix[k * N + j];
			}
		}
	}
}

//чистим матрицы__ завершение
void processTerm(double* pAMatrix, double* pBMatrix, double* pCMatrix, double* pCMatrixSum) {
	delete[] pAMatrix;
	delete[] pBMatrix;
	delete[] pCMatrix;
	delete[] pCMatrixSum;
}

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "Russian");

	double* pAMatrix;
	double* pBMatrix;
	double* pCMatrix;
	double* pCMatrixSum = 0;
	int N;
	int nCommRunk, nCommSize, namelen, nCounter;
	int nIntervals;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	double t1, t2;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &nCommRunk);
	MPI_Comm_size(MPI_COMM_WORLD, &nCommSize);
	MPI_Get_processor_name(processor_name, &namelen);
	if (nCommRunk == 0) {
		t1 = MPI_Wtime();
		printf("Параллельное умножение матриц\n ");
		processInitialize(pAMatrix, pBMatrix, pCMatrix, pCMatrixSum, N);
		printf("Инициализация матрицы А\n");
		printMatrix(pAMatrix, N, N);
		printf("Инициализация матрицы B\n");
		printMatrix(pBMatrix, N, N);

		for (nCounter = 1; nCounter < nCommSize; nCounter++) {
			MPI_Send(&N, 1, MPI_INT, nCounter, 0, MPI_COMM_WORLD);
			MPI_Send(pAMatrix, N * N, MPI_DOUBLE, nCounter, 1, MPI_COMM_WORLD);
			MPI_Send(pBMatrix, N * N, MPI_DOUBLE, nCounter, 2, MPI_COMM_WORLD);
			MPI_Send(pCMatrix, N * N, MPI_DOUBLE, nCounter, 3, MPI_COMM_WORLD);
		}
	}
	else{
		MPI_Recv(&N, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		pAMatrix = new double[N * N];
		pBMatrix = new double[N * N];
		pCMatrix = new double[N * N];
		MPI_Recv(pAMatrix, N * N, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(pBMatrix, N * N, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(pCMatrix, N * N, MPI_DOUBLE, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	///перемножение
	serialResultCalc(pAMatrix, pBMatrix, pCMatrix, N, nCommRunk, nCommSize);
	MPI_Reduce(pCMatrix, pCMatrixSum, N * N, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	printf("Process %d of %d running on %s\n", nCommRunk, nCommSize, processor_name);
	if (nCommRunk == 0) {
		printf("\nРезельтирующая сумма матриц: \n");
		printMatrix(pCMatrixSum, N, N);
		processTerm(pAMatrix, pBMatrix, pCMatrix, pCMatrixSum);
		t2 = MPI_Wtime();
		printf("Time is %f sec \n", t2 - t1);
	}
	MPI_Finalize();
}
