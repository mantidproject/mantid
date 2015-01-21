#include "MantidGPUAlgorithms/GPUAlgorithm.h"
#include "MantidKernel/System.h"
#include "MantidKernel/SingletonHolder.h"
#include <CL/cl.hpp>
#include <Poco/File.h>
#include <sstream>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace GPUAlgorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
GPUAlgorithm::GPUAlgorithm() : err(CL_SUCCESS) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
GPUAlgorithm::~GPUAlgorithm() {}

//------------------------------------------------------------------------------------------
/** Method that checks the OpenCL error code and throws an exception if it is
 *not CL_SUCCESS
 *
 * @param message :: string at the beginning of the exception message
 * @param err :: error code to check
 */
void GPUAlgorithm::checkError(const std::string &message) {
  if (err != CL_SUCCESS) {
    std::ostringstream mess;
    mess << "OpenCL Error: " << message << " (" << err << ")";
    throw std::runtime_error(mess.str());
  }
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
void GPUAlgorithm::buildKernelFromFile(std::string filename,
                                       std::string functionName,
                                       cl::Kernel &kernel,
                                       cl::CommandQueue &queue,
                                       cl::Context &context) {
  // Try the file path exactly
  std::string fullFilename = filename;
  if (!Poco::File(fullFilename).exists()) {
    // If file does not exist, look in the path for kernel files from the
    // Mantid.properties file
    fullFilename =
        ConfigService::Instance().getString("openclKernelFiles.directory") +
        std::string("/") + filename;
  }

  std::ifstream file(fullFilename.c_str());
  if (!file.is_open())
    throw std::runtime_error("Could not open the OpenCL file: " + fullFilename);
  std::string kernelStr;
  while (file.good()) {
    std::string line;
    std::getline(file, line);
    kernelStr += line + "\n";
  }
  file.close();

  // Pass-through to the build from code method
  buildKernelFromCode(kernelStr, functionName, kernel, queue, context);
}

//------------------------------------------------------------------------------------------
/** Build and compile a OpenCL kernel from a string of code
 *
 * @param code :: sting containing all the kernel code
 * @param functionName :: name of the function in the .cl file.
 * @param[out] kernel :: return the created cl::Kernel program
 * @param[out] queue :: return the CommandQueue for this kernel
 * @param[out] context :: return the Context (devices) for this kernel
 */
void GPUAlgorithm::buildKernelFromCode(const std::string &code,
                                       const std::string &functionName,
                                       cl::Kernel &kernel,
                                       cl::CommandQueue &queue,
                                       cl::Context &context) {
  bool verbose = true;
  cl_int err;
  // Code is loaded in kernelStr
  const std::string &kernelStr = code;

  // Platform info
  std::vector<cl::Platform> platforms;
  if (verbose)
    std::cout << "Getting Platform Information\n";
  err = cl::Platform::get(&platforms);
  checkError("Platform::get() failed");

  std::vector<cl::Platform>::iterator i;
  if (!platforms.empty()) {
    for (i = platforms.begin(); i != platforms.end(); ++i) {
      if (verbose)
        std::cout << "Platform: "
                  << (*i).getInfo<CL_PLATFORM_VENDOR>(&err).c_str()
                  << std::endl;
      if (!strcmp((*i).getInfo<CL_PLATFORM_VENDOR>(&err).c_str(),
                  "Advanced Micro Devices, Inc."))
        break;
    }
  }
  checkError("Platform::getInfo() failed");

  /*
   * If we could find our platform, use it. Otherwise pass a NULL and get
   * whatever the
   * implementation thinks we should be using.
   */

  cl_context_properties cps[3] = {CL_CONTEXT_PLATFORM,
                                  (cl_context_properties) (*i)(), 0};

  if (verbose)
    std::cout << "Creating a context AMD platform\n";
  context = cl::Context(CL_DEVICE_TYPE_CPU, cps, NULL, NULL, &err);
  checkError("Context::Context() failed");

  if (verbose)
    std::cout << "Getting device info\n";
  std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
  checkError("Context::getInfo() failed");
  if (devices.empty())
    throw std::runtime_error("OpenCL Error: No device available");

  if (verbose)
    std::cout << "Found " << devices.size() << " devices\n";

  for (size_t i = 0; i < devices.size(); i++) {
    cl::Device &dev = devices[i];
    std::cout << "----------" << std::endl;
    std::cout << "Device " << i << std::endl;
    std::cout << "----------" << std::endl;

    char deviceName[100];
    dev.getInfo(CL_DEVICE_NAME, &deviceName);
    std::cout << "CL_DEVICE_NAME = " << deviceName << std::endl;

    cl_platform_id platformId;
    dev.getInfo(CL_DEVICE_NAME, &platformId);
    std::cout << "CL_DEVICE_PLATFORM = " << platformId << std::endl;

    cl_ulong globalMemCacheSize = 0;
    dev.getInfo(CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, &globalMemCacheSize);
    std::cout << "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE = " << globalMemCacheSize
              << std::endl;

    cl_ulong globalMemSize = 0;
    dev.getInfo(CL_DEVICE_GLOBAL_MEM_SIZE, &globalMemSize);
    std::cout << "CL_DEVICE_GLOBAL_MEM_SIZE = " << globalMemSize << std::endl;

    cl_uint maxClockFrequency = 0;
    dev.getInfo(CL_DEVICE_MAX_CLOCK_FREQUENCY, &maxClockFrequency);
    std::cout << "CL_DEVICE_MAX_CLOCK_FREQUENCY = " << maxClockFrequency
              << std::endl;

    cl_uint maxComputeUnits = 0;
    dev.getInfo(CL_DEVICE_MAX_COMPUTE_UNITS, &maxComputeUnits);
    std::cout << "CL_DEVICE_MAX_COMPUTE_UNITS = " << maxComputeUnits
              << std::endl;
  }

  cl::Program::Sources sources(
      1, std::make_pair(kernelStr.c_str(), kernelStr.size()));

  cl::Program program = cl::Program(context, sources, &err);
  checkError("Program::Program() failed");

  // Build the kernel program
  err = program.build(devices);
  if (err != CL_SUCCESS) {

    if (err == CL_BUILD_PROGRAM_FAILURE) {
      std::string str = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);
      std::cout << " \n\t\t\tERROR BUILDING. BUILD LOG\n";
      std::cout << " ************************************************\n";
      std::cout << str.c_str() << std::endl;
      std::cout << " ************************************************\n";
    }

    checkError("Program::build() failed");
  }

  // Create the kernel
  kernel = cl::Kernel(program, functionName.c_str(), &err);
  checkError("Kernel::Kernel() failed");

  // Create the command queue
  queue = cl::CommandQueue(context, devices[0], 0, &err);
  checkError("CommandQueue::CommandQueue() failed");
}

} // namespace Mantid
} // namespace GPUAlgorithms
