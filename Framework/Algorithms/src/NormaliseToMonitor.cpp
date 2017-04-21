#include "MantidAlgorithms/NormaliseToMonitor.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/RawCountValidator.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/VectorHelper.h"

#include <cfloat>
#include <numeric>

using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using Mantid::Kernel::IPropertyManager;

namespace Mantid {

//
namespace Algorithms {

// Method of complex validator class
// method checks if the property is enabled
bool MonIDPropChanger::isEnabled(const IPropertyManager *algo) const {
  int sp_id = algo->getProperty(SpectraNum);
  // if there is spectra number set to normalize by, nothing else can be
  // selected;
  if (sp_id > 0) {
    is_enabled = false;
    return false;
  } else {
    is_enabled = true;
    ;
  }

  // is there the ws property, which describes monitors ws. It also disables the
  // monitors ID property
  API::MatrixWorkspace_const_sptr monitorsWS =
      algo->getProperty(MonitorWorkspaceProp);
  if (monitorsWS) {
    is_enabled = false;
  } else {
    is_enabled = true;
  }
  return is_enabled;
}

// method checks if other properties have changed and these changes affected
// MonID property
bool MonIDPropChanger::isConditionChanged(const IPropertyManager *algo) const {
  // is enabled is based on other properties:
  if (!is_enabled)
    return false;

  // read monitors list from the input workspace
  API::MatrixWorkspace_const_sptr inputWS = algo->getProperty(hostWSname);
  bool monitors_changed = monitorIdReader(inputWS);

  //       std::cout << "MonIDPropChanger::isConditionChanged() called  ";
  //       std::cout << monitors_changed << '\n';

  return monitors_changed;
}
// function which modifies the allowed values for the list of monitors.
void MonIDPropChanger::applyChanges(const IPropertyManager *algo,
                                    Kernel::Property *const pProp) {
  Kernel::PropertyWithValue<int> *piProp =
      dynamic_cast<Kernel::PropertyWithValue<int> *>(pProp);
  if (!piProp) {
    throw(std::invalid_argument(
        "modify allowed value has been called on wrong property"));
  }
  //
  if (iExistingAllowedValues.empty()) {
    API::MatrixWorkspace_const_sptr inputWS = algo->getProperty(hostWSname);
    int spectra_max(-1);
    if (inputWS) { // let's assume that detectors IDs correspond to spectraID --
                   // not always the case but often. TODO: should be fixed
      spectra_max = static_cast<int>(inputWS->getNumberHistograms()) + 1;
    }
    piProp->replaceValidator(
        boost::make_shared<Kernel::BoundedValidator<int>>(-1, spectra_max));
  } else {
    piProp->replaceValidator(
        boost::make_shared<Kernel::ListValidator<int>>(iExistingAllowedValues));
  }
}

// read the monitors list from the workspace and try to do it once for any
// particular ws;
bool MonIDPropChanger::monitorIdReader(
    API::MatrixWorkspace_const_sptr inputWS) const {
  // no workspace
  if (!inputWS)
    return false;

  // no instrument
  Geometry::Instrument_const_sptr pInstr = inputWS->getInstrument();
  if (!pInstr)
    return false;

  std::vector<detid_t> mon = pInstr->getMonitors();
  if (mon.empty()) {
    if (iExistingAllowedValues.empty()) {
      return false;
    } else {
      iExistingAllowedValues.clear();
      return true;
    }
  }
  // are these monitors really there?
  // got the index of correspondent spectra.
  std::vector<size_t> indexList = inputWS->getIndicesFromDetectorIDs(mon);
  if (indexList.empty()) {
    if (iExistingAllowedValues.empty()) {
      return false;
    } else {
      iExistingAllowedValues.clear();
      return true;
    }
  }
  // index list can be less or equal to the mon list size (some monitors do not
  // have spectra)
  size_t mon_count =
      (mon.size() < indexList.size()) ? mon.size() : indexList.size();
  std::vector<int> allowed_values(mon_count);
  for (size_t i = 0; i < mon_count; i++) {
    allowed_values[i] = mon[i];
  }

  // are known values the same as the values we have just identified?
  if (iExistingAllowedValues.size() != mon_count) {
    iExistingAllowedValues.clear();
    iExistingAllowedValues.assign(allowed_values.begin(), allowed_values.end());
    return true;
  }
  // the monitor list has the same size as before. Is it equivalent to the
  // existing one?
  bool values_redefined = false;
  for (size_t i = 0; i < mon_count; i++) {
    if (iExistingAllowedValues[i] != allowed_values[i]) {
      values_redefined = true;
      iExistingAllowedValues[i] = allowed_values[i];
    }
  }
  return values_redefined;
}

// Register with the algorithm factory
DECLARE_ALGORITHM(NormaliseToMonitor)

using namespace Kernel;
using namespace API;
using std::size_t;

void NormaliseToMonitor::init() {
  auto val = boost::make_shared<CompositeValidator>();
  val->add<HistogramValidator>();
  val->add<RawCountValidator>();
  // It's been said that we should restrict the unit to being wavelength, but
  // I'm not sure about that...
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                       val),
      "Name of the input workspace. Must be a non-distribution histogram.");

