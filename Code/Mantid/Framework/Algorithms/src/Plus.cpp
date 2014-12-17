//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Plus.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace Algorithms {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(Plus)

// ===================================== HISTOGRAM BINARY OPERATIONS
// ==========================================
//---------------------------------------------------------------------------------------------
void Plus::performBinaryOperation(const MantidVec &lhsX, const MantidVec &lhsY,
                                  const MantidVec &lhsE, const MantidVec &rhsY,
                                  const MantidVec &rhsE, MantidVec &YOut,
                                  MantidVec &EOut) {
  (void)lhsX; // Avoid compiler warning
  std::transform(lhsY.begin(), lhsY.end(), rhsY.begin(), YOut.begin(),
                 std::plus<double>());
  std::transform(lhsE.begin(), lhsE.end(), rhsE.begin(), EOut.begin(),
                 VectorHelper::SumGaussError<double>());
}

//---------------------------------------------------------------------------------------------
void Plus::performBinaryOperation(const MantidVec &lhsX, const MantidVec &lhsY,
                                  const MantidVec &lhsE, const double rhsY,
                                  const double rhsE, MantidVec &YOut,
                                  MantidVec &EOut) {
  (void)lhsX; // Avoid compiler warning
  std::transform(lhsY.begin(), lhsY.end(), YOut.begin(),
                 std::bind2nd(std::plus<double>(), rhsY));
  // Only do E if non-zero, otherwise just copy
  if (rhsE != 0)
    std::transform(lhsE.begin(), lhsE.end(), EOut.begin(),
                   std::bind2nd(VectorHelper::SumGaussError<double>(), rhsE));
  else
    EOut = lhsE;
}

// ===================================== EVENT LIST BINARY OPERATIONS
// ==========================================
/** Carries out the binary operation IN-PLACE on a single EventList,
 * with another EventList as the right-hand operand.
 * The event lists simply get appended.
 *
 *  @param lhs :: Reference to the EventList that will be modified in place.
 *  @param rhs :: Const reference to the EventList on the right hand side.
 */
void Plus::performEventBinaryOperation(DataObjects::EventList &lhs,
                                       const DataObjects::EventList &rhs) {
  // Easy, no? :) - This appends the event lists.
  lhs += rhs;
}

/** Carries out the binary operation IN-PLACE on a single EventList,
 * with another (histogrammed) spectrum as the right-hand operand.
 *
 *  @param lhs :: Reference to the EventList that will be modified in place.
 *  @param rhsX :: The vector of rhs X bin boundaries
 *  @param rhsY :: The vector of rhs data values
 *  @param rhsE :: The vector of rhs error values
 */
void Plus::performEventBinaryOperation(DataObjects::EventList &lhs,
                                       const MantidVec &rhsX,
                                       const MantidVec &rhsY,
                                       const MantidVec &rhsE) {
  (void)lhs; // Avoid compiler warnings
  (void)rhsX;
  (void)rhsY;
  (void)rhsE;
  throw Exception::NotImplementedError(
      "Plus::performEventBinaryOperation() cannot add a histogram to an event "
      "list in an EventWorkspace. Try switching to a Workspace2D before "
      "adding.");
}

/** Carries out the binary operation IN-PLACE on a single EventList,
 * with a single (double) value as the right-hand operand.
 * THROWS since it is not possible to add a value to an event list.
 *
 *  @param lhs :: Reference to the EventList that will be modified in place.
 *  @param rhsY :: The rhs data value
 *  @param rhsE :: The rhs error value
 */
void Plus::performEventBinaryOperation(DataObjects::EventList &lhs,
                                       const double &rhsY, const double &rhsE) {
  (void)lhs; // Avoid compiler warnings
  (void)rhsY;
  (void)rhsE;
  throw Exception::NotImplementedError("Plus::performEventBinaryOperation() "
                                       "cannot add a number to an event list "
                                       "in an EventWorkspace. Try switching to "
                                       "a Workspace2D before adding.");
}

//---------------------------------------------------------------------------------------------
/** Check what operation will be needed in order to apply the operation
 * to these two types of workspaces. This function must be overridden
 * and checked against all 9 possible combinations.
 *
 * Must set: m_matchXSize, m_flipSides, m_keepEventWorkspace
 */
