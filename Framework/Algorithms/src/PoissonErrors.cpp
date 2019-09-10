// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PoissonErrors.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Algorithms {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(PoissonErrors)

/** Performs a simple check to see if the sizes of two workspaces are
 * identically sized
 * @param lhs :: the first workspace to compare
 * @param rhs :: the second workspace to compare
 * @retval "" The two workspaces are size compatible
 * @retval "<reason why not compatible>" The two workspaces are NOT size
 * compatible
 */
std::string PoissonErrors::checkSizeCompatibility(
    const API::MatrixWorkspace_const_sptr lhs,
    const API::MatrixWorkspace_const_sptr rhs) const {
  // in order to be size compatible then the workspaces must be identically
  // sized
  if (lhs->size() == rhs->size()) {
    return "";
  } else {
    return "Workspaces not identically sized.";
  }
}

void PoissonErrors::performBinaryOperation(const HistogramData::Histogram &lhs,
                                           const HistogramData::Histogram &rhs,
                                           HistogramData::HistogramY &YOut,
                                           HistogramData::HistogramE &EOut) {
  // Just copy over the lhs data
  YOut = lhs.y();
  // Now make the fractional error the same as it was on the rhs
  const auto bins = static_cast<int>(lhs.e().size());
  for (int j = 0; j < bins; ++j) {
    if (rhs.y()[j] != 0.0)
      EOut[j] = rhs.e()[j] / rhs.y()[j] * lhs.y()[j];
    else
      EOut[j] = 0.0;
  }
}

void PoissonErrors::performBinaryOperation(const HistogramData::Histogram &lhs,
                                           const double rhsY, const double rhsE,
                                           HistogramData::HistogramY &YOut,
                                           HistogramData::HistogramE &EOut) {

  assert(lhs.x().size() == 1);
  // If we get here we've got two single column workspaces so it's easy.
  YOut[0] = lhs.y()[0];

  if (rhsY != 0.0)
    EOut[0] = rhsE / rhsY * lhs.y()[0];
  else
    EOut[0] = 0.0;
}
} // namespace Algorithms
} // namespace Mantid
