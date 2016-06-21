/// OpenCL kernel code for GPUTester algorithm

__kernel void
GPUTester_kernel(__global float* values, __global float *outputValues)
{
	/*
	Just a test kernel. 
	*/
  
    size_t i =  get_global_id(0);
    size_t j =  get_global_id(1);
    
    // Position into the data
    size_t pos = i + get_global_size(0) * j;
    
    outputValues[pos] = pos * 1;
}
