// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/WeightedMean.h"

namespace Mantid {
namespace Algorithms {

// Algorithm must be declared
DECLARE_ALGORITHM(WeightedMean)

bool WeightedMean::checkCompatibility(
    const API::MatrixWorkspace_const_sptr lhs,
    const API::MatrixWorkspace_const_sptr rhs) const {
  if (lhs->YUnit() != rhs->YUnit()) {
    g_log.error("The two workspaces are not compatible because they have "
                "different units for the data (Y).");
    return false;
  }
  if (lhs->isDistribution() != rhs->isDistribution()) {
    g_log.error("The two workspaces are not compatible because one is flagged "
                "as a distribution.");
    return false;
  }

  return BinaryOperation::checkCompatibility(lhs, rhs);
}

/** Performs a simple check to see if the sizes of two workspaces are
 * identically sized
 *  @param lhs :: the first workspace to compare
 *  @param rhs :: the second workspace to compare
 *  @retval "" The two workspaces are size compatible
 *  @retval "<reason why not compatible>" The two workspaces are NOT size
 * compatible
 */
std::string WeightedMean::checkSizeCompatibility(
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

void WeightedMean::performBinaryOperation(const HistogramData::Histogram &lhs,
                                          const HistogramData::Histogram &rhs,
                                          HistogramData::HistogramY &YOut,
                                          HistogramData::HistogramE &EOut) {
  const size_t bins = lhs.size();
  for (size_t j = 0; j < bins; ++j) {
    if (lhs.e()[j] > 0.0 && rhs.e()[j] > 0.0) {
      const double err1 = lhs.e()[j] * lhs.e()[j];
      const double err2 = rhs.e()[j] * rhs.e()[j];
      YOut[j] = (lhs.y()[j] / err1) + (rhs.y()[j] / err2);
      EOut[j] = (err1 * err2) / (err1 + err2);
      YOut[j] *= EOut[j];
      EOut[j] = sqrt(EOut[j]);
    } else if (lhs.e()[j] > 0.0 && rhs.e()[j] <= 0.0) {
      YOut[j] = lhs.y()[j];
      EOut[j] = lhs.e()[j];
    } else if (lhs.e()[j] <= 0.0 && rhs.e()[j] > 0.0) {
      YOut[j] = rhs.y()[j];
      EOut[j] = rhs.e()[j];
    } else {
      YOut[j] = 0.0;
      EOut[j] = 0.0;
    }
  }
}

void WeightedMean::performBinaryOperation(const HistogramData::Histogram &lhs,
                                          const double rhsY, const double rhsE,
                                          HistogramData::HistogramY &YOut,
                                          HistogramData::HistogramE &EOut) {
  assert(lhs.size() == 1);
  // If we get here we've got two single column workspaces so it's easy.
  if (lhs.e()[0] > 0.0 && rhsE > 0.0) {
    const double err1 = lhs.e()[0] * lhs.e()[0];
    const double err2 = rhsE * rhsE;
    YOut[0] = (lhs.y()[0] / err1) + (rhsY / err2);
    EOut[0] = (err1 * err2) / (err1 + err2);
    YOut[0] *= EOut[0];
    EOut[0] = sqrt(EOut[0]);
  } else if (lhs.e()[0] > 0.0 && rhsE <= 0.0) {
    YOut[0] = lhs.y()[0];
    EOut[0] = lhs.e()[0];
  } else if (lhs.e()[0] <= 0.0 && rhsE > 0.0) {
    YOut[0] = rhsY;
    EOut[0] = rhsE;
  } else {
    YOut[0] = 0.0;
    EOut[0] = 0.0;
  }
}

} // namespace Algorithms
} // namespace Mantid