void Plus::checkRequirements() {
  if (m_erhs && m_elhs) {
    // Two EventWorkspaces! They can be concatenated.
    // Output will be EW
    m_keepEventWorkspace = true;
    // Histogram sizes need not match
    m_matchXSize = false;
    // If adding in place to the right-hand-side: flip it so you add in-place to
    // the lhs.
    m_flipSides = (m_eout == m_erhs);
    m_useHistogramForRhsEventWorkspace = false;
    // Special case for plus/minus: if there is only one bin on the RHS, use the
    // 2D method (appending event lists)
    // so that the single bin is not treated as a scalar
    m_do2D_even_for_SingleColumn_on_rhs = true;
  } else {
    // either or both workspace are "other"
    // Use the default behaviour
    BinaryOperation::checkRequirements();

    // Except that commutation is possible
    // If LHS is smaller, e.g. single-value, flip sides to put it on the right
    m_flipSides = (m_lhs->size() < m_rhs->size());
  }
}

//---------------------------------------------------------------------------------------------
/**
 *  Return true if the units and distribution-type of the workspaces make them
 * compatible
 *  @param lhs :: first workspace to check for compatibility
 *  @param rhs :: second workspace to check for compatibility
 *  @return workspace unit compatibility flag
 */
bool
Plus::checkUnitCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                             const API::MatrixWorkspace_const_sptr rhs) const {
  if (lhs->size() > 1 && rhs->size() > 1) {
    if (lhs->YUnit() != rhs->YUnit()) {
      g_log.error("The two workspaces are not compatible because they have "
                  "different units for the data (Y).");
      return false;
    }
    if (lhs->isDistribution() != rhs->isDistribution()) {
      g_log.error("The two workspaces are not compatible because one is "
                  "flagged as a distribution.");
      return false;
    }
  }
  return true;
}

//---------------------------------------------------------------------------------------------
/**
 *  Check the given workspaces for unit, distribution and binary operation
 *  compatibility. Return is true is the workspaces are compatible.
 *  @param lhs :: first workspace to check for compatibility
 *  @param rhs :: second workspace to check for compatibility
 *  @return workspace compatibility flag
 */
bool Plus::checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                              const API::MatrixWorkspace_const_sptr rhs) const {
  if (!checkUnitCompatibility(lhs, rhs))
    return false;

  // Keep checking more generally.
  return CommutativeBinaryOperation::checkCompatibility(lhs, rhs);
}

//--------------------------------------------------------------------------------------------
/** Performs a simple check to see if the sizes of two workspaces are compatible
 * for a binary operation
 *  In order to be size compatible then the larger workspace
 *  must divide be the size of the smaller workspace leaving no remainder
 *  @param lhs :: the first workspace to compare
 *  @param rhs :: the second workspace to compare
 *  @retval "" The two workspaces are size compatible
 *  @retval "<reason why not compatible>" The two workspaces are NOT size
 * compatible
 */
std::string
Plus::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                             const API::MatrixWorkspace_const_sptr rhs) const {
  if (m_erhs && m_elhs) {
    if (lhs->getNumberHistograms() == rhs->getNumberHistograms()) {
      return "";
    } else {
      return "Number of histograms not identical.";
    }
  } else {
    // get the largest workspace
    API::MatrixWorkspace_const_sptr wsLarger;
    API::MatrixWorkspace_const_sptr wsSmaller;
    if (rhs->size() > lhs->size()) {
      wsLarger = rhs;
      wsSmaller = lhs;
    } else {
      wsLarger = lhs;
      wsSmaller = rhs;
    }
    // call the base routine
    return BinaryOperation::checkSizeCompatibility(wsLarger, wsSmaller);
  }
}

//---------------------------------------------------------------------------------------------
/** Adds the integrated proton currents, proton charges, of the two input
*  workspaces together
*  @param lhs :: one of the workspace samples to be summed
*  @param rhs :: the other workspace sample to be summed
*  @param ans :: the sample in the output workspace
*/
void Plus::operateOnRun(const Run &lhs, const Run &rhs, Run &ans) const {
  // The addition operator of Run will add the proton charges, append logs, etc.
  // If ans=lhs or ans=rhs then we need to be careful in which order we do this
  if (&rhs == &ans) {
    // The output is the same as the RHS workspace so needs to be set to that
    // run object
    ans = rhs;
    ans += lhs;
  } else {
    // The output is either the same as the LHS workspace so needs to be set to
    // that run object
    // or it is a completely separate workspace meaning that it actually doesn't
    // matter
    ans = lhs;
    ans += rhs;
  }
}
}
}
