//
// Copyright (c) 2009 Advanced Micro Devices, Inc. All rights reserved.
//

__kernel void
hello(__global float* values, __global float *outputValues)
{
	/*
	Just a stub kernel. 
	*/
  
    size_t i =  get_global_id(0);
    size_t j =  get_global_id(1);
    
    // Position into the data
    size_t pos = i + get_global_size(0) * j;
    
    float val = values[pos];
    for (int it=0;it<200;it++)
      val *= 1.0001;
    outputValues[pos] = val;
}
