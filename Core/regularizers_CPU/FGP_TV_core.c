/*
This work is part of the Core Imaging Library developed by
Visual Analytics and Imaging System Group of the Science Technology
Facilities Council, STFC

Copyright 2017 Daniil Kazantsev
Copyright 2017 Srikanth Nagella, Edoardo Pasca

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "FGP_TV_core.h"

/* C-OMP implementation of FGP-TV [1] denoising/regularization model (2D/3D case)
 *
 * Input Parameters:
 * 1. Noisy image/volume 
 * 2. lambda - regularization parameter 
 * 3. Number of iterations
 * 4. eplsilon: tolerance constant 
 * 5. TV-type: methodTV - 'iso' (0) or 'l1' (1)
 * 6. nonneg: 'nonnegativity (0 is OFF by default) 
 * 7. print information: 0 (off) or 1 (on) 
 *
 * Output:
 * [1] Filtered/regularized image
 *
 * This function is based on the Matlab's code and paper by
 * [1] Amir Beck and Marc Teboulle, "Fast Gradient-Based Algorithms for Constrained Total Variation Image Denoising and Deblurring Problems"
 */
 
float TV_FGP_CPU(float *Input, float *Output, float lambda, int iter, float epsil, int methodTV, int nonneg, int printM, int dimX, int dimY, int dimZ)
{
	int ll, j, DimTotal;
	float re, re1;
	float tk = 1.0f;
    float tkp1=1.0f;
    int count = 0;
	
	if (dimZ <= 1) {
		/*2D case */				
		float *Output_prev=NULL, *P1=NULL, *P2=NULL, *P1_prev=NULL, *P2_prev=NULL, *R1=NULL, *R2=NULL;		
		DimTotal = dimX*dimY;
		
		Output_prev = (float *) malloc( DimTotal * sizeof(float) );
		P1 = (float *) malloc( DimTotal * sizeof(float) );
		P2 = (float *) malloc( DimTotal * sizeof(float) );
		P1_prev = (float *) malloc( DimTotal * sizeof(float) );
		P2_prev = (float *) malloc( DimTotal * sizeof(float) );
		R1 = (float *) malloc( DimTotal * sizeof(float) );
		R2 = (float *) malloc( DimTotal * sizeof(float) );
		
		/* begin iterations */
        for(ll=0; ll<iter; ll++) {
            
            /* computing the gradient of the objective function */
            Obj_func2D(Input, Output, R1, R2, lambda, dimX, dimY);
            
            /* apply nonnegativity */
            if (nonneg == 1) for(j=0; j<dimX*dimY; j++) {if (Output[j] < 0.0f) Output[j] = 0.0f;}            
            
            /*Taking a step towards minus of the gradient*/
            Grad_func2D(P1, P2, Output, R1, R2, lambda, dimX, dimY);
            
            /* projection step */
            Proj_func2D(P1, P2, methodTV, dimX, dimY);
            
            /*updating R and t*/
            tkp1 = (1.0f + sqrt(1.0f + 4.0f*tk*tk))*0.5f;
            Rupd_func2D(P1, P1_prev, P2, P2_prev, R1, R2, tkp1, tk, dimX, dimY);
            
            /* check early stopping criteria */
            re = 0.0f; re1 = 0.0f;
            for(j=0; j<dimX*dimY; j++)
            {
                re += pow(Output[j] - Output_prev[j],2);
                re1 += pow(Output[j],2);
            }
            re = sqrt(re)/sqrt(re1);
            if (re < epsil)  count++;
				if (count > 4) break;				            
            
            /*storing old values*/
            copyIm(Output, Output_prev, dimX, dimY, 1);
            copyIm(P1, P1_prev, dimX, dimY, 1);
            copyIm(P2, P2_prev, dimX, dimY, 1);
            tk = tkp1;
        }
        if (printM == 1) printf("FGP-TV iterations stopped at iteration %i \n", ll);   
		free(Output_prev); free(P1); free(P2); free(P1_prev); free(P2_prev); free(R1); free(R2);		
	}
	else {
		/*3D case*/
		float *Output_prev=NULL, *P1=NULL, *P2=NULL, *P3=NULL, *P1_prev=NULL, *P2_prev=NULL, *P3_prev=NULL, *R1=NULL, *R2=NULL, *R3=NULL;		
		DimTotal = dimX*dimY*dimZ;
		
		Output_prev = (float *) malloc( DimTotal * sizeof(float) );
		P1 = (float *) malloc( DimTotal * sizeof(float) );
		P2 = (float *) malloc( DimTotal * sizeof(float) );
		P3 = (float *) malloc( DimTotal * sizeof(float) );
		P1_prev = (float *) malloc( DimTotal * sizeof(float) );
		P2_prev = (float *) malloc( DimTotal * sizeof(float) );
		P3_prev = (float *) malloc( DimTotal * sizeof(float) );
		R1 = (float *) malloc( DimTotal * sizeof(float) );
		R2 = (float *) malloc( DimTotal * sizeof(float) );
		R3 = (float *) malloc( DimTotal * sizeof(float) );
		
		    /* begin iterations */
        for(ll=0; ll<iter; ll++) {
            
            /* computing the gradient of the objective function */
            Obj_func3D(Input, Output, R1, R2, R3, lambda, dimX, dimY, dimZ);
            
            /* apply nonnegativity */
            if (nonneg == 1) for(j=0; j<dimX*dimY*dimZ; j++) {if (Output[j] < 0.0f) Output[j] = 0.0f;}  
            
            /*Taking a step towards minus of the gradient*/
            Grad_func3D(P1, P2, P3, Output, R1, R2, R3, lambda, dimX, dimY, dimZ);
            
            /* projection step */
            Proj_func3D(P1, P2, P3, methodTV, dimX, dimY, dimZ);
            
            /*updating R and t*/
            tkp1 = (1.0f + sqrt(1.0f + 4.0f*tk*tk))*0.5f;
            Rupd_func3D(P1, P1_prev, P2, P2_prev, P3, P3_prev, R1, R2, R3, tkp1, tk, dimX, dimY, dimZ);
            
            /* calculate norm - stopping rules*/
            re = 0.0f; re1 = 0.0f;
            for(j=0; j<dimX*dimY*dimZ; j++)
            {
                re += pow(Output[j] - Output_prev[j],2);
                re1 += pow(Output[j],2);
            }
            re = sqrt(re)/sqrt(re1);
            /* stop if the norm residual is less than the tolerance EPS */
            if (re < epsil)  count++;
            if (count > 4) break;            
                        
            /*storing old values*/
            copyIm(Output, Output_prev, dimX, dimY, dimZ);
            copyIm(P1, P1_prev, dimX, dimY, dimZ);
            copyIm(P2, P2_prev, dimX, dimY, dimZ);
            copyIm(P3, P3_prev, dimX, dimY, dimZ);
            tk = tkp1;            
        }	
		if (printM == 1) printf("FGP-TV iterations stopped at iteration %i \n", ll);   
		free(Output_prev); free(P1); free(P2); free(P3); free(P1_prev); free(P2_prev); free(P3_prev); free(R1); free(R2); free(R3);
	}
	return *Output;
}

