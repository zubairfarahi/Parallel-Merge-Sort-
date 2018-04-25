#define MAX_LOCAL_SIZE 256
__kernel void fpgasort(__global float* inArray,__global float* outArray)
{
	__local float local_buffer[2][MAX_LOCAL_SIZE * 2];
	const uint local_id = get_local_id(0);
	const uint index = get_group_id(0) * (MAX_LOCAL_SIZE * 2) + local_id;
	char ind = 0;
	char ind1 = 1;

	local_buffer[0][local_id] = inArray[index];
	local_buffer[0][local_id + MAX_LOCAL_SIZE] = inArray[index + MAX_LOCAL_SIZE];
	for(unsigned int stride=2;stride <= MAX_LOCAL_SIZE * 2;stride <<= 1)
	{
		ind1 = ind;
		ind = 1 - ind1;
		uint leftBoundary = local_id * stride;
		uint rightBoundary = leftBoundary + stride;

		uint middle = leftBoundary + (stride >> 1);
		uint left = leftBoundary, right = middle;
		barrier(CLK_LOCAL_MEM_FENCE);
		if (rightBoundary > MAX_LOCAL_SIZE * 2) continue;
		
		uint i=0;
		while(i < stride)
		{
			float leftVal = local_buffer[ind1][left];
			float rightVal = local_buffer[ind1][right];
			if(left < middle && (right >= rightBoundary || leftVal <= rightVal))
			{
				local_buffer[ind][leftBoundary + i] = leftVal;
				left=left+1;
			}
			else
			{
				local_buffer[ind][leftBoundary + i] =rightVal;
				right=right+1;
			}
			i++;
		}
	}
	
	//write back
	barrier(CLK_LOCAL_MEM_FENCE);
	outArray[index] = local_buffer[ind][local_id];
	outArray[index + MAX_LOCAL_SIZE] = local_buffer[ind][local_id + MAX_LOCAL_SIZE];
}
__kernel void mergearr(__global float* restrict inArray,__global float* restrict outArray, const uint stride,const uint size)
{
	const uint global_id = get_global_id(0);
	uint left = global_id*stride,right = (global_id*stride) + (stride >> 1);
	uint i = global_id*stride;

	if ((global_id*stride+stride)>size)
		return;

	while(i<((global_id*stride)+stride))
	{
		if((left < ((global_id*stride)+(stride>>1)) && (right == ((global_id*stride)+stride) || inArray[left] <= inArray[right])) == 1)
		{
			outArray[i] = inArray[left];
			left=left+1;
		}
		else
		{
			outArray[i]=inArray[right];
			right=right+1;
		}
		i++;
	}
}

