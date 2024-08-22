// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/BinaryOperation.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Unit.h"
#include <memory>

using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using std::size_t;

namespace Mantid::Algorithms {
/** Initialisation method.
 *  Defines input and output workspaces
 *
 */
void BinaryOperation::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(inputPropName1(), "", Direction::Input),
                  "The name of the input workspace on the left hand side of the operation");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(inputPropName2(), "", Direction::Input),
                  "The name of the input workspace on the right hand side of "
                  "the operation");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(outputPropName(), "", Direction::Output),
                  "The name to call the output workspace");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("AllowDifferentNumberSpectra", false, Direction::Input),
                  "Are workspaces with different number of spectra allowed? "
                  "For example, the LHSWorkspace might have one spectrum per detector, "
                  "but the RHSWorkspace could have its spectra averaged per bank. If true, "
                  "then matching between the LHS and RHS spectra is performed (all "
                  "detectors "
                  "in a LHS spectrum have to be in the corresponding RHS) in order to "
                  "apply the RHS spectrum to the LHS.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("ClearRHSWorkspace", false, Direction::Input),
                  "For EventWorkspaces only. This will clear out event lists "
                  "from the RHS workspace as the binary operation is applied. "
                  "This can prevent excessive memory use, e.g. when subtracting "
                  "an EventWorkspace from another: memory use will be approximately "
                  "constant instead of increasing by 50%. At completion, the RHS workspace "
                  "will be empty.");
}

/** Special handling for 1-WS and 1/WS.
 *
 * @return true if the operation was handled; exec() should then return
 */
bool BinaryOperation::handleSpecialDivideMinus() {
  // Is the LHS operand a single number?
  WorkspaceSingleValue_const_sptr lhs_singleVal = std::dynamic_pointer_cast<const WorkspaceSingleValue>(m_lhs);
  WorkspaceSingleValue_const_sptr rhs_singleVal = std::dynamic_pointer_cast<const WorkspaceSingleValue>(m_rhs);

  if (lhs_singleVal) {
    MatrixWorkspace_sptr out = getProperty("OutputWorkspace");
    if (this->name() == "Divide" && !bool(rhs_singleVal)) {
      // x / workspace = Power(workspace, -1) * x
      // workspace ^ -1
      auto pow = createChildAlgorithm("Power", 0.0, 0.5, true);
      pow->setProperty("InputWorkspace", std::const_pointer_cast<MatrixWorkspace>(m_rhs));
      pow->setProperty("Exponent", -1.0);
      pow->setProperty("OutputWorkspace", out);
      pow->executeAsChildAlg();
      out = pow->getProperty("OutputWorkspace");

      // Multiply by x
      auto mult = createChildAlgorithm("Multiply", 0.5, 1.0, true);
      mult->setProperty(inputPropName1(), out); //(workspace^-1)
      mult->setProperty(inputPropName2(),
                        std::const_pointer_cast<MatrixWorkspace>(m_lhs)); // (1.0) or other number
      mult->setProperty(outputPropName(), out);
      mult->executeAsChildAlg();
      out = mult->getProperty("OutputWorkspace");
      setProperty("OutputWorkspace", out);
      return true;
    } else if (this->name() == "Minus") {
      // x - workspace = x + (workspace * -1)
      MatrixWorkspace_sptr minusOne = create<WorkspaceSingleValue>(1, Points(1));
      minusOne->dataY(0)[0] = -1.0;
      minusOne->dataE(0)[0] = 0.0;

      // workspace * -1
      auto mult = createChildAlgorithm("Multiply", 0.0, 0.5, true);
      mult->setProperty(inputPropName1(), std::const_pointer_cast<MatrixWorkspace>(m_rhs));
      mult->setProperty(inputPropName2(), minusOne);
      mult->setProperty("OutputWorkspace", out);
      mult->executeAsChildAlg();
      out = mult->getProperty("OutputWorkspace");

      // Multiply by x
      auto plus = createChildAlgorithm("Plus", 0.5, 1.0, true);
      plus->setProperty(inputPropName1(), out); //(workspace^-1)
      plus->setProperty(inputPropName2(),
                        std::const_pointer_cast<MatrixWorkspace>(m_lhs)); // (1.0) or other number
      plus->setProperty(outputPropName(), out);
      plus->executeAsChildAlg();
      out = plus->getProperty("OutputWorkspace");
      setProperty("OutputWorkspace", out);
      return true;
    }

  } // lhs_singleVal

  // Process normally
  return false;
}

