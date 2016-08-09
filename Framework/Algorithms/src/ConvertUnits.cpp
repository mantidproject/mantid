//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

#include <cfloat>
#include <numeric>
#include <limits>

namespace Mantid {
namespace Algorithms {

// Register with the algorithm factory
DECLARE_ALGORITHM(ConvertUnits)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace HistogramData;
using boost::function;
using boost::bind;

/// Default constructor
ConvertUnits::ConvertUnits()
    : Algorithm(), m_numberOfSpectra(0), m_distribution(false),
      m_inputEvents(false), m_inputUnit(), m_outputUnit() {}

/// Initialisation method
void ConvertUnits::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>();
  wsValidator->add<HistogramValidator>();
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "Name of the input workspace");
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace, can be the same as the input");

  // Extract the current contents of the UnitFactory to be the allowed values of
  // the Target property
  declareProperty("Target", "", boost::make_shared<StringListValidator>(
                                    UnitFactory::Instance().getKeys()),
                  "The name of the units to convert to (must be one of those "
                  "registered in\n"
                  "the Unit Factory)");
  std::vector<std::string> propOptions{"Elastic", "Direct", "Indirect"};
  declareProperty("EMode", "Elastic",
                  boost::make_shared<StringListValidator>(propOptions),
                  "The energy mode (default: elastic)");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("EFixed", EMPTY_DBL(), mustBePositive,
                  "Value of fixed energy in meV : EI (EMode=Direct) or EF "
                  "(EMode=Indirect) . Must be\n"
                  "set if the target unit requires it (e.g. DeltaE)");

  declareProperty("AlignBins", false,
                  "If true (default is false), rebins after conversion to "
                  "ensure that all spectra in the output workspace\n"
                  "have identical bin boundaries. This option is not "
                  "recommended (see "
                  "http://www.mantidproject.org/ConvertUnits).");
}

/** Executes the algorithm
 *  @throw std::runtime_error If the input workspace has not had its unit set
 *  @throw NotImplementedError If the input workspace contains point (not
 * histogram) data
 *  @throw InstrumentDefinitionError If unable to calculate source-sample
 * distance
 */
void ConvertUnits::exec() {
  // Get the workspaces
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  this->setupMemberVariables(inputWS);

  // Check that the input workspace doesn't already have the desired unit.
  if (m_inputUnit->unitID() == m_outputUnit->unitID()) {
    const std::string outputWSName = getPropertyValue("OutputWorkspace");
    const std::string inputWSName = getPropertyValue("InputWorkspace");
    if (outputWSName == inputWSName) {
      // If it does, just set the output workspace to point to the input one and
      // be done.
      g_log.information() << "Input workspace already has target unit ("
                          << m_outputUnit->unitID()
                          << "), so just pointing the output workspace "
                             "property to the input workspace.\n";
      setProperty("OutputWorkspace",
                  boost::const_pointer_cast<MatrixWorkspace>(inputWS));
      return;
    } else {
      // Clone the workspace.
      IAlgorithm_sptr duplicate =
          createChildAlgorithm("CloneWorkspace", 0.0, 0.6);
      duplicate->initialize();
      duplicate->setProperty("InputWorkspace", inputWS);
      duplicate->execute();
      Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
      auto outputWs = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
      setProperty("OutputWorkspace", outputWs);
      return;
    }
  }

  if (inputWS->x(0).size() < 2) {
    std::stringstream msg;
    msg << "Input workspace has invalid X axis binning parameters. Should have "
           "at least 2 values. Found " << inputWS->x(0).size() << ".";
    throw std::runtime_error(msg.str());
  }
  if (inputWS->x(0).front() > inputWS->x(0).back() ||
      inputWS->x(m_numberOfSpectra / 2).front() >
          inputWS->x(m_numberOfSpectra / 2).back())
    throw std::runtime_error("Input workspace has invalid X axis binning "
                             "parameters. X values should be increasing.");

  MatrixWorkspace_sptr outputWS;
  // Check whether there is a quick conversion available
  double factor, power;
  if (m_inputUnit->quickConversion(*m_outputUnit, factor, power))
  // If test fails, could also check whether a quick conversion in the opposite
  // direction has been entered
  {
    outputWS = this->convertQuickly(inputWS, factor, power);
  } else {
    outputWS = this->convertViaTOF(m_inputUnit, inputWS);
  }

  // If the units conversion has flipped the ascending direction of X, reverse
  // all the vectors
  if (!outputWS->x(0).empty() &&
      (outputWS->x(0).front() > outputWS->x(0).back() ||
       outputWS->x(m_numberOfSpectra / 2).front() >
           outputWS->x(m_numberOfSpectra / 2).back())) {
    this->reverse(outputWS);
  }

  // Need to lop bins off if converting to energy transfer.
  // Don't do for EventWorkspaces, where you can easily rebin to recover the
  // situation without losing information
  /* This is an ugly test - could be made more general by testing for DBL_MAX
     values at the ends of all spectra, but that would be less efficient */
  if (m_outputUnit->unitID().find("Delta") == 0 && !m_inputEvents)
    outputWS = this->removeUnphysicalBins(outputWS);

  // Rebin the data to common bins if requested, and if necessary
  bool alignBins = getProperty("AlignBins");
  if (alignBins && !WorkspaceHelpers::commonBoundaries(outputWS))
    outputWS = this->alignBins(outputWS);

  // If appropriate, put back the bin width division into Y/E.
  if (m_distribution && !m_inputEvents) // Never do this for event workspaces
  {
    this->putBackBinWidth(outputWS);
  }

  // Point the output property to the right place.
  // Do right at end (workspace could could change in removeUnphysicalBins or
  // alignBins methods)
  setProperty("OutputWorkspace", outputWS);
}

