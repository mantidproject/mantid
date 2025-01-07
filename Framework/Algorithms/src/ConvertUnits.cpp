// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"

#include <numeric>

namespace Mantid::Algorithms {

// Register with the algorithm factory
DECLARE_ALGORITHM(ConvertUnits)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;

/// Initialisation method
void ConvertUnits::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>();
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input, wsValidator),
      "Name of the input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace, can be the same as the input");

  // Extract the current contents of the UnitFactory to be the allowed values of
  // the Target property
  declareProperty("Target", "", std::make_shared<StringListValidator>(UnitFactory::Instance().getConvertibleUnits()),
                  "The name of the units to convert to (must be one of those "
                  "registered in\n"
                  "the Unit Factory)");
  std::vector<std::string> propOptions{"Elastic", "Direct", "Indirect"};
  declareProperty("EMode", "Elastic", std::make_shared<StringListValidator>(propOptions),
                  "The energy mode (default: elastic)");
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("EFixed", EMPTY_DBL(), mustBePositive,
                  "Value of fixed energy in meV : EI (EMode='Direct') or EF "
                  "(EMode='Indirect') . Must be\n"
                  "set if the target unit requires it (e.g. DeltaE)");

  declareProperty("AlignBins", false,
                  "If true (default is false), rebins after conversion to "
                  "ensure that all spectra in the output workspace\n"
                  "have identical bin boundaries. This option is not "
                  "recommended (see "
                  "http://docs.mantidproject.org/algorithms/ConvertUnits).");

  declareProperty("ConvertFromPointData", true,
                  "When checked, if the Input Workspace contains Points\n"
                  "the algorithm ConvertToHistogram will be run to convert\n"
                  "the Points to Bins. The Output Workspace will contains Bins.");
}

/** Executes the algorithm
 *  @throw std::runtime_error :: Thrown in the following cases:
 *   - If the input workspace has not had its unit set
 *   - If the input workspace contains Points, but ConvertFromPointData
 *       has not been enabled.
 *   - If ConvertFromPointData has been enabled, but the conversion to Bins
 *       or back to Points fails.
 *  @throw InstrumentDefinitionError If unable to calculate source-sample
 * distance
 */
void ConvertUnits::exec() {
  // Get the workspaces
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const bool acceptPointData = getProperty("ConvertFromPointData");
  bool workspaceWasConverted = false;

  // we can do that before anything else, because it doesn't setup any blocksize, which is the one that changes with
  // conversion
  this->setupMemberVariables(inputWS);

  // Check that the input workspace doesn't already have the desired unit.
  if (m_inputUnit->unitID() == m_outputUnit->unitID()) {
    const std::string outputWSName = getPropertyValue("OutputWorkspace");
    const std::string inputWSName = getPropertyValue("InputWorkspace");
    if (outputWSName == inputWSName) {
      // If it does, just set the output workspace to point to the input one and be done
      g_log.information() << "Input workspace already has target unit (" << m_outputUnit->unitID()
                          << "), so just pointing the output workspace property to the input workspace.\n";
      setProperty("OutputWorkspace", std::const_pointer_cast<MatrixWorkspace>(inputWS));
      return;
    } else {
      // Clone the workspace.
      auto duplicate = createChildAlgorithm("CloneWorkspace", 0.0, 0.6);
      duplicate->initialize();
      duplicate->setProperty("InputWorkspace", inputWS);
      duplicate->execute();
      Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
      auto outputWs = std::dynamic_pointer_cast<MatrixWorkspace>(temp);
      setProperty("OutputWorkspace", outputWs);
      return;
    }
  }

  // Holder for the correctWS, because if we're converting from
  // PointData a new workspace is created
  MatrixWorkspace_sptr correctWS;
  if (!inputWS->isHistogramData()) {
    if (acceptPointData) {
      workspaceWasConverted = true;
      g_log.information("ConvertFromPointData is checked. Running ConvertToHistogram\n");
      // not histogram data
      // ConvertToHistogram
      auto convToHist = createChildAlgorithm("ConvertToHistogram");
      convToHist->setProperty("InputWorkspace", inputWS);
      convToHist->execute();
      MatrixWorkspace_sptr temp = convToHist->getProperty("OutputWorkspace");
      correctWS = std::dynamic_pointer_cast<MatrixWorkspace>(temp);

      if (!correctWS->isHistogramData()) {
        throw std::runtime_error("Failed to convert workspace from Points to Bins");
      }
    } else {
      throw std::runtime_error("Workspace contains points, you can either run ConvertToHistogram on it, or set "
                               "ConvertFromPointData to enabled");
    }
  } else {
    correctWS = inputWS;
  }

  MatrixWorkspace_sptr outputWS = executeUnitConversion(correctWS);

  // If InputWorkspace contained point data, convert back
  if (workspaceWasConverted) {
    g_log.information("ConvertUnits is completed. Running ConvertToPointData.\n");
    auto convtoPoints = createChildAlgorithm("ConvertToPointData");
    convtoPoints->setProperty("InputWorkspace", outputWS);
    convtoPoints->execute();
    MatrixWorkspace_sptr temp = convtoPoints->getProperty("OutputWorkspace");
    outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(temp);

    if (outputWS->isHistogramData()) {
      throw std::runtime_error("Failed to convert workspace from Bins to Points");
    }
  }

  // Point the output property to the right place.
  // Do right at end (workspace could could change in removeUnphysicalBins or alignBins methods)
  setProperty("OutputWorkspace", outputWS);
}

