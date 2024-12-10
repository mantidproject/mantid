// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/Divide.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid::Algorithms {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(Divide)

void Divide::init() {
  BinaryOperation::init();
  declareProperty("WarnOnZeroDivide", true,
                  "Algorithm usually warns if "
                  "division by 0 occurs. Set this "
                  "value to false if one does not "
                  "want this message appearing ");
}

void Divide::exec() {
  m_warnOnZeroDivide = getProperty("WarnOnZeroDivide");
  BinaryOperation::exec();
}

void Divide::performBinaryOperation(const HistogramData::Histogram &lhs, const HistogramData::Histogram &rhs,
                                    HistogramData::HistogramY &YOut, HistogramData::HistogramE &EOut) {
  const auto bins = static_cast<int>(lhs.e().size());

  for (int j = 0; j < bins; ++j) {
    // Get references to the input Y's
    const double leftY = lhs.y()[j];
    const double rightY = rhs.y()[j];

    //  error dividing two uncorrelated numbers, re-arrange so that you don't
    //  get infinity if leftY==0 (when rightY=0 the Y value and the result will
    //  both be infinity)
    // (Sa/a)2 + (Sb/b)2 = (Sc/c)2
    // (Sa c/a)2 + (Sb c/b)2 = (Sc)2
    // = (Sa 1/b)2 + (Sb (a/b2))2
    // (Sc)2 = (1/b)2( (Sa)2 + (Sb a/b)2 )
    EOut[j] = sqrt(pow(lhs.e()[j], 2) + pow(leftY * rhs.e()[j] / rightY, 2)) / fabs(rightY);

    // Copy the result last in case one of the input workspaces is also any
    // output
    YOut[j] = leftY / rightY;
  }
}

void Divide::performBinaryOperation(const HistogramData::Histogram &lhs, const double rhsY, const double rhsE,
                                    HistogramData::HistogramY &YOut, HistogramData::HistogramE &EOut) {
  if (rhsY == 0 && m_warnOnZeroDivide)
    g_log.warning() << "Division by zero: the RHS is a single-valued vector "
                       "with value zero."
                    << "\n";

  // Do the right-hand part of the error calculation just once
  const double rhsFactor = pow(rhsE / rhsY, 2);
  const auto bins = static_cast<int>(lhs.e().size());
  for (int j = 0; j < bins; ++j) {
    // Get reference to input Y
    const double leftY = lhs.y()[j];

    // see comment in the function above for the error formula
    EOut[j] = sqrt(pow(lhs.e()[j], 2) + pow(leftY, 2) * rhsFactor) / fabs(rhsY);
    // Copy the result last in case one of the input workspaces is also any
    // output
    YOut[j] = leftY / rhsY;
  }
}

void Divide::setOutputUnits(const API::MatrixWorkspace_const_sptr lhs, const API::MatrixWorkspace_const_sptr rhs,
                            API::MatrixWorkspace_sptr out) {

  if (lhs->YUnit() == rhs->YUnit()) {
    // If units match

    // output will be dimensionless
    out->setYUnit("");

    if ((lhs->isRaggedWorkspace() && rhs->isRaggedWorkspace()) || m_rhsBlocksize > 1) {
      // If both are RaggedWorkspaces OR RHS blocksize > 1

      // Output will be ragged
      out->setDistribution(true);
    }
  }

  // Else we need to set the unit that results from the division
  else {
    if (!lhs->YUnit().empty())
      out->setYUnit(lhs->YUnit() + "/" + rhs->YUnit());
    else
      out->setYUnit("1/" + rhs->YUnit());
  }
}

// ===================================== EVENT LIST BINARY OPERATIONS
// ==========================================
/** Carries out the binary operation IN-PLACE on a single EventList,
 * with another EventList as the right-hand operand.
 *
 *  @param lhs :: Reference to the EventList that will be modified in place.
 *  @param rhs :: Const reference to the EventList on the right hand side.
 */
void Divide::performEventBinaryOperation(DataObjects::EventList &lhs, const DataObjects::EventList &rhs) {
  // We must histogram the rhs event list to divide.
  MantidVec rhsY, rhsE;
  rhs.generateHistogram(rhs.readX(), rhsY, rhsE);
  lhs.divide(rhs.readX(), rhsY, rhsE);
}