  //
  // declareProperty("NofmalizeByAnySpectra",false,
  //   "Allows you to normalize the workspace by any spectra, not just by the
  //   monitor one");
  // Can either set a spectrum within the workspace to be the monitor
  // spectrum.....
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Name to use for the output workspace");
  // should be any spectrum number, but named this property MonitorSpectrum to
  // keep
  // compatibility with previous scripts
  // Can either set a spectrum within the workspace to be the monitor
  // spectrum.....
  declareProperty("MonitorSpectrum", -1,
                  "The spectrum number within the InputWorkspace you want to "
                  "normalize by (It can be a monitor spectrum or a spectrum "
                  "responsible for a group of detectors or monitors)",
                  Direction::InOut);

  // Or take monitor ID to identify the spectrum one wish to use or
  declareProperty("MonitorID", -1,
                  "The MonitorID (pixel ID), which defines the monitor's data "
                  "within the InputWorkspace. Will be overridden by the values "
                  "correspondent to MonitorSpectrum field if one is provided "
                  "in the field above.\n"
                  "If workspace do not have monitors, the MonitorID can refer "
                  "to empty data and the field then can accepts any MonitorID "
                  "within the InputWorkspace.");
  // set up the validator, which would verify if spectrum is correct
  setPropertySettings("MonitorID", Kernel::make_unique<MonIDPropChanger>(
                                       "InputWorkspace", "MonitorSpectrum",
                                       "MonitorWorkspace"));

  // ...or provide it in a separate workspace (note: optional WorkspaceProperty)
  declareProperty(make_unique<WorkspaceProperty<>>("MonitorWorkspace", "",
                                                   Direction::Input,
                                                   PropertyMode::Optional, val),
                  "A workspace containing one or more spectra to normalize the "
                  "InputWorkspace by.");
  setPropertySettings("MonitorWorkspace",
                      Kernel::make_unique<Kernel::EnabledWhenProperty>(
                          "MonitorSpectrum", IS_DEFAULT));

  declareProperty("MonitorWorkspaceIndex", 0,
                  "The index of the spectrum within the MonitorWorkspace(2 "
                  "(0<=ind<=nHistograms in MonitorWorkspace) you want to "
                  "normalize by\n"
                  "(usually related to the index, responsible for the "
                  "monitor's data but can be any).\n"
                  "If no value is provided in this field, '''InputWorkspace''' "
                  "will be normalized by first spectra (with index 0)",
                  Direction::InOut);
  setPropertySettings("MonitorWorkspaceIndex",
                      Kernel::make_unique<Kernel::EnabledWhenProperty>(
                          "MonitorSpectrum", IS_DEFAULT));

  // If users set either of these optional properties two things happen
  // 1) normalization is by an integrated count instead of bin-by-bin
  // 2) if the value is within the range of X's in the spectrum it crops the
  // spectrum
  declareProperty("IntegrationRangeMin", EMPTY_DBL(),
                  "If set, normalization will be by integrated count from this "
                  "minimum x value");
  declareProperty("IntegrationRangeMax", EMPTY_DBL(),
                  "If set, normalization will be by integrated count up to "
                  "this maximum x value");
  declareProperty(
      "IncludePartialBins", false,
      "If true and an integration range is set then partial bins at either \n"
      "end of the integration range are also included");

  declareProperty(
      make_unique<WorkspaceProperty<>>("NormFactorWS", "", Direction::Output,
                                       PropertyMode::Optional),
      "Name of the workspace, containing the normalization factor.\n"
      "If this name is empty, normalization workspace is not returned. If the "
      "name coincides with the output workspace name, _normFactor suffix is "
      "added to this name");
}

