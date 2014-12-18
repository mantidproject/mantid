#ifndef MANTID_GPUALGORITHMS_GPUTESTER_H_
#define MANTID_GPUALGORITHMS_GPUTESTER_H_
    
#include "MantidAPI/Algorithm.h"
#include "MantidGPUAlgorithms/GPUAlgorithm.h"
#include "MantidKernel/System.h"
#include <CL/cl.hpp>

namespace Mantid
{
namespace GPUAlgorithms
{

  /** A simple algorithm to test the capabilities
   * of OpenCL and the GPU card.
    
    @author Janik Zikovsky
    @date 2011-08-27

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport GPUTester  : public GPUAlgorithm
  {
  public:
    GPUTester();
    virtual ~GPUTester();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "GPUTester";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "GPUAlgorithms";}
    /// Summary of algorithms purpose
    virtual const std::string summary() const {return "A dummy algorithm to test the capabilities of the GPU card for computation.";}

  private:
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

  protected:
    static cl::Kernel kernel;
    static cl::CommandQueue queue;
    static cl::Context context;

    /// Bool set to true once the kernel has been built.
    static bool m_kernelBuilt;

  };


} // namespace GPUAlgorithms
} // namespace Mantid

#endif  /* MANTID_GPUALGORITHMS_GPUTESTER_H_ */