/** Executes the algorithm. Will call execEvent() if appropriate.
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void BinaryOperation::exec() {
  // get input workspace, dynamic cast not needed
  m_lhs = getProperty(inputPropName1());
  m_rhs = getProperty(inputPropName2());
  m_AllowDifferentNumberSpectra = getProperty("AllowDifferentNumberSpectra");

  try {
    m_lhsBlocksize = m_lhs->blocksize();
    m_lhsRagged = false;
  } catch (std::length_error &) {
    m_lhsBlocksize = 0;
    m_lhsRagged = true;
  }
  try {
    m_rhsBlocksize = m_rhs->blocksize();
    m_rhsRagged = false;
  } catch (std::length_error &) {
    m_rhsBlocksize = 0;
    m_rhsRagged = true;
  }

  // Special handling for 1-WS and 1/WS.
  if (this->handleSpecialDivideMinus())
    return;

  // Cast to EventWorkspace pointers
  m_elhs = std::dynamic_pointer_cast<const EventWorkspace>(m_lhs);
  m_erhs = std::dynamic_pointer_cast<const EventWorkspace>(m_rhs);

  // We can clear the RHS workspace if it is an event,
  //  and we are not doing mismatched spectra (in which case you might clear it
  //  too soon!)
  //  and lhs is not rhs.
  //  and out is not rhs.
  m_ClearRHSWorkspace = getProperty("ClearRHSWorkspace");
  if (m_ClearRHSWorkspace) {
    if (m_AllowDifferentNumberSpectra || (!m_erhs) || (m_rhs == m_lhs) || (m_out == m_rhs)) {
      // std::cout << "m_ClearRHSWorkspace = false\n";
      m_ClearRHSWorkspace = false;
    }
  }

  // Make a check of what will be needed to setup the workspaces, based on the
  // input types.
  this->checkRequirements();

  if (m_flipSides) {
    // Flip the workspaces left and right
    std::swap(m_lhs, m_rhs);
    std::swap(m_elhs, m_erhs);
    std::swap(m_lhsBlocksize, m_rhsBlocksize);
    std::swap(m_lhsRagged, m_rhsRagged);
  }

  // Check that the input workspaces are compatible
  if (!checkCompatibility(m_lhs, m_rhs)) {
    std::ostringstream ostr;
    ostr << "The two workspaces are not compatible for algorithm " << this->name();
    g_log.error() << ostr.str() << '\n';
    throw std::invalid_argument(ostr.str());
  }

  // Get the output workspace
  m_out = getProperty(outputPropName());
  if (m_elhs) {
    m_eout = std::dynamic_pointer_cast<EventWorkspace>(m_out);
  }

  // Is the output going to be an EventWorkspace?
  if (m_keepEventWorkspace) {
    // The output WILL be EventWorkspace (this implies lhs is EW or rhs is EW +
    // it gets flipped)
    if (!m_elhs)
      throw std::runtime_error("BinaryOperation:: the output was set to be an "
                               "EventWorkspace (m_keepEventWorkspace == true), "
                               "but the lhs is not an EventWorkspace. There "
                               "must be a mistake in the algorithm. Contact "
                               "the developers.");

    if (m_out == m_lhs) {
      // Will be modifying the EventWorkspace in-place on the lhs. Good.
      if (!m_eout)
        throw std::runtime_error("BinaryOperation:: the output was set to be lhs, and to be an "
                                 "EventWorkspace (m_keepEventWorkspace == true), but the output is "
                                 "not an EventWorkspace. There must be a mistake in the algorithm. "
                                 "Contact the developers.");
    } else {
      // You HAVE to copy the data from lhs to to the output!
      m_out = m_lhs->clone();
      // Make sure m_eout still points to the same as m_out;
      m_eout = std::dynamic_pointer_cast<EventWorkspace>(m_out);
    }

    // Always clear the MRUs.
    m_eout->clearMRU();
    m_elhs->clearMRU();
    if (m_erhs)
      m_erhs->clearMRU();

  } else {
    // ---- Output will be WS2D -------

    // We need to create a new workspace for the output if:
    //   (a) the output workspace hasn't been set to one of the input ones, or
    //   (b) it has been, but it's not the correct dimensions
    if ((m_out != m_lhs && m_out != m_rhs) || (m_out == m_rhs && (m_lhs->size() > m_rhs->size()))) {
      // if the input workspace are specialworkspace2d, then we need to ensure
      // the map is set
      auto specialLHS = dynamic_cast<const SpecialWorkspace2D *>(m_lhs.get());
      auto specialRHS = dynamic_cast<const SpecialWorkspace2D *>(m_rhs.get());
      if (specialLHS && specialRHS) {
        m_out = create<SpecialWorkspace2D>(*specialLHS);
      } else {
        m_out = create<HistoWorkspace>(*m_lhs);
      }
    }
  }

  // only overridden for some operations (plus and minus at the time of writing)
  operateOnRun(m_lhs->run(), m_rhs->run(), m_out->mutableRun());

  // Initialise the progress reporting object
  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, m_lhs->getNumberHistograms());

  // There are now 4 possible scenarios, shown schematically here:
  // xxx x   xxx xxx   xxx xxx   xxx x
  // xxx   , xxx xxx , xxx     , xxx x
  // xxx   , xxx xxx   xxx       xxx x
  // So work out which one we have and call the appropriate function

  // Single value workspace on the right : if it is an EventWorkspace with 1
  // spectrum, 1 bin, it is treated as a scalar
  if ((m_rhs->size() == 1) && !m_do2D_even_for_SingleColumn_on_rhs) {
    doSingleValue();                            // m_lhs,m_rhs,m_out
  } else if (m_rhs->getNumberHistograms() == 1) // Single spectrum on rhs
  {
    doSingleSpectrum();
  }
  // Single column on rhs; if the RHS is an event workspace with one bin, it is
  // treated as a scalar.
  else if ((m_rhsBlocksize == 1) && !m_do2D_even_for_SingleColumn_on_rhs) {
    doSingleColumn();
  } else // The two are both 2D and should be the same size (except if LHS is an
         // event workspace)
  {
    bool mismatchedSpectra =
        (m_AllowDifferentNumberSpectra && (m_rhs->getNumberHistograms() != m_lhs->getNumberHistograms()));
    do2D(mismatchedSpectra);
  }

  setOutputUnits(m_lhs, m_rhs, m_out);

  // Assign the result to the output workspace property
  setProperty(outputPropName(), m_out);
}

/**
 * Execute a binary operation on events. Should be overridden.
 * @param lhs :: left-hand event workspace
 * @param rhs :: right-hand event workspace
 */