void NormaliseToMonitor::exec() {
  // Get the input workspace
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // First check the inputs, throws std::runtime_error if a property is invalid
  this->checkProperties(inputWS);

  // See if the normalization with integration properties are set,
  // throws std::runtime_error if a property is invalid
  const bool integrate = this->setIntegrationProps();

  if (integrate) {
    this->normaliseByIntegratedCount(inputWS, outputWS);
  } else {
    this->normaliseBinByBin(inputWS, outputWS);
  }

  setProperty("OutputWorkspace", outputWS);
  std::string norm_ws_name = getPropertyValue("NormFactorWS");
  if (!norm_ws_name.empty()) {
    std::string out_name = getPropertyValue("OutputWorkspace");
    if (out_name == norm_ws_name) {
      // if the normalization factor workspace name coincides with output
      // workspace name, add _normFactor suffix to this name
      norm_ws_name = norm_ws_name + "_normFactor";
      auto pProp = (this->getPointerToProperty("NormFactorWS"));
      pProp->setValue(norm_ws_name);
    }
    setProperty("NormFactorWS", m_monitor);
  }
}

/** Makes sure that the input properties are set correctly
 *  @param inputWorkspace The input workspace
 *  @throw std::runtime_error If the properties are invalid
 */
void NormaliseToMonitor::checkProperties(
    const API::MatrixWorkspace_sptr &inputWorkspace) {

  // Check where the monitor spectrum should come from
  Property *monSpec = getProperty("MonitorSpectrum");
  Property *monWS = getProperty("MonitorWorkspace");
  Property *monID = getProperty("MonitorID");
  // Is the monitor spectrum within the main input workspace
  const bool inWS = !monSpec->isDefault();
  // Or is it in a separate workspace
  bool sepWS = !monWS->isDefault();
  // or monitor ID
  bool monIDs = !monID->isDefault();
  // something has to be set
  if (!inWS && !sepWS && !monIDs) {
    const std::string mess("Neither the MonitorSpectrum, nor the MonitorID or "
                           "the MonitorWorkspace property has been set");
    g_log.error() << mess << '\n';
    throw std::runtime_error(mess);
  }
  // One and only one of these properties should have been set
  // input from separate workspace is overwritten by monitor spectrum
  if (inWS && sepWS) {
    g_log.information("Both input workspace MonitorSpectrum number and monitor "
                      "workspace are specified. Ignoring Monitor Workspace");
    sepWS = false;
  }
  // input from detector ID is rejected in favor of monitor sp
  if (inWS && monIDs) {
    g_log.information("Both input workspace MonitorSpectrum number and "
                      "detector ID are specified. Ignoring Detector ID");
    monIDs = false;
  }
  // separate ws takes over detectorID (this logic is duplicated within
  // getInWSMonitorSpectrum)
  if (sepWS && monIDs) {
    g_log.information("Both input MonitorWorkspace and detector ID are "
                      "specified. Ignoring Detector ID");
  }

  // Do a check for common binning and store
  m_commonBins = API::WorkspaceHelpers::commonBoundaries(*inputWorkspace);

  int spec_num(-1);
  // Check the monitor spectrum or workspace and extract into new workspace
  m_monitor = sepWS ? this->getMonitorWorkspace(inputWorkspace, spec_num)
                    : this->getInWSMonitorSpectrum(inputWorkspace, spec_num);

  // Check that the 'monitor' spectrum actually relates to a monitor - warn if
  // not
  try {
    if (!m_monitor->spectrumInfo().isMonitor(0)) {
      g_log.warning() << "The spectrum N: " << spec_num
                      << " in MonitorWorkspace does not refer to a monitor.\n"
                      << "Continuing with normalization regardless.";
    }
  } catch (Kernel::Exception::NotFoundError &) {
    g_log.warning(
        "Unable to check if the spectrum provided relates to a monitor - "
        "the instrument is not fully specified.\n"
        "Continuing with normalization regardless.");
  }
}

/** Checks and retrieves the requested spectrum out of the input workspace
 *  @param inputWorkspace The input workspace.
 *  @param spectra_num The spectra number.
 *  @returns A workspace containing the monitor spectrum only.
 *  @returns spectra number (WS ID) which is used to normalize by.
 *  @throw std::runtime_error If the properties are invalid
 */
