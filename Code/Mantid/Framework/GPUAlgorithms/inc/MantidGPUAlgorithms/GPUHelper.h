#ifndef MANTID_GPUALGORITHMS_GPUHELPER_H_
#define MANTID_GPUALGORITHMS_GPUHELPER_H_
    
#include "MantidKernel/System.h"
#include <CL/cl.hpp>

namespace Mantid
{
namespace GPUAlgorithms
{
namespace GPUHelper
{

  void checkError(const std::string & message, cl_int & err);

  void buildOpenCLKernel(std::string filename, std::string functionName,
      cl::Kernel & kernel, cl::CommandQueue & queue, cl::Context & context);


}
} // namespace GPUAlgorithms
} // namespace Mantid

#endif  /* MANTID_GPUALGORITHMS_GPUHELPER_H_ */