void BinaryOperation::execEvent(DataObjects::EventWorkspace_const_sptr lhs,
                                DataObjects::EventWorkspace_const_sptr rhs) {
  UNUSED_ARG(lhs);
  UNUSED_ARG(rhs);
  // This should never happen
  throw Exception::NotImplementedError("BinaryOperation::execEvent() is not implemented for this operation.");
}

/**
 * Return true if the two workspaces are compatible for this operation
 * Virtual: will be overridden as needed.
 * @param lhs :: left-hand workspace to check
 * @param rhs :: right-hand workspace to check
 * @return flag for the compatibility to the two workspaces
 */
bool BinaryOperation::checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                                         const API::MatrixWorkspace_const_sptr rhs) const {
  Unit_const_sptr lhs_unit;
  Unit_const_sptr rhs_unit;
  if (lhs->axes() && rhs->axes()) // If one of these is a WorkspaceSingleValue
                                  // then we don't want to check units match
  {
    lhs_unit = lhs->getAxis(0)->unit();
    rhs_unit = rhs->getAxis(0)->unit();
  }

  const std::string lhs_unitID = (lhs_unit ? lhs_unit->unitID() : "");
  const std::string rhs_unitID = (rhs_unit ? rhs_unit->unitID() : "");

  // Check the workspaces have the same units and distribution flag
  if (lhs_unitID != rhs_unitID && m_lhsBlocksize != 1 && m_rhsBlocksize != 1) {
    g_log.error("The two workspace are not compatible because they have "
                "different units on the X axis.");
    return false;
  }

  // Check the size compatibility
  const std::string checkSizeCompatibilityResult = checkSizeCompatibility(lhs, rhs);
  if (!checkSizeCompatibilityResult.empty()) {
    throw std::invalid_argument(checkSizeCompatibilityResult);
  }

  return true;
}

/** Return true if the two workspaces can be treated as event workspaces
 * for the binary operation. If so, execEvent() will be called.
 * (e.g. Plus algorithm will concatenate event lists)
 * @param lhs :: left-hand event workspace to check
 * @param rhs :: right-hand event workspace to check
 * @return false by default; will be overridden by specific algorithms
 */
bool BinaryOperation::checkEventCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                                              const API::MatrixWorkspace_const_sptr rhs) {
  UNUSED_ARG(lhs);
  UNUSED_ARG(rhs);
  return false;
}

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
std::string BinaryOperation::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                                                    const API::MatrixWorkspace_const_sptr rhs) const {
  const size_t lhsSize = lhs->size();
  const size_t rhsSize = rhs->size();
  // A SingleValueWorkspace on the right matches anything
  if (rhsSize == 1)
    return "";
  // The lhs must not be smaller than the rhs
  if (lhsSize < rhsSize)
    return "Left hand side smaller than right hand side.";

  // Did checkRequirements() tell us that the X histogram size did not matter?
  if (!m_matchXSize) {
    // If so, only the vertical # needs to match

    if (lhs->getNumberHistograms() == rhs->getNumberHistograms()) {
      return "";
    } else {
      return "Number of histograms not identical.";
    }
  }
  // Otherwise they must match both ways, or horizontally or vertically with the
  // other rhs dimension=1
  if (m_rhsBlocksize == 1 && lhs->getNumberHistograms() == rhs->getNumberHistograms())
    return "";
  // Past this point, we require the X arrays to match. Note this only checks
  // the first spectrum except for ragged workspaces
  if (!WorkspaceHelpers::matchingBins(*lhs, *rhs, !m_lhsRagged && !m_rhsRagged)) {
    return "X arrays must match when performing this operation on a 2D "
           "workspaces.";
  }

  const size_t rhsSpec = rhs->getNumberHistograms();

  if (m_lhsBlocksize == m_rhsBlocksize) {
    if (rhsSpec == 1 || lhs->getNumberHistograms() == rhsSpec) {
      return "";
    } else {
      // can't be more specific as if this is reached both failed and only one
      // or both are needed
      return "Left and right sides should contain the same amount of spectra "
             "or the right side should contain only one spectra.";
    }
  } else {
    // blocksize check failed, but still check the number of spectra to see if
    // that was wrong too
    if (rhsSpec == 1 || lhs->getNumberHistograms() == rhsSpec) {
      return "Number of y values not equal on left and right sides.";
    } else {
      // can't be more specific as if this is reached both failed and only one
      // or both are needed
      return "Number of y values not equal on left and right sides and the "
             "right side contained neither only one spectra or the same amount "
             "of spectra as the left.";
    }
  }
}

