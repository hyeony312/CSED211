// 20220778 표승현
/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, l;
    int v0, v1, v2, v3, v4, v5, v6, v7;
    v0 = 0;
    v1=0;
    // 32x32 matrix
    if(M==32&&N==32){
        for(i=0;i<M;i += 8){
            for(j=0;j<N;j += 8){
                for(k=0;k<8;k++){
                    for(l=0;l<8;l++){
                        if(j+l==i+k) { 
                            v0 = A[i+k][i+k]; // v0: 대각성분 값
                            v2 = i+k; // v2: 대각성분 좌표
                            v1=1;
                            continue;
                        };
                        B[j+l][i+k]=A[i+k][j+l];
                    }

                    if(v1==1){
                        v1=0;
                        B[v2][v2] = v0;
                    }
                }
            }
        }
    }
    else if (M==64 && N==64)
    {
        i=0;
        j=0;
        while (i < 64)
        {
        j=0;
            while(j < 64)
            {
                for (k = 0; k < 4; k+=1)
                {
			        v0 = A[i+k][j+0];
                    v1 = A[i+k][j+1];
                    v2 = A[i+k][j+2];
                    v3 = A[i+k][j+3];
                    v4 = A[i+k][j+4];
                    v5 = A[i+k][j+5];
                    v6 = A[i+k][j+6];
                    v7 = A[i+k][j+7];

                    B[j+0][i+k] = v0;
                    B[j+1][i+k] = v1;
                    B[j+2][i+k] = v2;
                    B[j+3][i+k] = v3;
			        B[j+3][i+k+4] = v4;
			        B[j+2][i+k+4] = v5;
			        B[j+1][i+k+4] = v6;   
                    B[j+0][i+k+4] = v7;
		        } 
		        for (k = 0; k < 4; k+=1)
                {
                    v0 = A[i+4][j-k+3];
                    v1 = A[i+5][j-k+3];
                    v2 = A[i+6][j-k+3];
                    v3 = A[i+7][j-k+3];
                    v4 = A[i+4][j+k+4];
                    v5 = A[i+5][j+k+4];
                    v6 = A[i+6][j+k+4];
                    v7 = A[i+7][j+k+4];
                    for(l=0;l<4;l++){
                        B[j+k+4][i+l] = B[j-k+3][i+4+l];
                    }
                    B[j-k+3][i+4]=v0;
                    B[j-k+3][i+5]=v1;
                    B[j-k+3][i+6]=v2;
                    B[j-k+3][i+7]=v3;
                    B[j+k+4][i+4]=v4;
                    B[j+k+4][i+5]=v5;
                    B[j+k+4][i+6]=v6;
                    B[j+k+4][i+7]=v7;
                }
            j=j+8;
	        }
        i=i+8;
        }
    }
    else if(M==61 && N==67){ // 61x67 matrix
        for(i=0;i<N;i += 16){
            for(j=0;j<M;j += 16){
                for(k=0;k+i<N&&k<16;k++){
                    for(l=0;l+j<M&&l<16;l++){
                    if(j+l==i+k) { 
                            v0 = A[i+k][i+k]; // v0: 대각성분 값
                            v2 = i+k; // v2: 대각성분 좌표
                            v1=1;
                            continue;
                    };
                        B[j+l][i+k]=A[i+k][j+l];
                    }

                    if(v1==1){
                        v1=0;
                        B[v2][v2] = v0;
                    }
                }
            }
        }
    } 
    
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