/** Initialise the member variables
 *  @param inputWS The input workspace
 */
void ConvertUnits::setupMemberVariables(
    const API::MatrixWorkspace_const_sptr inputWS) {
  m_numberOfSpectra = inputWS->getNumberHistograms();
  // In the context of this algorithm, we treat things as a distribution if the
  // flag is set
  // AND the data are not dimensionless
  m_distribution = inputWS->isDistribution() && !inputWS->YUnit().empty();
  // Check if its an event workspace
  m_inputEvents =
      (boost::dynamic_pointer_cast<const EventWorkspace>(inputWS) != nullptr);

  m_inputUnit = inputWS->getAxis(0)->unit();
  const std::string targetUnit = getPropertyValue("Target");
  m_outputUnit = UnitFactory::Instance().create(targetUnit);
}

/** Create an output workspace of the appropriate (histogram or event) type and
 * copy over the data
 *  @param inputWS The input workspace
 */
API::MatrixWorkspace_sptr ConvertUnits::setupOutputWorkspace(
    const API::MatrixWorkspace_const_sptr inputWS) {
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  // If input and output workspaces are NOT the same, create a new workspace for
  // the output
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
  }

  if (!m_inputEvents && m_distribution) {
    // Loop over the histograms (detector spectra)
    Progress prog(this, 0.0, 0.2, m_numberOfSpectra);
    PARALLEL_FOR1(outputWS)
    for (int64_t i = 0; i < static_cast<int64_t>(m_numberOfSpectra); ++i) {
      PARALLEL_START_INTERUPT_REGION
      // Take the bin width dependency out of the Y & E data
      auto &X = outputWS->x(i);
      auto &Y = outputWS->mutableY(i);
      auto &E = outputWS->mutableE(i);
      for (size_t j = 0; j < outputWS->blocksize(); ++j) {
        const double width = std::abs(X[j + 1] - X[j]);
        Y[j] *= width;
        E[j] *= width;
      }

      prog.report("Convert to " + m_outputUnit->unitID());
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }

  // Set the final unit that our output workspace will have
  outputWS->getAxis(0)->unit() = m_outputUnit;
  // Store the emode
  const bool overwrite(true);
  outputWS->mutableRun().addProperty("deltaE-mode", getPropertyValue("EMode"),
                                     overwrite);

  return outputWS;
}

/** Convert the workspace units according to a simple output = a * (input^b)
 * relationship
 *  @param inputWS :: the input workspace
 *  @param factor :: the conversion factor a to apply
 *  @param power :: the Power b to apply to the conversion
 *  @returns A shared pointer to the output workspace
 */