/**
 * Checks if the spectra at the given index of either input workspace is masked.
 * If so then the output spectra has zeroed data
 * and is also masked.
 * @param lhsSpectrumInfo :: The LHS spectrum info object
 * @param rhsSpectrumInfo :: The RHS spectrum info object
 * @param index :: The workspace index to check
 * @param out :: A pointer to the output workspace
 * @param outSpectrumInfo :: The spectrum info object of `out`
 * @returns True if further processing is not required on the spectra, false if
 * the binary operation should be performed.
 */
bool BinaryOperation::propagateSpectraMask(const SpectrumInfo &lhsSpectrumInfo, const SpectrumInfo &rhsSpectrumInfo,
                                           const int64_t index, MatrixWorkspace &out, SpectrumInfo &outSpectrumInfo) {
  bool continueOp(true);

  if ((lhsSpectrumInfo.hasDetectors(index) && lhsSpectrumInfo.isMasked(index)) ||
      (rhsSpectrumInfo.hasDetectors(index) && rhsSpectrumInfo.isMasked(index))) {
    continueOp = false;
    out.getSpectrum(index).clearData();
    PARALLEL_CRITICAL(setMasked) { outSpectrumInfo.setMasked(index, true); }
  }
  return continueOp;
}

/**
 * Called when the rhs operand is a single value.
 *  Loops over the lhs workspace calling the abstract binary operation function
 * with a single number as the rhs operand.
 */
