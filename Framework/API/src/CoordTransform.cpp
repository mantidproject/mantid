// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/CoordTransform.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/VMD.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid::API {

//----------------------------------------------------------------------------------------------
/** Constructor. Validates the inputs
 *
 * @param inD :: input number of dimensions, >= 1
 * @param outD :: output number of dimensions, <= inD
 * @throw std::runtime_error if outD > inD
 */
CoordTransform::CoordTransform(const size_t inD, const size_t outD) : inD(inD), outD(outD) {
  if (outD > inD)
    throw std::runtime_error("CoordTransform: Cannot have more output "
                             "dimensions than input dimensions!");
  if (outD == 0)
    throw std::runtime_error("CoordTransform: invalid number of output dimensions!");
  if (inD == 0)
    throw std::runtime_error("CoordTransform: invalid number of input dimensions!");
}

//----------------------------------------------------------------------------------------------
/** Apply the transformation to an input vector (as a VMD type).
 * This wraps the apply(in,out) method (and will be slower!)
 *
 * @param inputVector :: an inD-length vector
 * @return the output vector as VMD
 */
Mantid::Kernel::VMD CoordTransform::applyVMD(const Mantid::Kernel::VMD &inputVector) const {
  if (inputVector.getNumDims() != inD)
    throw std::runtime_error("CoordTransform::apply(): inputVector has the "
                             "wrong number of coordinates!");
  auto outArray = new coord_t[outD];
  this->apply(inputVector.getBareArray(), outArray);
  VMD out(outD, outArray);
  delete[] outArray;
  return out;
}

} // namespace Mantid::API