MatrixWorkspace_sptr
ConvertUnits::convertQuickly(API::MatrixWorkspace_const_sptr inputWS,
                             const double &factor, const double &power) {
  Progress prog(this, 0.2, 1.0, m_numberOfSpectra);
  int64_t numberOfSpectra_i =
      static_cast<int64_t>(m_numberOfSpectra); // cast to make openmp happy
  // create the output workspace
  MatrixWorkspace_sptr outputWS = this->setupOutputWorkspace(inputWS);
  // See if the workspace has common bins - if so the X vector can be common
  // First a quick check using the validator
  CommonBinsValidator sameBins;
  bool commonBoundaries = false;
  if (sameBins.isValid(inputWS) == "") {
    commonBoundaries = WorkspaceHelpers::commonBoundaries(inputWS);
    // Only do the full check if the quick one passes
    if (commonBoundaries) {
      // Calculate the new (common) X values
      for (auto &x : outputWS->mutableX(0)) {
        x = factor * std::pow(x, power);
      }

      auto xVals = outputWS->sharedX(0);

      PARALLEL_FOR1(outputWS)
      for (int64_t j = 1; j < numberOfSpectra_i; ++j) {
        PARALLEL_START_INTERUPT_REGION
        outputWS->setX(j, xVals);
        prog.report("Convert to " + m_outputUnit->unitID());
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
      if (!m_inputEvents) // if in event mode the work is done
        return outputWS;
    }
  }

  EventWorkspace_sptr eventWS =
      boost::dynamic_pointer_cast<EventWorkspace>(outputWS);
  assert(static_cast<bool>(eventWS) == m_inputEvents); // Sanity check

  // If we get to here then the bins weren't aligned and each spectrum is unique
  // Loop over the histograms (detector spectra)
  PARALLEL_FOR1(outputWS)
  for (int64_t k = 0; k < numberOfSpectra_i; ++k) {
    PARALLEL_START_INTERUPT_REGION
    if (!commonBoundaries) {
      for (auto &x : outputWS->mutableX(k)) {
        x = factor * std::pow(x, power);
      }
    }
    // Convert the events themselves if necessary.
    if (m_inputEvents) {
      eventWS->getSpectrum(k).convertUnitsQuickly(factor, power);
    }
    prog.report("Convert to " + m_outputUnit->unitID());
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (m_inputEvents)
    eventWS->clearMRU();

  return outputWS;
}

/** Get the L2, theta and efixed values for a workspace index
* @param outputUnit :: The output unit
* @param source :: The source of the instrument
* @param sample :: The sample of the instrument
* @param l1 :: The source to sample distance
* @param emode :: The energy mode
* @param ws :: The workspace
* @param thetaFunction :: The function to calculate twotheta
* @param wsIndex :: The workspace index
* @param efixed :: the returned fixed energy
* @param l2 :: The returned sample - detector distance
* @param twoTheta :: the returned two theta angle
* @returns true if lookup successful, false on error
*/
bool ConvertUnits::getDetectorValues(
    const Kernel::Unit &outputUnit, const Geometry::IComponent &source,
    const Geometry::IComponent &sample, double l1, int emode,
    const MatrixWorkspace &ws,
    function<double(const Geometry::IDetector &)> thetaFunction,
    int64_t wsIndex, double &efixed, double &l2, double &twoTheta) {
  try {
    Geometry::IDetector_const_sptr det = ws.getDetector(wsIndex);
    // Get the sample-detector distance for this detector (in metres)
    if (!det->isMonitor()) {
      l2 = det->getDistance(sample);
      // The scattering angle for this detector (in radians).
      twoTheta = thetaFunction(*det);
      // If an indirect instrument, try getting Efixed from the geometry
      if (emode == 2) // indirect
      {
        if (efixed == EMPTY_DBL()) {
          try {
            // Get the parameter map
            Geometry::Parameter_sptr par =
                ws.constInstrumentParameters().getRecursive(det.get(),
                                                            "Efixed");
            if (par) {
              efixed = par->value<double>();
              g_log.debug() << "Detector: " << det->getID()
                            << " EFixed: " << efixed << "\n";
            }
          } catch (std::runtime_error &) { /* Throws if a DetectorGroup, use
                                                single provided value */
          }
        }
      }
    } else // If this is a monitor then make l1+l2 = source-detector distance
    // and twoTheta=0
    {
      l2 = det->getDistance(source);
      l2 = l2 - l1;
      twoTheta = 0.0;
      efixed = DBL_MIN;
      // Energy transfer is meaningless for a monitor, so set l2 to 0.
      if (outputUnit.unitID().find("DeltaE") != std::string::npos) {
        l2 = 0.0;
      }
    }
  } catch (Exception::NotFoundError &) {
    return false;
  }
  return true;
}

/** Convert the workspace units using TOF as an intermediate step in the
 * conversion
 * @param fromUnit :: The unit of the input workspace
 * @param inputWS :: The input workspace
 * @returns A shared pointer to the output workspace
 */
MatrixWorkspace_sptr
ConvertUnits::convertViaTOF(Kernel::Unit_const_sptr fromUnit,
                            API::MatrixWorkspace_const_sptr inputWS) {
  using namespace Geometry;

  Progress prog(this, 0.2, 1.0, m_numberOfSpectra);
  int64_t numberOfSpectra_i =
      static_cast<int64_t>(m_numberOfSpectra); // cast to make openmp happy

  // Get a pointer to the instrument contained in the workspace
  Instrument_const_sptr instrument = inputWS->getInstrument();

  Kernel::Unit_const_sptr outputUnit = m_outputUnit;

  // Get the distance between the source and the sample (assume in metres)
  IComponent_const_sptr source = instrument->getSource();
  IComponent_const_sptr sample = instrument->getSample();
  if (source == nullptr || sample == nullptr) {
    throw Exception::InstrumentDefinitionError("Instrument not sufficiently "
                                               "defined: failed to get source "
                                               "and/or sample");
  }
  double l1;
  try {
    l1 = source->getDistance(*sample);
    g_log.debug() << "Source-sample distance: " << l1 << '\n';
  } catch (Exception::NotFoundError &) {
    g_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError(
        "Unable to calculate source-sample distance", inputWS->getTitle());
  }

  int failedDetectorCount = 0;

  /// @todo No implementation for any of these in the geometry yet so using
  /// properties
  const std::string emodeStr = getProperty("EMode");
  // Convert back to an integer representation
  int emode = 0;
  if (emodeStr == "Direct")
    emode = 1;
  else if (emodeStr == "Indirect")
    emode = 2;

  // Not doing anything with the Y vector in to/fromTOF yet, so just pass empty
  // vector
  std::vector<double> emptyVec;
  const bool needEfixed =
      (outputUnit->unitID().find("DeltaE") != std::string::npos ||
       outputUnit->unitID().find("Wave") != std::string::npos);
  double efixedProp = getProperty("Efixed");
  if (emode == 1) {
    //... direct efixed gather
    if (efixedProp == EMPTY_DBL()) {
      // try and get the value from the run parameters
      const API::Run &run = inputWS->run();
      if (run.hasProperty("Ei")) {
        Kernel::Property *prop = run.getProperty("Ei");
        efixedProp = boost::lexical_cast<double, std::string>(prop->value());
      } else {
        if (needEfixed) {
          throw std::invalid_argument(
              "Could not retrieve incident energy from run object");
        } else {
          efixedProp = 0.0;
        }
      }
    }
  } else if (emode == 0 && efixedProp == EMPTY_DBL()) // Elastic
  {
    efixedProp = 0.0;
  }

  std::vector<std::string> parameters =
      inputWS->getInstrument()->getStringParameter("show-signed-theta");
  bool bUseSignedVersion =
      (!parameters.empty()) &&
      find(parameters.begin(), parameters.end(), "Always") != parameters.end();
  function<double(const IDetector &)> thetaFunction =
      bUseSignedVersion
          ? bind(&MatrixWorkspace::detectorSignedTwoTheta, inputWS, _1)
          : bind(&MatrixWorkspace::detectorTwoTheta, inputWS, _1);

  // Perform Sanity Validation before creating workspace
  double checkefixed = efixedProp;
  double checkl2;
  double checktwoTheta;
  size_t checkIndex = 0;
  if (getDetectorValues(*outputUnit, *source, *sample, l1, emode, *inputWS,
                        thetaFunction, checkIndex, checkefixed, checkl2,
                        checktwoTheta)) {
    const double checkdelta = 0.0;
    // copy the X values for the check
    auto checkXValues = inputWS->readX(checkIndex);
    // Convert the input unit to time-of-flight
    auto checkFromUnit = std::unique_ptr<Unit>(fromUnit->clone());
    auto checkOutputUnit = std::unique_ptr<Unit>(outputUnit->clone());
    checkFromUnit->toTOF(checkXValues, emptyVec, l1, checkl2, checktwoTheta,
                         emode, checkefixed, checkdelta);
    // Convert from time-of-flight to the desired unit
    checkOutputUnit->fromTOF(checkXValues, emptyVec, l1, checkl2, checktwoTheta,
                             emode, checkefixed, checkdelta);
  }

  // create the output workspace
  MatrixWorkspace_sptr outputWS = this->setupOutputWorkspace(inputWS);
  EventWorkspace_sptr eventWS =
      boost::dynamic_pointer_cast<EventWorkspace>(outputWS);
  assert(static_cast<bool>(eventWS) == m_inputEvents); // Sanity check

  // Loop over the histograms (detector spectra)
  PARALLEL_FOR1(outputWS)
  for (int64_t i = 0; i < numberOfSpectra_i; ++i) {
    PARALLEL_START_INTERUPT_REGION
    double efixed = efixedProp;

    // Now get the detector object for this histogram
    double l2;
    double twoTheta;
    if (getDetectorValues(*outputUnit, *source, *sample, l1, emode, *outputWS,
                          thetaFunction, i, efixed, l2, twoTheta)) {

      // Make local copies of the units. This allows running the loop in
      // parallel
      auto localFromUnit = std::unique_ptr<Unit>(fromUnit->clone());
      auto localOutputUnit = std::unique_ptr<Unit>(outputUnit->clone());

      /// @todo Don't yet consider hold-off (delta)
      const double delta = 0.0;

      // TODO toTOF and fromTOF need to be reimplemented outside of kernel
      localFromUnit->toTOF(outputWS->dataX(i), emptyVec, l1, l2, twoTheta,
                           emode, efixed, delta);
      // Convert from time-of-flight to the desired unit
      localOutputUnit->fromTOF(outputWS->dataX(i), emptyVec, l1, l2, twoTheta,
                               emode, efixed, delta);

      // EventWorkspace part, modifying the EventLists.
      if (m_inputEvents) {
        eventWS->getSpectrum(i)
            .convertUnitsViaTof(localFromUnit.get(), localOutputUnit.get());
      }
    } else {
      // Get to here if exception thrown when calculating distance to detector
      failedDetectorCount++;
      // Since you usually (always?) get to here when there's no attached
      // detectors, this call is
      // the same as just zeroing out the data (calling clearData on the
      // spectrum)
      outputWS->maskWorkspaceIndex(i);
    }

    prog.report("Convert to " + m_outputUnit->unitID());
    PARALLEL_END_INTERUPT_REGION
  } // loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION

  if (failedDetectorCount != 0) {
    g_log.information() << "Unable to calculate sample-detector distance for "
                        << failedDetectorCount
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
API::MatrixWorkspace_sptr
ConvertUnits::alignBins(API::MatrixWorkspace_sptr workspace) {
  // Create a Rebin child algorithm
  IAlgorithm_sptr childAlg = createChildAlgorithm("Rebin");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", workspace);
  // Next line for EventWorkspaces - needed for as long as in/out set same
  // keeps
  // as events.
  childAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", workspace);
  childAlg->setProperty<std::vector<double>>(
      "Params", this->calculateRebinParams(workspace));
  childAlg->executeAsChildAlg();
  return childAlg->getProperty("OutputWorkspace");
}

/// The Rebin parameters should cover the full range of the converted unit,
/// with
/// the same number of bins
const std::vector<double> ConvertUnits::calculateRebinParams(
    const API::MatrixWorkspace_const_sptr workspace) const {
  // Need to loop round and find the full range
  double XMin = DBL_MAX, XMax = DBL_MIN;
  const size_t numSpec = workspace->getNumberHistograms();
  for (size_t i = 0; i < numSpec; ++i) {
    try {
      Geometry::IDetector_const_sptr det = workspace->getDetector(i);
      if (!det->isMasked()) {
        auto &XData = workspace->x(i);
        double xfront = XData.front();
        double xback = XData.back();
        if (boost::math::isfinite(xfront) && boost::math::isfinite(xback)) {
          if (xfront < XMin)
            XMin = xfront;
          if (xback > XMax)
            XMax = xback;
        }
      }
    } catch (Exception::NotFoundError &) {
    } // Do nothing
  }
  const double step =
      (XMax - XMin) / static_cast<double>(workspace->blocksize());

  return {XMin, step, XMax};
}

/** Reverses the workspace if X values are in descending order
 *  @param WS The workspace to operate on
 */
void ConvertUnits::reverse(API::MatrixWorkspace_sptr WS) {
  EventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<EventWorkspace>(WS);
  bool isInputEvents = static_cast<bool>(eventWS);
  size_t numberOfSpectra = WS->getNumberHistograms();
  if (WorkspaceHelpers::commonBoundaries(WS) && !isInputEvents) {
    auto reverseX = make_cow<HistogramData::HistogramX>(WS->x(0).crbegin(),
                                                        WS->x(0).crend());

    for (size_t j = 0; j < numberOfSpectra; ++j) {
      WS->setSharedX(j, reverseX);
      std::reverse(WS->dataY(j).begin(), WS->dataY(j).end());
      std::reverse(WS->dataE(j).begin(), WS->dataE(j).end());
      if (j % 100 == 0)
        interruption_point();
    }
  } else {
    // either events or ragged boundaries
    int numberOfSpectra_i = static_cast<int>(numberOfSpectra);
    PARALLEL_FOR1(WS)
    for (int j = 0; j < numberOfSpectra_i; ++j) {
      PARALLEL_START_INTERUPT_REGION
      if (isInputEvents) {
        eventWS->getSpectrum(j).reverse();
      } else {
        std::reverse(WS->mutableX(j).begin(), WS->mutableX(j).end());
        std::reverse(WS->mutableY(j).begin(), WS->mutableY(j).end());
        std::reverse(WS->mutableE(j).begin(), WS->mutableE(j).end());
      }
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
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
API::MatrixWorkspace_sptr ConvertUnits::removeUnphysicalBins(
    const Mantid::API::MatrixWorkspace_const_sptr workspace) {
  MatrixWorkspace_sptr result;

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
      try {
        Geometry::IDetector_const_sptr det = workspace->getDetector(i);
        if (!det->isMonitor())
          break;
      } catch (Exception::NotFoundError &) { /* Do nothing */
      }
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

    result =
        WorkspaceFactory::Instance().create(workspace, numSpec, bins, bins - 1);

    for (size_t i = 0; i < numSpec; ++i) {
      auto &X = workspace->x(i);
      auto &Y = workspace->y(i);
      auto &E = workspace->e(i);
      result->mutableX(i).assign(X.begin() + first, X.end());
      result->mutableY(i).assign(Y.begin() + first, Y.end());
      result->mutableE(i).assign(E.begin() + first, E.end());
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
    result = WorkspaceFactory::Instance().create(workspace, numSpec, maxBins,
                                                 maxBins - 1);
    // Next, loop again copying in the correct range for each spectrum
    for (int64_t j = 0; j < int64_t(numSpec); ++j) {
      auto edges = workspace->binEdges(j);
      auto k = lastBins[j];

      result->mutableX(j).assign(edges.cbegin(), edges.cbegin() + k);

      // If the entire X range is not covered, generate fake values.
      if (k < maxBins) {
        std::iota(result->mutableX(j).begin() + k, result->mutableX(j).end(),
                  workspace->x(j)[k] + 1);
      }

      result->mutableY(j)
          .assign(workspace->y(j).cbegin(), workspace->y(j).cbegin() + (k - 1));
      result->mutableE(j)
          .assign(workspace->e(j).cbegin(), workspace->e(j).cbegin() + (k - 1));
    }
  }

  return result;
}

/** Divide by the bin width if workspace is a distribution
 *  @param outputWS The workspace to operate on
 */
void ConvertUnits::putBackBinWidth(const API::MatrixWorkspace_sptr outputWS) {
  const size_t outSize = outputWS->blocksize();

  for (size_t i = 0; i < m_numberOfSpectra; ++i) {
    for (size_t j = 0; j < outSize; ++j) {
      const double width = std::abs(outputWS->x(i)[j + 1] - outputWS->x(i)[j]);
      outputWS->mutableY(i)[j] = outputWS->y(i)[j] / width;
      outputWS->mutableE(i)[j] = outputWS->e(i)[j] / width;
    }
  }
}

} // namespace Algorithm
} // namespace Mantid
