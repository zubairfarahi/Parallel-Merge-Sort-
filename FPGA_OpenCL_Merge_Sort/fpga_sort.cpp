#include <stdio.h>
#include <stdlib.h>
#include "mysort.h"
#include "CL/opencl.h"
#include "AOCL_Utils.h"
#include <math.h>
#include <sstream>
#include <cstring>
using namespace aocl_utils;

// OpenCL runtime configuration
cl_platform_id platform = NULL;
unsigned num_devices = 0;
scoped_array<cl_device_id> device; // num_devices elements
cl_context context = NULL;
scoped_array<cl_command_queue> queue; // num_devices elements
cl_program program = NULL;
scoped_array<cl_kernel> kernel; // num_devices elements
scoped_array<cl_kernel> kernel1;
cl_mem inp_data; // to store input array
cl_mem out_data;
bool init_opencl();

int fpga_sort(int num_of_elements, float *data)
{
//	fprintf(stderr,"HERE");
	int err=0;
        cl_int status;
	int size=num_of_elements * sizeof(float);
//	fprintf(stderr,"enter");
	int localworksize=256;
	init_opencl();
//	fprintf(stderr,"\nexit\n");
	//	localWorkSize[0]=64;
//	globalWorkSize[0]=num_of_elements;//((num_of_elements/2)%localWorkSize[0]==0)?(num_of_elements/2):(num_of_elements/2)+localWorkSize[0]-((num_of_elements/2)%localWorkSize[0]);
        inp_data = clCreateBuffer(context, CL_MEM_READ_WRITE,size, NULL, &err);
	if (err != CL_SUCCESS)
	{  
		fprintf(stderr, "Failed to create buffer for input A");
		exit(1);
	}
        out_data = clCreateBuffer(context, CL_MEM_READ_WRITE,size, NULL, &err);
        if (err != CL_SUCCESS)
        {
                fprintf(stderr, "Failed to create buffer for input A");
                exit(1);
        }
	err = clEnqueueWriteBuffer(queue[0], inp_data, CL_FALSE, 0, size, data, 0, NULL, NULL);
	if (err != CL_SUCCESS)
	{  
		fprintf(stderr, "Failed to transfer buffer for output C");
		exit(1);
	}
  	size_t global_work_size[1];
  	size_t local_work_size[1];
	local_work_size[0] = localworksize;
	int x=(num_of_elements / 2)%local_work_size[0];
	global_work_size[0] = (x==0)?num_of_elements/2:num_of_elements/2+local_work_size[0]-x;
	unsigned int locLimit = 1;
	int p=0;
	locLimit = 2 * local_work_size[0];
	status = clSetKernelArg(kernel[0], 0, sizeof(cl_mem), &inp_data);
	checkError(status, "Failed to set argument %d", 0);
	status = clSetKernelArg(kernel[0], 1, sizeof(cl_mem), &out_data);
	checkError(status, "Failed to set argument %d", 1);
	//fprintf(stderr,"before\n");
	status = clEnqueueNDRangeKernel(queue[0], kernel[0], 1, NULL, global_work_size, local_work_size, 0, NULL, NULL);
	checkError(status, "Failed to launch kernel");
	cl_mem tmp;
	tmp = out_data;
	out_data = inp_data;
	inp_data = tmp;
	
	local_work_size[0] = localworksize;
	x=(num_of_elements / 2)%local_work_size[0];
	global_work_size[0] = (x==0)?num_of_elements/2:num_of_elements/2+local_work_size[0]-x;
	
	status = clSetKernelArg(kernel1[0], 3, sizeof(cl_uint), &num_of_elements);
        checkError(status, "Failed to set argument of kernel2 %d", 3);
	for (int stride = 2*locLimit; stride <= num_of_elements; stride <<= 1)
	{
		size_t neededWorkers = num_of_elements / stride;
		local_work_size[0] = (256<= neededWorkers)?256:neededWorkers;
		global_work_size[0] = (neededWorkers%local_work_size[0]==0)?neededWorkers:neededWorkers+local_work_size[0]-(neededWorkers%local_work_size[0]);
		status = clSetKernelArg(kernel1[0], 0, sizeof(cl_mem), &inp_data);
		status |= clSetKernelArg(kernel1[0], 1, sizeof(cl_mem),&out_data);
		status |= clSetKernelArg(kernel1[0], 2, sizeof(cl_uint), &stride);
		checkError(status, "Failed to set argument %d", 1);
		status = clEnqueueNDRangeKernel(queue[0], kernel1[0], 1, NULL, global_work_size, local_work_size, 0, NULL, NULL);
		checkError(status, "Failed to set argument %d", 1);
		cl_mem tmp=inp_data;
		inp_data=out_data;
		out_data=tmp;
	}
	status = clEnqueueReadBuffer(queue[0], inp_data, CL_TRUE,0, size, data, 0, NULL, NULL);
	checkError(status, "Failed to read output matrix");	
	clFinish(queue[0]);
	return 0;
}

// Initializes the OpenCL objects.
bool init_opencl() {
  cl_int status;

  printf("Initializing OpenCL\n");

  if(!setCwdToExeDir()) {
    return false;
  }

  // Get the OpenCL platform.
  platform = findPlatform("Altera");
  if(platform == NULL) {
    printf("ERROR: Unable to find Altera OpenCL platform.\n");
    return false;
  }

  // Query the available OpenCL device.
  device.reset(getDevices(platform, CL_DEVICE_TYPE_ALL, &num_devices));
  printf("Platform: %s\n", getPlatformName(platform).c_str());
  printf("Using %d device(s)\n", num_devices);
  for(unsigned i = 0; i < num_devices; ++i) {
    printf("  %s\n", getDeviceName(device[i]).c_str());
  }

  // Create the context.
  context = clCreateContext(NULL, num_devices, device, NULL, NULL, &status);
  checkError(status, "Failed to create context");

  // Create the program for all device. Use the first device as the
  // representative device (assuming all device are of the same type).
  std::string binary_file = getBoardBinaryFile("fpgasort", device[0]);
  printf("Using AOCX: %s\n", binary_file.c_str());
  program = createProgramFromBinary(context, binary_file.c_str(), device, num_devices);

  // Build the program that was just created.
  status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
  checkError(status, "Failed to build program");

  // Create per-device objects.
  queue.reset(num_devices);
  kernel.reset(num_devices);
  kernel1.reset(num_devices);
  for(unsigned i = 0; i < num_devices; ++i) {
    // Command queue.
    queue[i] = clCreateCommandQueue(context, device[i], CL_QUEUE_PROFILING_ENABLE, &status);
    checkError(status, "Failed to create command queue");

    // Kernel.
    const char *kernel_name = "fpgasort";
    kernel[i] = clCreateKernel(program, kernel_name, &status);
    checkError(status, "Failed to create kernel");
    kernel1[i] = clCreateKernel(program, "mergearr", &status);
    checkError(status, "Failed to create kernel");
  }

  return true;
}

void cleanup() {
  for(unsigned i = 0; i < num_devices; ++i) {
    if(kernel && kernel[i]) {
      clReleaseKernel(kernel[i]);
    }
    if(kernel1 && kernel1[i])
	clReleaseKernel(kernel1[i]);
    if(queue && queue[i]) {
      clReleaseCommandQueue(queue[i]);
    }
  }

  if(program) {
    clReleaseProgram(program);
  }
  if(context) {
    clReleaseContext(context);
  }
}


