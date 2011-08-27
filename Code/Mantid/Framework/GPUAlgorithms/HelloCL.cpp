//
// Copyright (c) 2009 Advanced Micro Devices, Inc. All rights reserved.
//

#include <CL/cl.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include "MantidKernel/CPUTimer.h"

using namespace Mantid::Kernel;

//#define __NO_STD_VECTOR // Use cl::vector instead of STL version
//#define __NO_STD_STRING


//------------------------------------------------------------------------------------------
/** Method that checks the OpenCL error code and throws an exception if it is not CL_SUCCESS
 *
 * @param message :: string at the beginning of the exception message
 * @param err :: error code to check
 */
void checkError(const std::string & message, cl_int & err)
{
  std::ostringstream mess;
  mess << "OpenCL Error: " << message << " (" << err << ")";
  if(err != CL_SUCCESS)
    throw std::runtime_error(mess.str());
}


//------------------------------------------------------------------------------------------
/** Build and compile a OpenCL kernel from a .cl file
 *
 * @param filename :: path to a .cl kernel file
 * @param functionName :: name of the function in the .cl file.
 * @param[out] kernel :: return the created cl::Kernel program
 * @param[out] queue :: return the CommandQueue for this kernel
 * @param[out] context :: return the Context (devices) for this kernel
 */
void buildOpenCLKernel(std::string filename, std::string functionName,
    cl::Kernel & kernel, cl::CommandQueue & queue, cl::Context & context)
{
  bool verbose = false;
  cl_int err;

  // Platform info
  std::vector<cl::Platform> platforms;
  if (verbose) std::cout<<"Getting Platform Information\n";
  err = cl::Platform::get(&platforms);
  checkError("Platform::get() failed", err);

  std::vector<cl::Platform>::iterator i;
  if(platforms.size() > 0)
  {
    for(i = platforms.begin(); i != platforms.end(); ++i)
    {
      if (verbose) std::cout << "Platform: " << (*i).getInfo<CL_PLATFORM_VENDOR>(&err).c_str() << std::endl;
      if(!strcmp((*i).getInfo<CL_PLATFORM_VENDOR>(&err).c_str(), "Advanced Micro Devices, Inc."))
        break;
    }
  }
  checkError("Platform::getInfo() failed", err);

  /*
   * If we could find our platform, use it. Otherwise pass a NULL and get whatever the
   * implementation thinks we should be using.
   */

  cl_context_properties cps[3] = { CL_CONTEXT_PLATFORM, (cl_context_properties)(*i)(), 0 };

  if (verbose) std::cout<<"Creating a context AMD platform\n";
  context = cl::Context(CL_DEVICE_TYPE_CPU, cps, NULL, NULL, &err);
  checkError("Context::Context() failed", err);

  if (verbose) std::cout<<"Getting device info\n";
  std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
  checkError("Context::getInfo() failed", err);
  if (devices.size() == 0)
    throw std::runtime_error("OpenCL Error: No device available");

  if (verbose) std::cout<<"Loading and compiling CL source\n";
  std::ifstream file(filename.c_str());
  if (!file.is_open())
    throw std::runtime_error("Could not open the OpenCL file: " + filename);
  std::string kernelStr;
  while ( file.good() )
  {
    std::string line;
    std::getline(file,line);
    kernelStr += line + "\n";
  }
  file.close();


  cl::Program::Sources sources(1, std::make_pair(kernelStr.c_str(), kernelStr.size()));

  cl::Program program = cl::Program(context, sources, &err);
  checkError("Program::Program() failed", err);

  std::cout << devices.size() << " devices\n";
  // Build the kernel program
  err = program.build(devices);
  if (err != CL_SUCCESS) {

    if(err == CL_BUILD_PROGRAM_FAILURE)
    {
      std::string str = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);
      std::cout << " \n\t\t\tERROR BUILDING. BUILD LOG\n";
      std::cout << " ************************************************\n";
      std::cout << str.c_str() << std::endl;
      std::cout << " ************************************************\n";
    }

    checkError("Program::build() failed", err);
  }

  // Create the kernel
  kernel = cl::Kernel(program, functionName.c_str(), &err);
  checkError("Kernel::Kernel() failed", err);

  // Queue a command
  queue = cl::CommandQueue(context, devices[0], 0, &err);
  checkError("CommandQueue::CommandQueue() failed", err);

}

