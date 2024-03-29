// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Exponential.h"
#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid::Algorithms {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(Exponential)

Exponential::Exponential() : UnaryOperation() { this->useHistogram = true; }

void Exponential::performUnaryOperation(const double XIn, const double YIn, const double EIn, double &YOut,
                                        double &EOut) {
  (void)XIn; // Avoid compiler warning
  // Multiply the data and error by the correction factor
  YOut = exp(YIn);
  EOut = EIn * YOut;
}

/*
void Exponential::setOutputUnits(const API::MatrixWorkspace_const_sptr
lhs,API::MatrixWorkspace_sptr out)
{
  // If Y has not units, then the output will be dimensionless, but not a
distribution
  if ( lhs->YUnit().empty() )
  {
    out->setYUnit("");
    out->setDistribution(false); // might be, maybe not?
  }
  // Else units are questionable...
  else
  {
    out->setYUnit("exp(" + lhs->YUnit()+ ")");
  }
}
*/

} // namespace Mantid::Algorithms