API::MatrixWorkspace_sptr NormaliseToMonitor::getInWSMonitorSpectrum(
    const API::MatrixWorkspace_sptr &inputWorkspace, int &spectra_num) {
  // this is the index of the spectra within the workspace and we need to
  // identify it either from DetID or from SpecID
  // size_t spectra_num(-1);
  // try monitor spectrum. If it is specified, it overrides everything
  int monitorSpec = getProperty("MonitorSpectrum");
  if (monitorSpec < 0) {
    // Get hold of the monitor spectrum through detector ID
    int monitorID = getProperty("MonitorID");
    if (monitorID < 0) {
      throw std::runtime_error(
          "Both MonitorSpectrum and MonitorID can not be negative");
    }
    // set spectra of detector's ID of one selected monitor ID
    std::vector<detid_t> detID(1, monitorID);
    // got the index of correspondent spectra (should be only one).
    auto indexList = inputWorkspace->getIndicesFromDetectorIDs(detID);
    if (indexList.empty()) {
      throw std::runtime_error(
          "Can not find spectra, corresponding to the requested monitor ID");
    }
    if (indexList.size() > 1) {
      throw std::runtime_error("More then one spectra corresponds to the "
                               "requested monitor ID, which is unheard of");
    }
    spectra_num = static_cast<int>(indexList[0]);
  } else { // monitor spectrum is specified.
    const SpectraAxis *axis =
        dynamic_cast<const SpectraAxis *>(inputWorkspace->getAxis(1));
    if (!axis) {
      throw std::runtime_error("Cannot retrieve monitor spectrum - spectrum "
                               "numbers not attached to workspace");
    }
    auto specs = axis->getSpectraIndexMap();
    if (!specs.count(monitorSpec)) {
      throw std::runtime_error("Input workspace does not contain spectrum "
                               "number given for MonitorSpectrum");
    }
    spectra_num = static_cast<int>(specs[monitorSpec]);
  }
  return this->extractMonitorSpectrum(inputWorkspace, spectra_num);
}

/** Checks and retrieves the monitor spectrum out of the input workspace
 *  @param inputWorkspace The input workspace.
 *  @param wsID The workspace ID.
 *  @returns A workspace containing the monitor spectrum only
 *  @throw std::runtime_error If the properties are invalid
 */
API::MatrixWorkspace_sptr NormaliseToMonitor::getMonitorWorkspace(
    const API::MatrixWorkspace_sptr &inputWorkspace, int &wsID) {
  // Get the workspace from the ADS. Will throw if it's not there.
  MatrixWorkspace_sptr monitorWS = getProperty("MonitorWorkspace");
  wsID = getProperty("MonitorWorkspaceIndex");
  // Check that it's a single spectrum workspace
  if (static_cast<int>(monitorWS->getNumberHistograms()) < wsID) {
    throw std::runtime_error(
        "The MonitorWorkspace must contain the MonitorWorkspaceIndex");
  }
  // Check that the two workspace come from the same instrument
  if (monitorWS->getInstrument()->getName() !=
      inputWorkspace->getInstrument()->getName()) {
    throw std::runtime_error(
        "The Input and Monitor workspaces must come from the same instrument");
  }
  // Check that they're in the same units
  if (monitorWS->getAxis(0)->unit()->unitID() !=
      inputWorkspace->getAxis(0)->unit()->unitID()) {
    throw std::runtime_error(
        "The Input and Monitor workspaces must have the same unit");
  }

  // In this case we need to test whether the bins in the monitor workspace
  // match
  m_commonBins = (m_commonBins && API::WorkspaceHelpers::matchingBins(
                                      *inputWorkspace, *monitorWS, true));

  // If the workspace passes all these tests, make a local copy because it will
  // get changed
  return this->extractMonitorSpectrum(monitorWS, wsID);
}

/** Pulls the monitor spectrum out of a larger workspace
 *  @param WS :: The workspace containing the spectrum to extract
 *  @param index :: The index of the spectrum to extract
 *  @returns A workspace containing the single spectrum requested
 */
API::MatrixWorkspace_sptr
NormaliseToMonitor::extractMonitorSpectrum(const API::MatrixWorkspace_sptr &WS,
                                           const std::size_t index) {
  IAlgorithm_sptr childAlg = createChildAlgorithm("ExtractSingleSpectrum");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->setProperty<int>("WorkspaceIndex", static_cast<int>(index));
  childAlg->executeAsChildAlg();
  MatrixWorkspace_sptr outWS = childAlg->getProperty("OutputWorkspace");

  IAlgorithm_sptr alg = createChildAlgorithm("ConvertToMatrixWorkspace");
  alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outWS);
  alg->executeAsChildAlg();
  outWS = alg->getProperty("OutputWorkspace");

  // Only get to here if successful
  return outWS;
}