void BinaryOperation::doSingleValue() {
  // Don't propate masking from the rhs here - it would be decidedly odd if the
  // single value was masked

  // Pull out the single value and its error
  const double rhsY = m_rhs->y(0)[0];
  const double rhsE = m_rhs->e(0)[0];

  // Now loop over the spectra of the left hand side calling the virtual
  // function
  const int64_t numHists = m_lhs->getNumberHistograms();

  if (m_eout) {
    // ---- The output is an EventWorkspace ------
    PARALLEL_FOR_IF(Kernel::threadSafe(*m_lhs, *m_rhs, *m_out))
    for (int64_t i = 0; i < numHists; ++i) {
      PARALLEL_START_INTERRUPT_REGION
      m_out->setSharedX(i, m_lhs->sharedX(i));
      performEventBinaryOperation(m_eout->getSpectrum(i), rhsY, rhsE);
      m_progress->report(this->name());
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  } else {
    // ---- Histogram Output -----
    PARALLEL_FOR_IF(Kernel::threadSafe(*m_lhs, *m_rhs, *m_out))
    for (int64_t i = 0; i < numHists; ++i) {
      PARALLEL_START_INTERRUPT_REGION
      m_out->setSharedX(i, m_lhs->sharedX(i));
      // Get reference to output vectors here to break any sharing outside the
      // function call below
      // where the order of argument evaluation is not guaranteed (if it's L->R
      // there would be a data race)
      HistogramData::HistogramY &outY = m_out->mutableY(i);
      HistogramData::HistogramE &outE = m_out->mutableE(i);
      performBinaryOperation(m_lhs->histogram(i), rhsY, rhsE, outY, outE);
      m_progress->report(this->name());
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  }
}

/** Called when the m_rhs operand is a 2D workspace of single values.
 *  Loops over the workspaces calling the abstract binary operation function
 * with a single number as the m_rhs operand.
 */
void BinaryOperation::doSingleColumn() {
  // Don't propate masking from the m_rhs here - it would be decidedly odd if
  // the single bin was masked

  // Now loop over the spectra of the left hand side pulling m_out the single
  // value from each m_rhs 'spectrum'
  // and then calling the virtual function
  const int64_t numHists = m_lhs->getNumberHistograms();
  auto &outSpectrumInfo = m_out->mutableSpectrumInfo();
  auto &lhsSpectrumInfo = m_lhs->spectrumInfo();
  auto &rhsSpectrumInfo = m_rhs->spectrumInfo();
  if (m_eout) {
    // ---- The output is an EventWorkspace ------
    PARALLEL_FOR_IF(Kernel::threadSafe(*m_lhs, *m_rhs, *m_out))
    for (int64_t i = 0; i < numHists; ++i) {
      PARALLEL_START_INTERRUPT_REGION
      const double rhsY = m_rhs->y(i)[0];
      const double rhsE = m_rhs->e(i)[0];
      if (propagateSpectraMask(lhsSpectrumInfo, rhsSpectrumInfo, i, *m_out, outSpectrumInfo)) {
        performEventBinaryOperation(m_eout->getSpectrum(i), rhsY, rhsE);
      }
      m_progress->report(this->name());
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  } else {
    // ---- Histogram Output -----
    PARALLEL_FOR_IF(Kernel::threadSafe(*m_lhs, *m_rhs, *m_out))
    for (int64_t i = 0; i < numHists; ++i) {
      PARALLEL_START_INTERRUPT_REGION
      const double rhsY = m_rhs->y(i)[0];
      const double rhsE = m_rhs->e(i)[0];

      m_out->setSharedX(i, m_lhs->sharedX(i));
      if (propagateSpectraMask(lhsSpectrumInfo, rhsSpectrumInfo, i, *m_out, outSpectrumInfo)) {
        // Get reference to output vectors here to break any sharing outside the
        // function call below
        // where the order of argument evaluation is not guaranteed (if it's
        // L->R there would be a data race)
        HistogramData::HistogramY &outY = m_out->mutableY(i);
        HistogramData::HistogramE &outE = m_out->mutableE(i);
        performBinaryOperation(m_lhs->histogram(i), rhsY, rhsE, outY, outE);
      }
      m_progress->report(this->name());
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  }
}

/** Called when the m_rhs operand is a single spectrum.
 *  Loops over the lhs workspace calling the abstract binary operation function.
 */
void BinaryOperation::doSingleSpectrum() {

  // Propagate any masking first or it could mess up the numbers
  // TODO: Check if this works for event workspaces...
  propagateBinMasks(m_rhs, m_out);

  if (m_eout) {
    // ----------- The output is an EventWorkspace -------------

    if (m_erhs) {
      // -------- The rhs is ALSO an EventWorkspace --------

      // Pull out the single eventList on the right
      const EventList &rhs_spectrum = m_erhs->getSpectrum(0);

      // Now loop over the spectra of the left hand side calling the virtual
      // function
      const int64_t numHists = m_lhs->getNumberHistograms();
      PARALLEL_FOR_IF(Kernel::threadSafe(*m_lhs, *m_rhs, *m_out))
      for (int64_t i = 0; i < numHists; ++i) {
        PARALLEL_START_INTERRUPT_REGION
        // Perform the operation on the event list on the output (== lhs)
        performEventBinaryOperation(m_eout->getSpectrum(i), rhs_spectrum);
        m_progress->report(this->name());
        PARALLEL_END_INTERRUPT_REGION
      }
      PARALLEL_CHECK_INTERRUPT_REGION
    } else {
      // -------- The rhs is a histogram ---------
      // Pull m_out the m_rhs spectrum
      const MantidVec &rhsX = m_rhs->readX(0);
      const MantidVec &rhsY = m_rhs->readY(0);
      const MantidVec &rhsE = m_rhs->readE(0);

      // Now loop over the spectra of the left hand side calling the virtual
      // function
      const int64_t numHists = m_lhs->getNumberHistograms();

      PARALLEL_FOR_IF(Kernel::threadSafe(*m_lhs, *m_rhs, *m_out))
      for (int64_t i = 0; i < numHists; ++i) {
        PARALLEL_START_INTERRUPT_REGION
        // Perform the operation on the event list on the output (== lhs)
        performEventBinaryOperation(m_eout->getSpectrum(i), rhsX, rhsY, rhsE);
        m_progress->report(this->name());
        PARALLEL_END_INTERRUPT_REGION
      }
      PARALLEL_CHECK_INTERRUPT_REGION
    }

  } else {
    // -------- The output is a histogram ----------
    // (inputs can be EventWorkspaces, but their histogram representation
    //  will be used instead)

    // Pull m_out the m_rhs spectrum
    const auto rhs = m_rhs->histogram(0);

    // Now loop over the spectra of the left hand side calling the virtual
    // function
    const int64_t numHists = m_lhs->getNumberHistograms();

    PARALLEL_FOR_IF(Kernel::threadSafe(*m_lhs, *m_rhs, *m_out))
    for (int64_t i = 0; i < numHists; ++i) {
      PARALLEL_START_INTERRUPT_REGION
      m_out->setSharedX(i, m_lhs->sharedX(i));
      // Get reference to output vectors here to break any sharing outside the
      // function call below
      // where the order of argument evaluation is not guaranteed (if it's L->R
      // there would be a data race)
      HistogramData::HistogramY &outY = m_out->mutableY(i);
      HistogramData::HistogramE &outE = m_out->mutableE(i);
      performBinaryOperation(m_lhs->histogram(i), rhs, outY, outE);
      m_progress->report(this->name());
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  }
}

/** Called when the two workspaces are the same size.
 *  Loops over the workspaces extracting the appropriate spectra and calling the
 *abstract binary operation function.
 *
 *  @param mismatchedSpectra :: allow the # of spectra to not be the same. Will
 *use the
 *      detector IDs to find the corresponding spectrum on RHS
 */
void BinaryOperation::do2D(bool mismatchedSpectra) {
  BinaryOperationTable_sptr table;
  if (mismatchedSpectra) {
    table = BinaryOperation::buildBinaryOperationTable(m_lhs, m_rhs);
  }

  // Propagate any masking first or it could mess up the numbers
  // TODO: Check if this works for event workspaces...
  propagateBinMasks(m_rhs, m_out);

  auto &outSpectrumInfo = m_out->mutableSpectrumInfo();
  auto &lhsSpectrumInfo = m_lhs->spectrumInfo();
  auto &rhsSpectrumInfo = m_rhs->spectrumInfo();

  if (m_eout) {
    // ----------- The output is an EventWorkspace -------------

    if (m_erhs && !m_useHistogramForRhsEventWorkspace) {
      // ------------ The rhs is ALSO an EventWorkspace ---------------
      // Now loop over the spectra of each one calling the virtual function
      const int64_t numHists = m_lhs->getNumberHistograms();
      PARALLEL_FOR_IF(Kernel::threadSafe(*m_lhs, *m_rhs, *m_out))
      for (int64_t i = 0; i < numHists; ++i) {
        PARALLEL_START_INTERRUPT_REGION
        m_progress->report(this->name());

        int64_t rhs_wi = i;
        if (mismatchedSpectra && table) {
          rhs_wi = (*table)[i];
          if (rhs_wi < 0)
            continue;
        } else {
          // Check for masking except when mismatched sizes
          if (!propagateSpectraMask(lhsSpectrumInfo, rhsSpectrumInfo, i, *m_out, outSpectrumInfo))
            continue;
        }
        // Reach here? Do the division
        // Perform the operation on the event list on the output (== lhs)
        performEventBinaryOperation(m_eout->getSpectrum(i), m_erhs->getSpectrum(rhs_wi));

        // Free up memory on the RHS if that is possible
        if (m_ClearRHSWorkspace)
          const_cast<EventList &>(m_erhs->getSpectrum(rhs_wi)).clear();
        PARALLEL_END_INTERRUPT_REGION
      }
      PARALLEL_CHECK_INTERRUPT_REGION
    } else {
      // -------- The rhs is a histogram, or we want to use the histogram
      // representation of it ---------

      // Now loop over the spectra of each one calling the virtual function
      const int64_t numHists = m_lhs->getNumberHistograms();

      PARALLEL_FOR_IF(Kernel::threadSafe(*m_lhs, *m_rhs, *m_out))
      for (int64_t i = 0; i < numHists; ++i) {
        PARALLEL_START_INTERRUPT_REGION
        m_progress->report(this->name());
        int64_t rhs_wi = i;
        if (mismatchedSpectra && table) {
          rhs_wi = (*table)[i];
          if (rhs_wi < 0)
            continue;
        } else {
          // Check for masking except when mismatched sizes
          if (!propagateSpectraMask(lhsSpectrumInfo, rhsSpectrumInfo, i, *m_out, outSpectrumInfo))
            continue;
        }

        // Reach here? Do the division
        performEventBinaryOperation(m_eout->getSpectrum(i), m_rhs->readX(rhs_wi), m_rhs->readY(rhs_wi),
                                    m_rhs->readE(rhs_wi));

        // Free up memory on the RHS if that is possible
        if (m_ClearRHSWorkspace)
          const_cast<EventList &>(m_erhs->getSpectrum(rhs_wi)).clear();

        PARALLEL_END_INTERRUPT_REGION
      }
      PARALLEL_CHECK_INTERRUPT_REGION
    }

  } else {
    // -------- The output is a histogram ----------
    // (inputs can be EventWorkspaces, but their histogram representation
    //  will be used instead)

    // Now loop over the spectra of each one calling the virtual function
    const int64_t numHists = m_lhs->getNumberHistograms();

    PARALLEL_FOR_IF(Kernel::threadSafe(*m_lhs, *m_rhs, *m_out))
    for (int64_t i = 0; i < numHists; ++i) {
      PARALLEL_START_INTERRUPT_REGION
      m_progress->report(this->name());
      m_out->setSharedX(i, m_lhs->sharedX(i));
      int64_t rhs_wi = i;
      if (mismatchedSpectra && table) {
        rhs_wi = (*table)[i];
        if (rhs_wi < 0)
          continue;
      } else {
        // Check for masking except when mismatched sizes
        if (!propagateSpectraMask(lhsSpectrumInfo, rhsSpectrumInfo, i, *m_out, outSpectrumInfo))
          continue;
      }
      // Reach here? Do the division
      // Get reference to output vectors here to break any sharing outside the
      // function call below
      // where the order of argument evaluation is not guaranteed (if it's L->R
      // there would be a data race)
      HistogramData::HistogramY &outY = m_out->mutableY(i);
      HistogramData::HistogramE &outE = m_out->mutableE(i);
      performBinaryOperation(m_lhs->histogram(i), m_rhs->histogram(rhs_wi), outY, outE);

      // Free up memory on the RHS if that is possible
      if (m_ClearRHSWorkspace)
        const_cast<EventList &>(m_erhs->getSpectrum(rhs_wi)).clear();

      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  }
  // Make sure we don't use outdated MRU
  if (m_ClearRHSWorkspace)
    m_erhs->clearMRU();
}

/** Copies any bin masking from the smaller/rhs input workspace to the output.
 *  Masks on the other input workspace are copied automatically by the workspace
 * factory.
 *  @param rhs :: The workspace which is the right hand operand
 *  @param out :: The result workspace
 */
void BinaryOperation::propagateBinMasks(const API::MatrixWorkspace_const_sptr &rhs,
                                        const API::MatrixWorkspace_sptr &out) {
  const int64_t outHists = out->getNumberHistograms();
  const int64_t rhsHists = rhs->getNumberHistograms();
  for (int64_t i = 0; i < outHists; ++i) {
    // Copy over masks from the rhs, if any exist.
    // If rhs is single spectrum, copy masks from that to all spectra in the
    // output.
    if (rhs->hasMaskedBins((rhsHists == 1) ? 0 : i)) {
      const MatrixWorkspace::MaskList &masks = rhs->maskedBins((rhsHists == 1) ? 0 : i);
      MatrixWorkspace::MaskList::const_iterator it;
      for (it = masks.begin(); it != masks.end(); ++it) {
        out->flagMasked(i, it->first, it->second);
      }
    }
  }
}

// ------- Default implementations of Event binary operations --------

/**
 * Carries out the binary operation IN-PLACE on a single EventList,
 * with another EventList as the right-hand operand.
 * The event lists simply get appended.
 *
 *  @param lhs :: Reference to the EventList that will be modified in place.
 *  @param rhs :: Const reference to the EventList on the right hand side.
 */
void BinaryOperation::performEventBinaryOperation(DataObjects::EventList &lhs, const DataObjects::EventList &rhs) {
  UNUSED_ARG(lhs);
  UNUSED_ARG(rhs);
  throw Exception::NotImplementedError("BinaryOperation::performEventBinaryOperation() not implemented.");
}

/**
 * Carries out the binary operation IN-PLACE on a single EventList,
 * with another (histogrammed) spectrum as the right-hand operand.
 *
 *  @param lhs :: Reference to the EventList that will be modified in place.
 *  @param rhsX :: Rhs X bin boundaries
 *  @param rhsY :: Rhs data values
 *  @param rhsE :: Rhs error values
 */
void BinaryOperation::performEventBinaryOperation(DataObjects::EventList &lhs, const MantidVec &rhsX,
                                                  const MantidVec &rhsY, const MantidVec &rhsE) {
  UNUSED_ARG(lhs);
  UNUSED_ARG(rhsX);
  UNUSED_ARG(rhsY);
  UNUSED_ARG(rhsE);
  throw Exception::NotImplementedError("BinaryOperation::performEventBinaryOperation() not implemented.");
}

/**
 * Carries out the binary operation IN-PLACE on a single EventList,
 * with a single (double) value as the right-hand operand
 *
 *  @param lhs :: Reference to the EventList that will be modified in place.
 *  @param rhsY :: The rhs data value
 *  @param rhsE :: The rhs error value
 */
void BinaryOperation::performEventBinaryOperation(DataObjects::EventList &lhs, const double &rhsY, const double &rhsE) {
  UNUSED_ARG(lhs);
  UNUSED_ARG(rhsY);
  UNUSED_ARG(rhsE);
  throw Exception::NotImplementedError("BinaryOperation::performEventBinaryOperation() not implemented.");
}

/**
 * Get the type of operand from a workspace
 * @param ws :: workspace to check
 * @return OperandType describing what type of workspace it will be operated as.
 */
OperandType BinaryOperation::getOperandType(const API::MatrixWorkspace_const_sptr &ws) {
  // An event workspace?
  EventWorkspace_const_sptr ews = std::dynamic_pointer_cast<const EventWorkspace>(ws);
  if (ews)
    return eEventList;

  // If the workspace has no axes, then it is a WorkspaceSingleValue
  if (!ws->axes())
    return eNumber;

  // TODO: Check if it is a single-colum one, then
  //  return Number;

  // Otherwise, we take it as a histogram (workspace 2D)
  return eHistogram;
}

/** Check what operation will be needed in order to apply the operation
 * to these two types of workspaces. This function must be overridden
 * and checked against all 9 possible combinations.
 *
 * Must set: m_matchXSize, m_flipSides, m_keepEventWorkspace
 */
void BinaryOperation::checkRequirements() {

  // In general, the X sizes have to match.
  //  (only for some EventWorkspace cases does it NOT matter...)
  m_matchXSize = true;

  // Operations are not always commutative! Don't flip sides.
  m_flipSides = false;

  // And in general, EventWorkspaces get turned to Workspace2D
  m_keepEventWorkspace = false;

  // This will be set to true for Divide/Multiply
  m_useHistogramForRhsEventWorkspace = false;
}

/** Build up an BinaryOperationTable for performing a binary operation
 * e.g. lhs = (lhs + rhs)
 * where the spectra in rhs are to go into lhs.
 * This function looks to match the detector IDs in rhs to those in the lhs.
 *
 * @param lhs :: matrix workspace in which the operation is being done.
 * @param rhs :: matrix workspace on the right hand side of the operand
 * @return map from detector ID to workspace index for the RHS workspace.
 *        NULL if there is not a 1:1 mapping from detector ID to workspace index
 *(e.g more than one detector per pixel).
 */
BinaryOperation::BinaryOperationTable_sptr
BinaryOperation::buildBinaryOperationTable(const MatrixWorkspace_const_sptr &lhs,
                                           const MatrixWorkspace_const_sptr &rhs) {
  // An addition table is a list of pairs:
  //  First int = workspace index in the EW being added
  //  Second int = workspace index to which it will be added in the OUTPUT EW.
  //  -1 if it should add a new entry at the end.
  auto table = std::make_shared<BinaryOperationTable>();

  auto rhs_nhist = static_cast<int>(rhs->getNumberHistograms());
  auto lhs_nhist = static_cast<int>(lhs->getNumberHistograms());

  // Initialize the table; filled with -1 meaning no match
  table->resize(lhs_nhist, -1);

  const detid2index_map rhs_det_to_wi = rhs->getDetectorIDToWorkspaceIndexMap();

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int lhsWI = 0; lhsWI < lhs_nhist; lhsWI++) {
    bool done = false;

    // List of detectors on lhs side
    const auto &lhsDets = lhs->getSpectrum(lhsWI).getDetectorIDs();

    // ----------------- Matching Workspace Indices and Detector IDs
    // --------------------------------------
    // First off, try to match the workspace indices. Most times, this will be
    // ok right away.
    int64_t rhsWI = lhsWI;
    if (rhsWI < rhs_nhist) // don't go out of bounds
    {
      // Get the detector IDs at that workspace index.
      const auto &rhsDets = rhs->getSpectrum(rhsWI).getDetectorIDs();

      // Checks that lhsDets is a subset of rhsDets
      if (std::includes(rhsDets.begin(), rhsDets.end(), lhsDets.begin(), lhsDets.end())) {
        // We found the workspace index right away. No need to keep looking
        (*table)[lhsWI] = rhsWI;
        done = true;
      }
    }

    // ----------------- Scrambled Detector IDs with one Detector per Spectrum
    // --------------------------------------
    if (!done && (lhsDets.size() == 1)) {
      // Didn't find it. Try to use the RHS map.

      // First, we have to get the (single) detector ID of the LHS
      auto lhsDets_it = lhsDets.cbegin();
      detid_t lhs_detector_ID = *lhsDets_it;

      // Now we use the RHS map to find it. This only works if both the lhs and
      // rhs have 1 detector per pixel
      auto map_it = rhs_det_to_wi.find(lhs_detector_ID);
      if (map_it != rhs_det_to_wi.end()) {
        rhsWI = map_it->second; // This is the workspace index in the RHS that
                                // matched lhs_detector_ID
      } else {
        // Did not find it!
        rhsWI = -1; // Marker to mean its not in the LHS.

        //            std::ostringstream mess;
        //            mess << "BinaryOperation: cannot find a RHS spectrum that
        //            contains the detectors in LHS workspace index " << lhsWI
        //            << "\n";
        //            throw std::runtime_error(mess.str());
      }
      (*table)[lhsWI] = rhsWI;
      done = true; // Great, we did it.
    }

    // ----------------- LHS detectors are subset of RHS, which are Grouped
    // --------------------------------------
    if (!done) {

      // Didn't find it? Now we need to iterate through the output workspace to
      //  match the detector ID.
      // NOTE: This can be SUPER SLOW!
      for (rhsWI = 0; rhsWI < static_cast<int64_t>(rhs_nhist); rhsWI++) {
        const auto &rhsDets = rhs->getSpectrum(rhsWI).getDetectorIDs();

        // Checks that lhsDets is a subset of rhsDets
        if (std::includes(rhsDets.begin(), rhsDets.end(), lhsDets.begin(), lhsDets.end())) {
          // This one is right. Now we can stop looking.
          (*table)[lhsWI] = rhsWI;
          done = true;
          continue;
        }
      }
    }

    // ------- Still nothing ! -----------
    if (!done) {
      (*table)[lhsWI] = -1;

      //          std::ostringstream mess;
      //          mess << "BinaryOperation: cannot find a RHS spectrum that
      //          contains the detectors in LHS workspace index " << lhsWI <<
      //          "\n";
      //          throw std::runtime_error(mess.str());
    }
  }

  return table;
}
} // namespace Mantid::Algorithms