float Obj_func2D(float *A, float *D, float *R1, float *R2, float lambda, int dimX, int dimY)
{
    float val1, val2;
    int i,j,index;
#pragma omp parallel for shared(A,D,R1,R2) private(index,i,j,val1,val2)
    for(i=0; i<dimX; i++) {
        for(j=0; j<dimY; j++) {
			index = j*dimX+i;
            /* boundary conditions  */
            if (i == 0) {val1 = 0.0f;} else {val1 = R1[j*dimX + (i-1)];}
            if (j == 0) {val2 = 0.0f;} else {val2 = R2[(j-1)*dimX + i];}
            D[index] = A[index] - lambda*(R1[index] + R2[index] - val1 - val2);
        }}
    return *D;
}
float Grad_func2D(float *P1, float *P2, float *D, float *R1, float *R2, float lambda, int dimX, int dimY)
{
    float val1, val2, multip;
    int i,j,index;
    multip = (1.0f/(8.0f*lambda));
#pragma omp parallel for shared(P1,P2,D,R1,R2,multip) private(index,i,j,val1,val2)
    for(i=0; i<dimX; i++) {
        for(j=0; j<dimY; j++) {
			index = j*dimX+i;
            /* boundary conditions */
            if (i == dimX-1) val1 = 0.0f; else val1 = D[index] - D[j*dimX + (i+1)];
            if (j == dimY-1) val2 = 0.0f; else val2 = D[index] - D[(j+1)*dimX + i];
            P1[index] = R1[index] + multip*val1;
            P2[index] = R2[index] + multip*val2;
        }}
    return 1;
}
float Proj_func2D(float *P1, float *P2, int methTV, int dimX, int dimY)
{
    float val1, val2, denom, sq_denom;
    int i,j,index;
    if (methTV == 0) {
        /* isotropic TV*/
#pragma omp parallel for shared(P1,P2) private(index,i,j,denom,sq_denom)
        for(i=0; i<dimX; i++) {
            for(j=0; j<dimY; j++) {
				index = j*dimX+i;
                denom = pow(P1[index],2) +  pow(P2[index],2);
                if (denom > 1.0f) {
					sq_denom = 1.0f/sqrt(denom);
                    P1[index] = P1[index]*sq_denom;
                    P2[index] = P2[index]*sq_denom;
                }
            }}
    }
    else {
        /* anisotropic TV*/
#pragma omp parallel for shared(P1,P2) private(index,i,j,val1,val2)
        for(i=0; i<dimX; i++) {
            for(j=0; j<dimY; j++) {
				index = j*dimX+i;
                val1 = fabs(P1[index]);
                val2 = fabs(P2[index]);
                if (val1 < 1.0f) {val1 = 1.0f;}
                if (val2 < 1.0f) {val2 = 1.0f;}
                P1[index] = P1[index]/val1;
                P2[index] = P2[index]/val2;
            }}
    }
    return 1;
}
float Rupd_func2D(float *P1, float *P1_old, float *P2, float *P2_old, float *R1, float *R2, float tkp1, float tk, int dimX, int dimY)
{
    int i,j,index;
    float multip;
    multip = ((tk-1.0f)/tkp1);
#pragma omp parallel for shared(P1,P2,P1_old,P2_old,R1,R2,multip) private(index,i,j)
    for(i=0; i<dimX; i++) {
        for(j=0; j<dimY; j++) {
			index = j*dimX+i;
            R1[index] = P1[index] + multip*(P1[index] - P1_old[index]);
            R2[index] = P2[index] + multip*(P2[index] - P2_old[index]);
        }}
    return 1;
}