//------------------------------------------------------------------------------------------
int main()
{
  cl_int err;
  cl::Kernel kernel;
  cl::CommandQueue queue;
  cl::Context context;
  buildOpenCLKernel("HelloCL_Kernels.cl", "hello", kernel, queue, context);

  // Create a buffer to write to target.
  std::cout<<"Writing input buffer to host \n";

  // Set Presistent memory only for AMD platform
  cl_mem_flags inMemFlags = CL_MEM_READ_ONLY;
  // TODO: For AMD platform only?
  inMemFlags |= CL_MEM_USE_PERSISTENT_MEM_AMD;

  // -------------------- Create the input data -------------------------
  size_t numValues = 2048*2048;
  size_t bufferSize = numValues * sizeof(float);
  float * values = new float[numValues];
  for (size_t i=0; i<numValues; i++)
    values[i] = float(i);

  /* Create memory object for input */
  cl::Buffer inputBuffer = cl::Buffer(context,
      inMemFlags, bufferSize,
      /* (void *) values*/ NULL,  &err);
  if (err != CL_SUCCESS) throw std::runtime_error("Input Buffer creation failed.");

  /* ------------------Create memory object for output ---------------  */
  float * outputValues = new float[numValues];
  float * expectedValues = new float[numValues];

  cl::Buffer outputBuffer = cl::Buffer(context,
      CL_MEM_WRITE_ONLY, bufferSize,
      NULL, &err);
  if (err != CL_SUCCESS) throw std::runtime_error("Output Buffer creation failed.");

  CPUTimer tim;

  /* ------------------ Write memory from host to target ---------------  */
  cl::Event writeEvt;
  err = queue.enqueueWriteBuffer(
      inputBuffer,
      CL_FALSE, /* Non-blocking! dont modify the buffer */
      0, /* offset */
      bufferSize, /* size */
      (void *) values, /* Memory location in HOST */
      NULL, &writeEvt);
  if (err != CL_SUCCESS) throw std::runtime_error("queue.enqueueWriteBuffer() failed.");

  /* Set appropriate arguments to the kernel */

  /* input buffer image */
  err = kernel.setArg(0, inputBuffer);
  if (err != CL_SUCCESS) throw std::runtime_error("kernel.setArg(0) failed.");

  /* outBuffer imager */
  err = kernel.setArg(1, outputBuffer);
  if (err != CL_SUCCESS) throw std::runtime_error("kernel.setArg(1) failed.");

  // ---------------------------------------------------------------
  cl::NDRange globalThreads(2048, 2048);
  cl::NDRange localThreads(32, 32);

  std::cout<<"Running CL program\n";
  err = queue.enqueueNDRangeKernel(
      kernel, cl::NullRange, globalThreads, localThreads
  );
  checkError("CommandQueue::enqueueNDRangeKernel()", err);

  /* -------------------- Enqueue readBuffer ---------------------------*/
  cl::Event readEvt;
  err = queue.enqueueReadBuffer(
      outputBuffer,
      CL_FALSE,  0 /*offset*/,
      bufferSize,
      outputValues,
      NULL,  &readEvt);
  if (err != CL_SUCCESS) throw std::runtime_error(" queue.enqueueReadBuffer() failed.");

  std::cout << tim << " to queue the commands" << std::endl;

  err = queue.finish();
  if (err != CL_SUCCESS) throw std::runtime_error("Event::wait() failed");
  std::cout << tim << " to run the OpenCL kernel" << std::endl;

  for (size_t pos=0; pos<numValues; pos++)
  {
    float val = values[pos];
    for (int it=0;it<200;it++)
      val *= 1.0001;
    expectedValues[pos] = val;
  }
  std::cout << tim << " to do the same with one regular CPU" << std::endl;

  std::cout<<"Done" << std::endl;
 return 1;

  for (size_t i=0; i<numValues; i++)
  {
    if (outputValues[i] != expectedValues[i])
      throw std::runtime_error("Error in output values");
    if (false)
    {
      if (i % 8 == 0) std::cout << std::endl;
      std::cout << std::setw(7) << std::setprecision(4) << outputValues[i] << ", ";
    }
  }
  std::cout << "PASSED!" << std::endl;

  return 0;
}