/** Carries out the binary operation IN-PLACE on a single EventList,
 * with another (histogrammed) spectrum as the right-hand operand.
 *
 *  @param lhs :: Reference to the EventList that will be modified in place.
 *  @param rhsX :: The vector of rhs X bin boundaries
 *  @param rhsY :: The vector of rhs data values
 *  @param rhsE :: The vector of rhs error values
 */
void Divide::performEventBinaryOperation(DataObjects::EventList &lhs, const MantidVec &rhsX, const MantidVec &rhsY,
                                         const MantidVec &rhsE) {
  // Divide is implemented at the EventList level.
  lhs.divide(rhsX, rhsY, rhsE);
}

/** Carries out the binary operation IN-PLACE on a single EventList,
 * with a single (double) value as the right-hand operand.
 * Performs the multiplication by a scalar (with error)
 *
 *  @param lhs :: Reference to the EventList that will be modified in place.
 *  @param rhsY :: The rhs data value
 *  @param rhsE :: The rhs error value
 */
void Divide::performEventBinaryOperation(DataObjects::EventList &lhs, const double &rhsY, const double &rhsE) {
  // Multiply is implemented at the EventList level.
  lhs.divide(rhsY, rhsE);
}

//---------------------------------------------------------------------------------------------
/** Check what operation will be needed in order to apply the operation
 * to these two types of workspaces. This function must be overridden
 * and checked against all 9 possible combinations.
 *
 * Must set: m_matchXSize, m_flipSides, m_keepEventWorkspace
 */
void Divide::checkRequirements() {
  if (m_elhs) {
    // The lhs workspace is an EventWorkspace. It can be divided while keeping
    // event-ishness
    // Output will be EW
    m_keepEventWorkspace = true;
    // Histogram sizes need not match
    m_matchXSize = false;
  } else {
    m_keepEventWorkspace = false;
    m_matchXSize = true;
  }

  // Division is not commutative = you can't flip sides.
  m_flipSides = false;
  // The RHS operand will be histogrammed first.
  m_useHistogramForRhsEventWorkspace = true;
}

//--------------------------------------------------------------------------------------------
/** Performs a simple check to see if the sizes of two workspaces are compatible
 *for a binary operation
 *  In order to be size compatible then the larger workspace
 *  must divide be the size of the smaller workspace leaving no remainder
 *
 *  @param lhs :: the first workspace to compare
 *  @param rhs :: the second workspace to compare
 *  @retval "" The two workspaces are size compatible
 *  @retval "<reason why not compatible>" The two workspaces are NOT size
 *compatible
 */
std::string Divide::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                                           const API::MatrixWorkspace_const_sptr rhs) const {
  // --- Check for event workspaces - different than workspaces 2D! ---

  // A SingleValueWorkspace on the right matches anything
  if (rhs->size() == 1)
    return "";

  // A SingleValueWorkspace on the left only matches if rhs was single value
  // too. Why are you using mantid to do simple math?!?
  if (lhs->size() == 1)
    return "The left side cannot contain a single value if the right side "
           "isn't also a single value.";

  // If RHS only has one value (1D vertical), the number of histograms needs to
  // match.
  // Each lhs spectrum will be divided by that scalar
  // Are we allowing the division by different # of spectra, using detector IDs
  // to match up?
  if (m_AllowDifferentNumberSpectra ||
      (m_rhsBlocksize == 1 && lhs->getNumberHistograms() == rhs->getNumberHistograms())) {
    return "";
  }

  if (m_matchXSize) {
    // Past this point, for a 2D WS operation, we require the X arrays to match.
    // Note this only checks the first spectrum except for ragged workspaces
    if (!WorkspaceHelpers::matchingBins(*lhs, *rhs, !m_lhsRagged && !m_rhsRagged)) {
      return "X arrays must match when dividing 2D workspaces.";
    }
  }

  // We don't need to check for matching bins for events. Yay events!
  const size_t rhsSpec = rhs->getNumberHistograms();

  // If the rhs has a single spectrum, then we can divide. The block size does
  // NOT need to match,
  if (rhsSpec == 1)
    return "";

  // Otherwise, the number of histograms needs to match, but the block size of
  // each does NOT need to match.

  if (lhs->getNumberHistograms() == rhs->getNumberHistograms()) {
    return "";
  } else {
    return "Number of histograms not identical.";
  }
}

} // namespace Mantid::Algorithms