/* 3D-case related Functions */
/*****************************************************************/
float Obj_func3D(float *A, float *D, float *R1, float *R2, float *R3, float lambda, int dimX, int dimY, int dimZ)
{
    float val1, val2, val3;
    int i,j,k,index;
#pragma omp parallel for shared(A,D,R1,R2,R3) private(index,i,j,k,val1,val2,val3)
    for(i=0; i<dimX; i++) {
        for(j=0; j<dimY; j++) {
            for(k=0; k<dimZ; k++) {
				index = (dimX*dimY)*k + j*dimX+i;	
                /* boundary conditions */
                if (i == 0) {val1 = 0.0f;} else {val1 = R1[(dimX*dimY)*k + j*dimX + (i-1)];}
                if (j == 0) {val2 = 0.0f;} else {val2 = R2[(dimX*dimY)*k + (j-1)*dimX + i];}
                if (k == 0) {val3 = 0.0f;} else {val3 = R3[(dimX*dimY)*(k-1) + j*dimX + i];}
                D[index] = A[index] - lambda*(R1[index] + R2[index] + R3[index] - val1 - val2 - val3);
            }}}
    return *D;
}
float Grad_func3D(float *P1, float *P2, float *P3, float *D, float *R1, float *R2, float *R3, float lambda, int dimX, int dimY, int dimZ)
{
    float val1, val2, val3, multip;
    int i,j,k, index;
    multip = (1.0f/(8.0f*lambda));
#pragma omp parallel for shared(P1,P2,P3,D,R1,R2,R3,multip) private(index,i,j,k,val1,val2,val3)
    for(i=0; i<dimX; i++) {
        for(j=0; j<dimY; j++) {
            for(k=0; k<dimZ; k++) {
				index = (dimX*dimY)*k + j*dimX+i;				
                /* boundary conditions */
                if (i == dimX-1) val1 = 0.0f; else val1 = D[index] - D[(dimX*dimY)*k + j*dimX + (i+1)];
                if (j == dimY-1) val2 = 0.0f; else val2 = D[index] - D[(dimX*dimY)*k + (j+1)*dimX + i];
                if (k == dimZ-1) val3 = 0.0f; else val3 = D[index] - D[(dimX*dimY)*(k+1) + j*dimX + i];
                P1[index] = R1[index] + multip*val1;
                P2[index] = R2[index] + multip*val2;
                P3[index] = R3[index] + multip*val3;
            }}}
    return 1;
}
float Proj_func3D(float *P1, float *P2, float *P3, int methTV, int dimX, int dimY, int dimZ)
{		
    float val1, val2, val3, denom, sq_denom;
    int i,j,k, index;
    if (methTV == 0) {
	/* isotropic TV*/
	#pragma omp parallel for shared(P1,P2,P3) private(index,i,j,k,val1,val2,val3,sq_denom)
    for(i=0; i<dimX; i++) {
        for(j=0; j<dimY; j++) {
            for(k=0; k<dimZ; k++) {
				index = (dimX*dimY)*k + j*dimX+i;				
				denom = pow(P1[index],2) + pow(P2[index],2) + pow(P3[index],2);				
                if (denom > 1.0f) {
					sq_denom = 1.0f/sqrt(denom);
                    P1[index] = P1[index]*sq_denom;
                    P2[index] = P2[index]*sq_denom;
                    P3[index] = P3[index]*sq_denom;
                }				
			}}}	
	}    
    else {
    /* anisotropic TV*/
#pragma omp parallel for shared(P1,P2,P3) private(index,i,j,k,val1,val2,val3)
    for(i=0; i<dimX; i++) {
        for(j=0; j<dimY; j++) {
            for(k=0; k<dimZ; k++) {
				index = (dimX*dimY)*k + j*dimX+i;		
                val1 = fabs(P1[index]);
                val2 = fabs(P2[index]);
                val3 = fabs(P3[index]);
                if (val1 < 1.0f) {val1 = 1.0f;}
                if (val2 < 1.0f) {val2 = 1.0f;}
                if (val3 < 1.0f) {val3 = 1.0f;}                
                P1[index] = P1[index]/val1;
                P2[index] = P2[index]/val2;
                P3[index] = P3[index]/val3;
            }}}
		}
    return 1;
}
float Rupd_func3D(float *P1, float *P1_old, float *P2, float *P2_old, float *P3, float *P3_old, float *R1, float *R2, float *R3, float tkp1, float tk, int dimX, int dimY, int dimZ)
{
    int i,j,k,index;
    float multip;
    multip = ((tk-1.0f)/tkp1);
#pragma omp parallel for shared(P1,P2,P3,P1_old,P2_old,P3_old,R1,R2,R3,multip) private(index,i,j,k)
    for(i=0; i<dimX; i++) {
        for(j=0; j<dimY; j++) {
            for(k=0; k<dimZ; k++) {
				index = (dimX*dimY)*k + j*dimX+i;	
                R1[index] = P1[index] + multip*(P1[index] - P1_old[index]);
                R2[index] = P2[index] + multip*(P2[index] - P2_old[index]);
                R3[index] = P3[index] + multip*(P3[index] - P3_old[index]);
            }}}
    return 1;
}
