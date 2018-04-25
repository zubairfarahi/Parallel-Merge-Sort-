#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>

#define THREADS 512
#ifdef __cplusplus
__global__ void mergesort(float* source, float* dest, long size, long width, long slices, dim3* threads, dim3* blocks)
{

	unsigned int x=threads->x;
	unsigned int y=threads->y;
	unsigned int z=threads->z;
	unsigned int idx = threadIdx.x + (x*threadIdx.y)+(x*y*threadIdx.z) +(x*y*z*blockIdx.x) +(x*y*z*z*blockIdx.y)+(x*y*z*y*blockIdx.z);
	long count=0,start = width*idx*slices,middle,end;

	while(count < slices)
	{
        	if (start >= size)
	            break;

        	middle = min(start + (width >> 1), size);
	        end = min(start + width, size);
     
		long i = start, j = middle,k=start;
		while(k < end) 
		{
		        if (i < middle && (j >= end || source[i] < source[j]))
			{
		            dest[k] = source[i];
		            i++;
		        }
			else
			{
		            dest[k] = source[j];
		            j++;
		        }
			k++;
		}

	        start =start+width;
		count++;
	}
}

extern "C"
{
#endif

int cuda_sort(int number_of_elements, float *a)
{
	dim3 thread_units,block_units;
/*	printf("\n Before Sort \n");
        for(int i=0;i<number_of_elements;i++)
        {
                printf("%f \n",a[i]);
        }
*/
//	int ret=(number_of_elements / 2)%512;
	thread_units.x = THREADS;
	thread_units.y = 1;
	thread_units.z = 1;

	block_units.x = THREADS/2;//(ret==0)?number_of_elements/2:number_of_elements/2+512-ret;
	block_units.y = 1;
	block_units.z = 1;

	float *in,*out;
        dim3 *threads,*blocks;
  
//	printf("INSIDE CUDA SORT\n");
	cudaDeviceSetLimit(cudaLimitDevRuntimeSyncDepth, 16);
	cudaMalloc((void**)&in,number_of_elements*sizeof(float));
	cudaMalloc((void**)&out,number_of_elements*sizeof(float));
	cudaMalloc((void**) &threads, sizeof(dim3));
	cudaMalloc((void**) &blocks, sizeof(dim3));
	
	cudaMemcpy(in,a, number_of_elements*sizeof(float), cudaMemcpyHostToDevice);
	cudaMemcpy(threads, &thread_units, sizeof(dim3), cudaMemcpyHostToDevice);
	cudaMemcpy(blocks, &block_units, sizeof(dim3), cudaMemcpyHostToDevice);
	long nThreads = THREADS*(THREADS/2);
	float *data1 = in,*data2 = out;

	for (int i = 2; i < (number_of_elements << 1); i <<= 1) 
	{
        	long slices = number_of_elements / ((nThreads) * i) + 1;
		mergesort<<<block_units,thread_units>>>(data1, data2, number_of_elements, i, slices, threads, blocks);
	//swapping	
		if(data1==in)			
			data1 = out;
		else
			data1=in;
		if(data2==in)
		        data2 = out;
		else 
			data2=in;
	}
	cudaMemcpy(a, data1, number_of_elements * sizeof(float), cudaMemcpyDeviceToHost);
/*	printf("\n After Sort \n");
	for(int i=0;i<number_of_elements;i++)
	{
		printf("%f \n",a[i]);
	}*/
	cudaFree(out);
	cudaFree(in);
	cudaDeviceReset();
	return 0;
}

#ifdef __cplusplus
}
#endif