/**Executes the main part of the algorithm that handles the conversion of the
 * units
 * @param inputWS :: the input workspace that will be converted
 * @throw std::runtime_error :: If the workspace has invalid X axis binning
 * @return A pointer to a MatrixWorkspace_sptr that contains the converted units
 */
MatrixWorkspace_sptr ConvertUnits::executeUnitConversion(const API::MatrixWorkspace_sptr &inputWS) {

  // A WS holding BinEdges cannot have less than 2 values, as a bin has
  // 2 edges, having less than 2 values would mean that the WS contains Points
  if (inputWS->x(0).size() < 2) {
    std::stringstream msg;
    msg << "Input workspace has invalid X axis binning parameters. Should have at least 2 values. Found "
        << inputWS->x(0).size() << ".";
    throw std::runtime_error(msg.str());
  }
  if (inputWS->x(0).front() > inputWS->x(0).back() ||
      inputWS->x(m_numberOfSpectra / 2).front() > inputWS->x(m_numberOfSpectra / 2).back())
    throw std::runtime_error("Input workspace has invalid X axis binning "
                             "parameters. X values should be increasing.");

  MatrixWorkspace_sptr outputWS;
  // Check whether there is a quick conversion available
  double factor, power;
  if (m_inputUnit->quickConversion(*m_outputUnit, factor, power))
  // If test fails, could also check whether a quick conversion in the opposite direction has been entered
  {
    outputWS = this->convertQuickly(inputWS, factor, power);
  } else {
    outputWS = this->convertViaTOF(m_inputUnit, inputWS);
  }

  // If the units conversion has flipped the ascending direction of X, reverse all the vectors
  if (!outputWS->x(0).empty() &&
      (outputWS->x(0).front() > outputWS->x(0).back() ||
       outputWS->x(m_numberOfSpectra / 2).front() > outputWS->x(m_numberOfSpectra / 2).back())) {
    this->reverse(outputWS);
  }

  // Need to lop bins off if converting to energy transfer.
  // Don't do for EventWorkspaces, where you can easily rebin to recover the situation without losing information
  /* This is an ugly test - could be made more general by testing for DBL_MAX
  values at the ends of all spectra, but that would be less efficient */
  if (m_outputUnit->unitID().find("Delta") == 0 && !m_inputEvents)
    outputWS = this->removeUnphysicalBins(outputWS);

  // Rebin the data to common bins if requested, and if necessary
  const bool doAlignBins = getProperty("AlignBins");
  if (doAlignBins && !outputWS->isCommonBins())
    outputWS = this->alignBins(outputWS);

  // If appropriate, put back the bin width division into Y/E.
  if (m_distribution && !m_inputEvents) { // Never do this for event workspaces
    this->putBackBinWidth(outputWS);
  }

  return outputWS;
}
/** Initialise the member variables
 *  @param inputWS The input workspace
 */
