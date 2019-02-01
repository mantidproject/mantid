// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GPUALGORITHMS_GPUALGORITHM_H_
#define MANTID_GPUALGORITHMS_GPUALGORITHM_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include <CL/cl.hpp>

namespace Mantid {
namespace GPUAlgorithms {

/** GPUAlgorithm is a base algorithm class for algorithms
 * using OpenCL code.
 * It groups together some useful methods for building OpenCL kernels.
 *

  @author Janik Zikovsky
  @date 2011-08-28
*/
class DLLExport GPUAlgorithm : public API::Algorithm {
public:
  GPUAlgorithm();
  virtual ~GPUAlgorithm();

protected:
  /// Check the openCL error code, throw if not CL_SUCCESS
  void checkError(const std::string &message);

  /// Build openCL kernel by loading a .cl file
  void buildKernelFromFile(std::string filename, std::string functionName,
                           cl::Kernel &kernel, cl::CommandQueue &queue,
                           cl::Context &context);

  /// Build openCL kernel from a string
  void buildKernelFromCode(const std::string &code,
                           const std::string &functionName, cl::Kernel &kernel,
                           cl::CommandQueue &queue, cl::Context &context);

  /// OpenCL error code from the latest command.
  cl_int err;
};

} // namespace GPUAlgorithms
} // namespace Mantid

#endif /* MANTID_GPUALGORITHMS_GPUALGORITHM_H_ */