/** Sets the maximum and minimum X values of the monitor spectrum to use for
 * integration
 *  @return True if the maximum or minimum values are set
 *  @throw std::runtime_error If the minimum was set higher than the maximum
 */
bool NormaliseToMonitor::setIntegrationProps() {
  m_integrationMin = getProperty("IntegrationRangeMin");
  m_integrationMax = getProperty("IntegrationRangeMax");

  // Check if neither of these have been changed from their defaults
  // (EMPTY_DBL())
  if (isEmpty(m_integrationMin) && isEmpty(m_integrationMax)) {
    // Nothing has been set so the user doesn't want to use integration so let's
    // move on
    return false;
  }
  // Yes integration is going to be used...

  // There is only one set of values that is unacceptable let's check for that
  if (!isEmpty(m_integrationMin) && !isEmpty(m_integrationMax)) {
    if (m_integrationMin > m_integrationMax) {
      throw std::runtime_error(
          "Integration minimum set to larger value than maximum!");
    }
  }

  // Now check the end X values are within the X value range of the workspace
  if (isEmpty(m_integrationMin) || m_integrationMin < m_monitor->x(0).front()) {
    g_log.warning() << "Integration range minimum set to workspace min: "
                    << m_integrationMin << '\n';
    m_integrationMin = m_monitor->x(0).front();
  }
  if (isEmpty(m_integrationMax) || m_integrationMax > m_monitor->x(0).back()) {
    g_log.warning() << "Integration range maximum set to workspace max: "
                    << m_integrationMax << '\n';
    m_integrationMax = m_monitor->x(0).back();
  }

  // Return indicating that these properties should be used
  return true;
}

/** Carries out a normalization based on the integrated count of the monitor
 * over a range
 *  @param inputWorkspace The input workspace
 *  @param outputWorkspace The result workspace
 */
void NormaliseToMonitor::normaliseByIntegratedCount(
    const API::MatrixWorkspace_sptr &inputWorkspace,
    API::MatrixWorkspace_sptr &outputWorkspace) {
  // Add up all the bins so it's just effectively a single value with an error
  IAlgorithm_sptr integrate = createChildAlgorithm("Integration");
  integrate->setProperty<MatrixWorkspace_sptr>("InputWorkspace", m_monitor);
  integrate->setProperty("RangeLower", m_integrationMin);
  integrate->setProperty("RangeUpper", m_integrationMax);
  integrate->setProperty<bool>("IncludePartialBins",
                               getProperty("IncludePartialBins"));

  integrate->executeAsChildAlg();

  // Get back the result
  m_monitor = integrate->getProperty("OutputWorkspace");

  // Run the divide algorithm explicitly to enable progress reporting
  IAlgorithm_sptr divide = createChildAlgorithm("Divide", 0.0, 1.0);
  divide->setProperty<MatrixWorkspace_sptr>("LHSWorkspace", inputWorkspace);
  divide->setProperty<MatrixWorkspace_sptr>("RHSWorkspace", m_monitor);
  divide->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWorkspace);

  divide->executeAsChildAlg();

  // Get back the result
  outputWorkspace = divide->getProperty("OutputWorkspace");
}

/** Carries out the bin-by-bin normalization
 *  @param inputWorkspace The input workspace
 *  @param outputWorkspace The result workspace
 */