void ConvertUnits::setupMemberVariables(const API::MatrixWorkspace_const_sptr &inputWS) {
  m_numberOfSpectra = inputWS->getNumberHistograms();
  // In the context of this algorithm, we treat things as a distribution if
  // the flag is set AND the data are not dimensionless
  m_distribution = inputWS->isDistribution() && !inputWS->YUnit().empty();
  // Check if its an event workspace
  m_inputEvents = (std::dynamic_pointer_cast<const EventWorkspace>(inputWS) != nullptr);

  m_inputUnit = inputWS->getAxis(0)->unit();
  const std::string targetUnit = getPropertyValue("Target");
  m_outputUnit = UnitFactory::Instance().create(targetUnit);
}

/** Create an output workspace of the appropriate (histogram or event) type
 * and
 * copy over the data
 *  @param inputWS The input workspace
 */
API::MatrixWorkspace_sptr ConvertUnits::setupOutputWorkspace(const API::MatrixWorkspace_const_sptr &inputWS) {
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  // If input and output workspaces are NOT the same, create a new workspace for the output
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
  }

  if (!m_inputEvents && m_distribution) {
    // Loop over the histograms (detector spectra)
    Progress prog(this, 0.0, 0.2, m_numberOfSpectra);
    PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
    for (int64_t i = 0; i < static_cast<int64_t>(m_numberOfSpectra); ++i) {
      PARALLEL_START_INTERRUPT_REGION
      // Take the bin width dependency out of the Y & E data
      const auto &X = outputWS->x(i);
      auto &Y = outputWS->mutableY(i);
      auto &E = outputWS->mutableE(i);
      for (size_t j = 0; j < Y.size(); ++j) {
        const double width = std::abs(X[j + 1] - X[j]);
        Y[j] *= width;
        E[j] *= width;
      }

      prog.report("Convert to " + m_outputUnit->unitID());
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  }

  // Set the final unit that our output workspace will have
  outputWS->getAxis(0)->unit() = m_outputUnit;
  storeEModeOnWorkspace(outputWS);

  return outputWS;
}

/** Stores the emode in the provided workspace
 *  @param outputWS The workspace
 */
void ConvertUnits::storeEModeOnWorkspace(API::MatrixWorkspace_sptr outputWS) {
  // Store the emode
  const bool overwrite(true);
  outputWS->mutableRun().addProperty("deltaE-mode", getPropertyValue("EMode"), overwrite);
}

/** Convert the workspace units according to a simple output = a * (input^b)
 * relationship
 *  @param inputWS :: the input workspace
 *  @param factor :: the conversion factor a to apply
 *  @param power :: the Power b to apply to the conversion
 *  @returns A shared pointer to the output workspace
 */
