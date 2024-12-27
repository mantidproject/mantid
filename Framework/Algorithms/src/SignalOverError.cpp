// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SignalOverError.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SignalOverError)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SignalOverError::SignalOverError() : UnaryOperation() {
  // Flag that EventWorkspaces will become histograms
  this->useHistogram = true;
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SignalOverError::name() const { return "SignalOverError"; }

/// Algorithm's version for identification. @see Algorithm::version
int SignalOverError::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SignalOverError::category() const { return "Arithmetic\\Errors"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Perform the Y/E */
void SignalOverError::performUnaryOperation(const double XIn, const double YIn, const double EIn, double &YOut,
                                            double &EOut) {
  (void)XIn; // Avoid compiler warning
  // Signal / Error
  YOut = YIn / EIn;
  // Clear the error
  EOut = 0.0;
}

} // namespace Mantid::Algorithms
