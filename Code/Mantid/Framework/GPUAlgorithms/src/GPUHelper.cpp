#include "MantidGPUAlgorithms/GPUHelper.h"
#include "MantidKernel/System.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <CL/cl.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>
#include "MantidKernel/ConfigService.h"

using Mantid::Kernel::ConfigService;

namespace Mantid
{
namespace GPUAlgorithms
{
namespace GPUHelper
{


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

    // Try the file path exactly
    std::string fullFilename = filename;
    if (!Poco::File(fullFilename).exists())
    {
      // If file does not exist, look in the path for kernel files from the Mantid.properties file
      fullFilename = ConfigService::Instance().getString("openclKernelFiles.directory")
          + std::string("/") + filename;
    }

    std::ifstream file(fullFilename.c_str());
    if (!file.is_open())
      throw std::runtime_error("Could not open the OpenCL file: " + fullFilename);
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

} // namespace GPUHelper
} // namespace Mantid
} // namespace GPUAlgorithms