MatrixWorkspace_sptr ConvertUnits::convertQuickly(const API::MatrixWorkspace_const_sptr &inputWS, const double &factor,
                                                  const double &power) {
  Progress prog(this, 0.2, 1.0, m_numberOfSpectra);
  auto numberOfSpectra_i = static_cast<int64_t>(m_numberOfSpectra); // cast to make openmp happy
                                                                    // create the output workspace
  MatrixWorkspace_sptr outputWS = this->setupOutputWorkspace(inputWS);
  // See if the workspace has common bins - if so the X vector can be common
  const bool commonBoundaries = inputWS->isCommonBins();
  if (commonBoundaries) {
    // Calculate the new (common) X values
    std::transform(outputWS->mutableX(0).cbegin(), outputWS->mutableX(0).cend(), outputWS->mutableX(0).begin(),
                   [&](const auto &x) { return factor * std::pow(x, power); });

    auto xVals = outputWS->sharedX(0);

    PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
    for (int64_t j = 1; j < numberOfSpectra_i; ++j) {
      PARALLEL_START_INTERRUPT_REGION
      outputWS->setX(j, xVals);
      prog.report("Convert to " + m_outputUnit->unitID());
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
    if (!m_inputEvents) // if in event mode the work is done
      return outputWS;
  }

  EventWorkspace_sptr eventWS = std::dynamic_pointer_cast<EventWorkspace>(outputWS);
  assert(static_cast<bool>(eventWS) == m_inputEvents); // Sanity check

  // If we get to here then the bins weren't aligned and each spectrum is
  // unique
  // Loop over the histograms (detector spectra)
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int64_t k = 0; k < numberOfSpectra_i; ++k) {
    PARALLEL_START_INTERRUPT_REGION
    if (!commonBoundaries) {
      std::transform(outputWS->mutableX(k).cbegin(), outputWS->mutableX(k).cend(), outputWS->mutableX(k).begin(),
                     [&](const auto &x) { return factor * std::pow(x, power); });
    }
    // Convert the events themselves if necessary.
    if (m_inputEvents) {
      eventWS->getSpectrum(k).convertUnitsQuickly(factor, power);
    }
    prog.report("Convert to " + m_outputUnit->unitID());
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  if (m_inputEvents)
    eventWS->clearMRU();

  return outputWS;
}

/** Convert the workspace units using TOF as an intermediate step in the
 * conversion
 * @param fromUnit :: The unit of the input workspace
 * @param inputWS :: The input workspace
 * @returns A shared pointer to the output workspace
 */
MatrixWorkspace_sptr ConvertUnits::convertViaTOF(Kernel::Unit_const_sptr fromUnit,
                                                 API::MatrixWorkspace_const_sptr inputWS) {
  using namespace Geometry;

  Progress prog(this, 0.2, 1.0, m_numberOfSpectra);
  auto numberOfSpectra_i = static_cast<int64_t>(m_numberOfSpectra); // cast to make openmp happy

  Kernel::Unit_const_sptr outputUnit = m_outputUnit;

  const auto &spectrumInfo = inputWS->spectrumInfo();
  double l1 = spectrumInfo.l1();
  g_log.debug() << "Source-sample distance: " << l1 << '\n';

  int failedDetectorCount = 0;

  /// @todo No implementation for any of these in the geometry yet so using
  /// properties
  const std::string emodeStr = getProperty("EMode");
  DeltaEMode::Type emode = DeltaEMode::fromString(emodeStr);

  // Not doing anything with the Y vector in to/fromTOF yet, so just pass
  // empty
  // vector
  std::vector<double> emptyVec;
  double efixedProp = getProperty("Efixed");
  if (efixedProp == EMPTY_DBL() && emode == DeltaEMode::Type::Direct) {
    try {
      efixedProp = inputWS->getEFixedGivenEMode(nullptr, emode);
    } catch (std::runtime_error &) {
    }
  }

  std::vector<std::string> parameters = inputWS->getInstrument()->getStringParameter("show-signed-theta");
  bool signedTheta = (!parameters.empty()) && find(parameters.begin(), parameters.end(), "Always") != parameters.end();

  auto checkFromUnit = std::unique_ptr<Unit>(fromUnit->clone());
  auto checkOutputUnit = std::unique_ptr<Unit>(outputUnit->clone());

  // Perform Sanity Validation before creating workspace
  const double checkdelta = 0.0;
  UnitParametersMap upmap = {{UnitParams::delta, checkdelta}};
  if (efixedProp != EMPTY_DBL()) {
    upmap[UnitParams::efixed] = efixedProp;
  }
  size_t checkIndex = 0;
  spectrumInfo.getDetectorValues(*fromUnit, *outputUnit, emode, signedTheta, checkIndex, upmap);
  // copy the X values for the check
  auto checkXValues = inputWS->readX(checkIndex);
  try {
    // Convert the input unit to time-of-flight
    checkFromUnit->toTOF(checkXValues, emptyVec, l1, emode, upmap);
    // Convert from time-of-flight to the desired unit
    checkOutputUnit->fromTOF(checkXValues, emptyVec, l1, emode, upmap);
  } catch (std::runtime_error &) { // if it's a detector specific problem then ignore
  }

  // create the output workspace
  MatrixWorkspace_sptr outputWS = this->setupOutputWorkspace(inputWS);
  EventWorkspace_sptr eventWS = std::dynamic_pointer_cast<EventWorkspace>(outputWS);
  assert(static_cast<bool>(eventWS) == m_inputEvents); // Sanity check

  auto &outSpectrumInfo = outputWS->mutableSpectrumInfo();
  // Loop over the histograms (detector spectra)
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int64_t i = 0; i < numberOfSpectra_i; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    double efixed = efixedProp;

    // Now get the detector object for this histogram
    /// @todo Don't yet consider hold-off (delta)
    const double delta = 0.0;

    auto localFromUnit = std::unique_ptr<Unit>(fromUnit->clone());
    auto localOutputUnit = std::unique_ptr<Unit>(outputUnit->clone());

    // TODO toTOF and fromTOF need to be reimplemented outside of kernel
    UnitParametersMap pmap = {{UnitParams::delta, delta}};
    if (efixedProp != EMPTY_DBL()) {
      pmap[UnitParams::efixed] = efixed;
    }
    outSpectrumInfo.getDetectorValues(*fromUnit, *outputUnit, emode, signedTheta, i, pmap);
    try {
      localFromUnit->toTOF(outputWS->dataX(i), emptyVec, l1, emode, pmap);
      // Convert from time-of-flight to the desired unit
      localOutputUnit->fromTOF(outputWS->dataX(i), emptyVec, l1, emode, pmap);

      // EventWorkspace part, modifying the EventLists.
      if (m_inputEvents) {
        eventWS->getSpectrum(i).convertUnitsViaTof(localFromUnit.get(), localOutputUnit.get());
      }
    } catch (std::runtime_error &) {
      // Get to here if exception thrown in unit conversion eg when calculating
      // distance to detector
      failedDetectorCount++;
      // Since you usually (always?) get to here when there's no attached
      // detectors, this call is
      // the same as just zeroing out the data (calling clearData on the
      // spectrum)
      outputWS->getSpectrum(i).clearData();
      if (outSpectrumInfo.hasDetectors(i))
        outSpectrumInfo.setMasked(i, true);
    }

    prog.report("Convert to " + m_outputUnit->unitID());
    PARALLEL_END_INTERRUPT_REGION
  } // loop over spectra
  PARALLEL_CHECK_INTERRUPT_REGION

  if (failedDetectorCount != 0) {
    g_log.warning() << "Unable to calculate sample-detector distance for " << failedDetectorCount
                    << " spectra. Masking spectrum.\n";
  }
  if (m_inputEvents)
    eventWS->clearMRU();

  if (emode == 1) {
    //... direct efixed gather
    if (efixedProp != EMPTY_DBL()) {
      // set the Ei value in the run parameters
      API::Run &run = outputWS->mutableRun();
      run.addProperty<double>("Ei", efixedProp, true);
    }
  }

  return outputWS;
}

