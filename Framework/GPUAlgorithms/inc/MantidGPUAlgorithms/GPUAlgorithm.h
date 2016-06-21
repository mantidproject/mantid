#ifndef MANTID_GPUALGORITHMS_GPUALGORITHM_H_
#define MANTID_GPUALGORITHMS_GPUALGORITHM_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include <CL/cl.hpp>

namespace Mantid {
namespace GPUAlgorithms {

/** GPUAlgorithm is a base algorithm class for algorithms
 * using OpenCL code.
 * It groups together some useful methods for building OpenCL kernels.
 *

  @author Janik Zikovsky
  @date 2011-08-28

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
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