void NormaliseToMonitor::normaliseBinByBin(
    const API::MatrixWorkspace_sptr &inputWorkspace,
    API::MatrixWorkspace_sptr &outputWorkspace) {
  EventWorkspace_sptr inputEvent =
      boost::dynamic_pointer_cast<EventWorkspace>(inputWorkspace);

  // Only create output workspace if different to input one
  if (outputWorkspace != inputWorkspace) {
    if (inputEvent) {
      outputWorkspace = inputWorkspace->clone();
    } else
      outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace);
  }
  auto outputEvent =
      boost::dynamic_pointer_cast<EventWorkspace>(outputWorkspace);

  // Get hold of the monitor spectrum
  const auto &monX = m_monitor->binEdges(0);
  auto monY = m_monitor->counts(0);
  auto monE = m_monitor->countStandardDeviations(0);
  // Calculate the overall normalization just the once if bins are all matching
  if (m_commonBins)
    this->normalisationFactor(monX, monY, monE);

  const size_t numHists = inputWorkspace->getNumberHistograms();
  auto specLength = inputWorkspace->blocksize();
  // Flag set when a division by 0 is found
  bool hasZeroDivision = false;
  Progress prog(this, 0.0, 1.0, numHists);
  // Loop over spectra
  PARALLEL_FOR_IF(
      Kernel::threadSafe(*inputWorkspace, *outputWorkspace, *m_monitor))
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION
    prog.report();

    const auto &X = inputWorkspace->binEdges(i);
    // If not rebinning, just point to our monitor spectra, otherwise create new
    // vectors

    auto Y = (m_commonBins ? monY : Counts(specLength));
    auto E = (m_commonBins ? monE : CountStandardDeviations(specLength));

    if (!m_commonBins) {
      // ConvertUnits can give X vectors of all zeros - skip these, they cause
      // problems
      if (X.back() == 0.0 && X.front() == 0.0)
        continue;
      // Rebin the monitor spectrum to match the binning of the current data
      // spectrum
      VectorHelper::rebinHistogram(
          monX.rawData(), monY.mutableRawData(), monE.mutableRawData(),
          X.rawData(), Y.mutableRawData(), E.mutableRawData(), false);
      // Recalculate the overall normalization factor
      this->normalisationFactor(X, Y, E);
    }

    if (inputEvent) {
      // ----------------------------------- EventWorkspace
      // ---------------------------------------
      EventList &outEL = outputEvent->getSpectrum(i);
      outEL.divide(X.rawData(), Y.mutableRawData(), E.mutableRawData());
    } else {
      // ----------------------------------- Workspace2D
      // ---------------------------------------
      auto &YOut = outputWorkspace->mutableY(i);
      auto &EOut = outputWorkspace->mutableE(i);
      const auto &inY = inputWorkspace->y(i);
      const auto &inE = inputWorkspace->e(i);
      outputWorkspace->mutableX(i) = inputWorkspace->x(i);

      // The code below comes more or less straight out of Divide.cpp
      for (size_t k = 0; k < specLength; ++k) {
        // Get the input Y's
        const double leftY = inY[k];
        const double rightY = Y[k];

        if (rightY == 0.0) {
          hasZeroDivision = true;
        }

        // Calculate result and store in local variable to avoid overwriting
        // original data if
        // output workspace is same as one of the input ones
        const double newY = leftY / rightY;

        if (fabs(rightY) > 1.0e-12 && fabs(newY) > 1.0e-12) {
          const double lhsFactor = (inE[k] < 1.0e-12 || fabs(leftY) < 1.0e-12)
                                       ? 0.0
                                       : pow((inE[k] / leftY), 2);
          const double rhsFactor =
              E[k] < 1.0e-12 ? 0.0 : pow((E[k] / rightY), 2);
          EOut[k] = std::abs(newY) * sqrt(lhsFactor + rhsFactor);
        }

        // Now store the result
        YOut[k] = newY;
      } // end Workspace2D case
    }   // end loop over current spectrum

    PARALLEL_END_INTERUPT_REGION
  } // end loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION
  if (hasZeroDivision) {
    g_log.warning() << "Division by zero in some of the bins.\n";
  }
  if (inputEvent)
    outputEvent->clearMRU();
}

/** Calculates the overall normalization factor.
 *  This multiplies result by (bin width * sum of monitor counts) / total frame
 * width.
 *  @param X The BinEdges of the workspace
 *  @param Y The Counts of the workspace
 *  @param E The CountStandardDeviations of the workspace
 */
void NormaliseToMonitor::normalisationFactor(const BinEdges &X, Counts &Y,
                                             CountStandardDeviations &E) {
  const double monitorSum = std::accumulate(Y.begin(), Y.end(), 0.0);
  const double range = X.back() - X.front();
  auto specLength = Y.size();

  auto &yNew = Y.mutableRawData();
  auto &eNew = E.mutableRawData();

  for (size_t j = 0; j < specLength; ++j) {
    const double factor = range / ((X[j + 1] - X[j]) * monitorSum);
    yNew[j] *= factor;
    eNew[j] *= factor;
  }
}

} // namespace Algorithm
} // namespace Mantid