/// Calls Rebin as a Child Algorithm to align the bins
API::MatrixWorkspace_sptr ConvertUnits::alignBins(const API::MatrixWorkspace_sptr &workspace) {
  // Create a Rebin child algorithm
  auto childAlg = createChildAlgorithm("Rebin");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", workspace);
  // Next line for EventWorkspaces - needed for as long as in/out set same
  // keeps
  // as events.
  childAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", workspace);
  childAlg->setProperty<std::vector<double>>("Params", this->calculateRebinParams(workspace));
  childAlg->executeAsChildAlg();
  return childAlg->getProperty("OutputWorkspace");
}

/// The Rebin parameters should cover the full range of the converted unit,
/// with the same number of bins
const std::vector<double> ConvertUnits::calculateRebinParams(const API::MatrixWorkspace_const_sptr &workspace) const {
  const auto &spectrumInfo = workspace->spectrumInfo();
  // Need to loop round and find the full range
  double XMin = DBL_MAX, XMax = DBL_MIN;
  const size_t numSpec = workspace->getNumberHistograms();
  for (size_t i = 0; i < numSpec; ++i) {
    if (spectrumInfo.hasDetectors(i) && !spectrumInfo.isMasked(i)) {
      auto &XData = workspace->x(i);
      double xfront = XData.front();
      double xback = XData.back();
      if (std::isfinite(xfront) && std::isfinite(xback)) {
        if (xfront < XMin)
          XMin = xfront;
        if (xback > XMax)
          XMax = xback;
      }
    }
  }
  const double step = (XMax - XMin) / static_cast<double>(workspace->blocksize());

  return {XMin, step, XMax};
}

/** Reverses the workspace if X values are in descending order
 *  @param WS The workspace to operate on
 */
void ConvertUnits::reverse(const API::MatrixWorkspace_sptr &WS) {
  EventWorkspace_sptr eventWS = std::dynamic_pointer_cast<EventWorkspace>(WS);
  auto isInputEvents = static_cast<bool>(eventWS);
  size_t numberOfSpectra = WS->getNumberHistograms();
  if (WS->isCommonBins() && !isInputEvents) {
    auto reverseX = make_cow<HistogramData::HistogramX>(WS->x(0).crbegin(), WS->x(0).crend());
    for (size_t j = 0; j < numberOfSpectra; ++j) {
      WS->setSharedX(j, reverseX);
      std::reverse(WS->dataY(j).begin(), WS->dataY(j).end());
      std::reverse(WS->dataE(j).begin(), WS->dataE(j).end());
      if (j % 100 == 0)
        interruption_point();
    }
  } else {
    // either events or ragged boundaries
    auto numberOfSpectra_i = static_cast<int>(numberOfSpectra);
    PARALLEL_FOR_IF(Kernel::threadSafe(*WS))
    for (int j = 0; j < numberOfSpectra_i; ++j) {
      PARALLEL_START_INTERRUPT_REGION
      if (isInputEvents) {
        eventWS->getSpectrum(j).reverse();
      } else {
        std::reverse(WS->mutableX(j).begin(), WS->mutableX(j).end());
        std::reverse(WS->mutableY(j).begin(), WS->mutableY(j).end());
        std::reverse(WS->mutableE(j).begin(), WS->mutableE(j).end());
      }
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  }
}

/** Unwieldy method which removes bins which lie in a physically inaccessible
 * region.
 *  This presently only occurs in conversions to energy transfer, where the
 * initial
 *  unit conversion sets them to +/-DBL_MAX. This method removes those bins,
 * leading
 *  to a workspace which is smaller than the input one.
 *  As presently implemented, it unfortunately requires testing for and
 * knowledge of
 *  aspects of the particular units conversion instead of keeping all that in
 * the
 *  units class. It could be made more general, but that would be less
 * efficient.
 *  @param workspace :: The workspace after initial unit conversion
 *  @return The workspace after bins have been removed
 */
API::MatrixWorkspace_sptr ConvertUnits::removeUnphysicalBins(const Mantid::API::MatrixWorkspace_const_sptr &workspace) {
  MatrixWorkspace_sptr result;

  const auto &spectrumInfo = workspace->spectrumInfo();
  const size_t numSpec = workspace->getNumberHistograms();
  const std::string emode = getProperty("Emode");
  if (emode == "Direct") {
    // First the easy case of direct instruments, where all spectra will need
    // the
    // same number of bins removed
    // Need to make sure we don't pick a monitor as the 'reference' X spectrum
    // (X0)
    size_t i = 0;
    for (; i < numSpec; ++i) {
      if (spectrumInfo.hasDetectors(i) && !spectrumInfo.isMonitor(i))
        break;
    }
    // Get an X spectrum to search (they're all the same, monitors excepted)
    auto &X0 = workspace->x(i);
    auto start = std::lower_bound(X0.cbegin(), X0.cend(), -1.0e-10 * DBL_MAX);
    if (start == X0.end()) {
      const std::string e("Check the input EFixed: the one given leads to all "
                          "bins being in the physically inaccessible region.");
      g_log.error(e);
      throw std::invalid_argument(e);
    }
    MantidVec::difference_type bins = X0.cend() - start;
    MantidVec::difference_type first = start - X0.cbegin();

    result = create<MatrixWorkspace>(*workspace, BinEdges(bins));

    for (size_t wsIndex = 0; wsIndex < numSpec; ++wsIndex) {
      auto &X = workspace->x(wsIndex);
      auto &Y = workspace->y(wsIndex);
      auto &E = workspace->e(wsIndex);
      result->mutableX(wsIndex).assign(X.begin() + first, X.end());
      result->mutableY(wsIndex).assign(Y.begin() + first, Y.end());
      result->mutableE(wsIndex).assign(E.begin() + first, E.end());
    }
  } else if (emode == "Indirect") {
    // Now the indirect instruments. In this case we could want to keep a
    // different
    // number of bins in each spectrum because, in general L2 is different for
    // each
    // one.
    // Thus, we first need to loop to find largest 'good' range
    std::vector<MantidVec::difference_type> lastBins(numSpec);
    int maxBins = 0;
    for (size_t i = 0; i < numSpec; ++i) {
      const MantidVec &X = workspace->readX(i);
      auto end = std::lower_bound(X.cbegin(), X.cend(), 1.0e-10 * DBL_MAX);
      MantidVec::difference_type bins = end - X.cbegin();
      lastBins[i] = bins;
      if (bins > maxBins)
        maxBins = static_cast<int>(bins);
    }
    g_log.debug() << maxBins << '\n';
    // Now create an output workspace large enough for the longest 'good'
    // range
    result = create<MatrixWorkspace>(*workspace, numSpec, BinEdges(maxBins));
    // Next, loop again copying in the correct range for each spectrum
    for (int64_t j = 0; j < int64_t(numSpec); ++j) {
      auto edges = workspace->binEdges(j);
      auto k = lastBins[j];

      auto &X = result->mutableX(j);
      std::copy(edges.cbegin(), edges.cbegin() + k, X.begin());

      // If the entire X range is not covered, generate fake values.
      if (k < maxBins) {
        std::iota(X.begin() + k, X.end(), workspace->x(j)[k] + 1);
      }

      std::copy(workspace->y(j).cbegin(), workspace->y(j).cbegin() + (k - 1), result->mutableY(j).begin());
      std::copy(workspace->e(j).cbegin(), workspace->e(j).cbegin() + (k - 1), result->mutableE(j).begin());
    }
  }

  return result;
}

/** Divide by the bin width if workspace is a distribution
 *  @param outputWS The workspace to operate on
 */
void ConvertUnits::putBackBinWidth(const API::MatrixWorkspace_sptr &outputWS) {
  for (size_t i = 0; i < m_numberOfSpectra; ++i) {
    const size_t outSize = outputWS->getNumberBins(i);
    for (size_t j = 0; j < outSize; ++j) {
      const double width = std::abs(outputWS->x(i)[j + 1] - outputWS->x(i)[j]);
      outputWS->mutableY(i)[j] = outputWS->y(i)[j] / width;
      outputWS->mutableE(i)[j] = outputWS->e(i)[j] / width;
    }
  }
}

} // namespace Mantid::Algorithms
