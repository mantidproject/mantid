// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/NormaliseToMonitor.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/RawCountValidator.h"
#include "MantidAPI/SingleCountValidator.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <cfloat>
#include <numeric>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using Mantid::Kernel::IPropertyManager;

namespace Mantid::Algorithms {

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
  }

  // is there the ws property, which describes monitors ws. It also disables the
  // monitors ID property
  MatrixWorkspace_const_sptr monitorsWS = algo->getProperty(MonitorWorkspaceProp);
  if (monitorsWS) {
    is_enabled = false;
  } else {
    is_enabled = true;
  }
  return is_enabled;
}

// method checks if other properties have changed and these changes affected
// MonID property
bool MonIDPropChanger::isConditionChanged(const IPropertyManager *algo, const std::string &changedPropName) const {
  UNUSED_ARG(changedPropName);
  // is enabled is based on other properties:
  if (!is_enabled)
    return false;
  // read monitors list from the input workspace
  MatrixWorkspace_const_sptr inputWS = algo->getProperty(hostWSname);
  return monitorIdReader(inputWS);
}

// function which modifies the allowed values for the list of monitors.
void MonIDPropChanger::applyChanges(const IPropertyManager *algo, Kernel::Property *const pProp) {
  auto *piProp = dynamic_cast<Kernel::PropertyWithValue<int> *>(pProp);
  if (!piProp) {
    throw(std::invalid_argument("modify allowed value has been called on wrong property"));
  }

  if (iExistingAllowedValues.empty()) {
    MatrixWorkspace_const_sptr inputWS = algo->getProperty(hostWSname);
    int spectra_max(-1);
    if (inputWS) {
      // let's assume that detectors IDs correspond to spectraID --
      // not always the case but often. TODO: should be fixed
      spectra_max = static_cast<int>(inputWS->getNumberHistograms()) + 1;
    }
    piProp->replaceValidator(std::make_shared<Kernel::BoundedValidator<int>>(-1, spectra_max));
  } else {
    piProp->replaceValidator(std::make_shared<Kernel::ListValidator<int>>(iExistingAllowedValues));
  }
}

// read the monitors list from the workspace and try to do it once for any
// particular ws;
bool MonIDPropChanger::monitorIdReader(const MatrixWorkspace_const_sptr &inputWS) const {
  // no workspace
  if (!inputWS)
    return false;

  // no instrument
  Geometry::Instrument_const_sptr pInstr = inputWS->getInstrument();
  if (!pInstr)
    return false;

  // are these monitors really there?
  std::vector<detid_t> monitorIDList = pInstr->getMonitors();
  {
    const auto &specInfo = inputWS->spectrumInfo();
    std::set<detid_t> idsInWorkspace;
    size_t i = 0;
    // Loop over spectra, but finish early if we find everything
    while (i < specInfo.size() && idsInWorkspace.size() < monitorIDList.size()) {
      if (specInfo.isMonitor(i))
        idsInWorkspace.insert(specInfo.detector(i).getID());
      ++i;
    }
    monitorIDList = std::vector<detid_t>(idsInWorkspace.begin(), idsInWorkspace.end());
  }

  if (monitorIDList.empty()) {
    if (iExistingAllowedValues.empty()) {
      return false;
    } else {
      iExistingAllowedValues.clear();
      return true;
    }
  }

  // are known values the same as the values we have just identified?
  if (iExistingAllowedValues.size() != monitorIDList.size()) {
    iExistingAllowedValues.clear();
    iExistingAllowedValues.assign(monitorIDList.begin(), monitorIDList.end());
    return true;
  }
  // the monitor list has the same size as before. Is it equivalent to the
  // existing one?
  bool values_redefined = false;
  for (size_t i = 0; i < monitorIDList.size(); i++) {
    if (iExistingAllowedValues[i] != monitorIDList[i]) {
      values_redefined = true;
      iExistingAllowedValues[i] = monitorIDList[i];
    }
  }
  return values_redefined;
}

bool spectrumDefinitionsMatchTimeIndex(const SpectrumDefinition &specDef, const size_t timeIndex) {
  return std::none_of(specDef.cbegin(), specDef.cend(),
                      [timeIndex](const auto &spec) { return spec.second != timeIndex; });
}

// Register with the algorithm factory
DECLARE_ALGORITHM(NormaliseToMonitor)

using namespace Kernel;
using namespace API;
using std::size_t;

void NormaliseToMonitor::init() {
  // Must be histograms OR one count per bin
  // Must be raw counts
  auto validatorHistSingle = std::make_shared<CompositeValidator>(CompositeRelation::OR);
  validatorHistSingle->add<HistogramValidator>();
  validatorHistSingle->add<SingleCountValidator>();
  auto validator = std::make_shared<CompositeValidator>();
  validator->add(validatorHistSingle);
  validator->add<RawCountValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, validator),
                  "Name of the input workspace. Must be a non-distribution histogram.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "Name to use for the output workspace");
  // should be any spectrum number, but named this property MonitorSpectrum to
  // keep compatibility with previous scripts
  // Can either set a spectrum within the workspace to be the monitor
  // spectrum.....
  declareProperty("MonitorSpectrum", -1,
                  "The spectrum number within the InputWorkspace you want to "
                  "normalize by (It can be a monitor spectrum or a spectrum "
                  "responsible for a group of detectors or monitors)",
                  Direction::InOut);

  // Or take monitor ID to identify the spectrum one wish to use or
  declareProperty("MonitorID", -1,
                  "The MonitorID (detector ID), which defines the monitor's data "
                  "within the InputWorkspace. Will be overridden by the values "
                  "correspondent to MonitorSpectrum field if one is provided "
                  "in the field above.\n"
                  "If workspace do not have monitors, the MonitorID can refer "
                  "to empty data and the field then can accepts any MonitorID "
                  "within the InputWorkspace.");
  // set up the validator, which would verify if spectrum is correct
  setPropertySettings("MonitorID",
                      std::make_unique<MonIDPropChanger>("InputWorkspace", "MonitorSpectrum", "MonitorWorkspace"));

  // ...or provide it in a separate workspace (note: optional WorkspaceProperty)
  declareProperty(std::make_unique<WorkspaceProperty<>>("MonitorWorkspace", "", Direction::Input,
                                                        PropertyMode::Optional, validator),
                  "A workspace containing one or more spectra to normalize the "
                  "InputWorkspace by.");
  setPropertySettings("MonitorWorkspace", std::make_unique<Kernel::EnabledWhenProperty>("MonitorSpectrum", IS_DEFAULT));

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
                      std::make_unique<Kernel::EnabledWhenProperty>("MonitorSpectrum", IS_DEFAULT));

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
  declareProperty("IncludePartialBins", false,
                  "If true and an integration range is set then partial bins at either \n"
                  "end of the integration range are also included");

  declareProperty(std::make_unique<WorkspaceProperty<>>("NormFactorWS", "", Direction::Output, PropertyMode::Optional),
                  "Name of the workspace, containing the normalization factor.\n"
                  "If this name is empty, normalization workspace is not returned. If the "
                  "name coincides with the output workspace name, _normFactor suffix is "
                  "added to this name");
}

void NormaliseToMonitor::exec() {
  // Get the input workspace
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // First check the inputs
  checkProperties(inputWS);

  bool isSingleCountWorkspace = false;
  try {
    isSingleCountWorkspace = (!inputWS->isHistogramData()) && (inputWS->blocksize() == 1);
  } catch (std::length_error &) {
    // inconsistent bin size, not a single count workspace
    isSingleCountWorkspace = false;
  }

  // See if the normalization with integration properties are set.
  const bool integrate = setIntegrationProps(isSingleCountWorkspace);

  if (integrate)
    normaliseByIntegratedCount(inputWS, outputWS, isSingleCountWorkspace);
  else
    normaliseBinByBin(inputWS, outputWS);

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
    if (!integrate)
      m_monitor = extractMonitorSpectra(m_monitor, m_workspaceIndexes);
    setProperty("NormFactorWS", m_monitor);
  }
}

/**
 * Pulls the monitor spectra out of a larger workspace
 * @param ws
 * @param workspaceIndexes The indexes of the spectra to extract
 * @return A workspace containing the spectra requested
 */
MatrixWorkspace_sptr NormaliseToMonitor::extractMonitorSpectra(const MatrixWorkspace_sptr &ws,
                                                               const std::vector<std::size_t> &workspaceIndexes) {
  auto childAlg = createChildAlgorithm("ExtractSpectra");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", ws);
  childAlg->setProperty("WorkspaceIndexList", workspaceIndexes);
  childAlg->executeAsChildAlg();
  MatrixWorkspace_sptr outWS = childAlg->getProperty("OutputWorkspace");
  return outWS;
}

/** Validates input properties.
 *  @return A map of input properties as keys and (error) messages as values.
 */
std::map<std::string, std::string> NormaliseToMonitor::validateInputs() {
  std::map<std::string, std::string> issues;
  // Check where the monitor spectrum should come from
  const Property *monSpecProp = getProperty("MonitorSpectrum");
  const Property *monIDProp = getProperty("MonitorID");
  MatrixWorkspace_const_sptr monWS = getProperty("MonitorWorkspace");
  // something has to be set
  if (monSpecProp->isDefault() && !monWS && monIDProp->isDefault()) {
    const std::string mess("Either MonitorSpectrum, MonitorID or "
                           "MonitorWorkspace has to be provided.");
    issues["MonitorSpectrum"] = mess;
    issues["MonitorID"] = mess;
    issues["MonitorWorkspace"] = mess;
  }

  const double intMin = getProperty("IntegrationRangeMin");
  const double intMax = getProperty("IntegrationRangeMax");
  if (!isEmpty(intMin) && !isEmpty(intMax)) {
    if (intMin > intMax) {
      issues["IntegrationRangeMin"] = "Range minimum set to a larger value than maximum.";
      issues["IntegrationRangeMax"] = "Range maximum set to a smaller value than minimum.";
    }
  }

  if (monWS && monSpecProp->isDefault()) {
    const int monIndex = getProperty("MonitorWorkspaceIndex");
    if (monIndex < 0) {
      issues["MonitorWorkspaceIndex"] = "A workspace index cannot be negative.";
    } else if (monWS->getNumberHistograms() <= static_cast<size_t>(monIndex)) {
      issues["MonitorWorkspaceIndex"] = "The MonitorWorkspace must contain the MonitorWorkspaceIndex.";
    }
    MatrixWorkspace_const_sptr inWS = getProperty("InputWorkspace");
    if (monWS->getInstrument()->getName() != inWS->getInstrument()->getName()) {
      issues["MonitorWorkspace"] = "The Input and Monitor workspaces must come "
                                   "from the same instrument.";
    }
    if (monWS->getAxis(0)->unit()->unitID() != inWS->getAxis(0)->unit()->unitID()) {
      issues["MonitorWorkspace"] = "The Input and Monitor workspaces must have the same unit";
    }
  }

  return issues;
}

/** Makes sure that the input properties are set correctly
 *  @param inputWorkspace The input workspace
 */
void NormaliseToMonitor::checkProperties(const MatrixWorkspace_sptr &inputWorkspace) {

  // Check where the monitor spectrum should come from
  Property *monSpec = getProperty("MonitorSpectrum");
  MatrixWorkspace_sptr monWS = getProperty("MonitorWorkspace");
  Property *monID = getProperty("MonitorID");
  // Is the monitor spectrum within the main input workspace
  const bool inWS = !monSpec->isDefault();
  m_scanInput = inputWorkspace->detectorInfo().isScanning();
  // Or is it in a separate workspace
  bool sepWS{monWS};
  if (m_scanInput && sepWS)
    throw std::runtime_error("Can not currently use a separate monitor "
                             "workspace with a detector scan input workspace.");
  // or monitor ID
  bool monIDs = !monID->isDefault();
  // something has to be set
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
  m_commonBins = inputWorkspace->isCommonBins();

  // Check the monitor spectrum or workspace and extract into new workspace
  m_monitor = sepWS ? getMonitorWorkspace(inputWorkspace) : getInWSMonitorSpectrum(inputWorkspace);

  // Check that the 'monitor' spectrum actually relates to a monitor - warn if
  // not
  try {
    const auto &monitorSpecInfo = m_monitor->spectrumInfo();
    for (const auto workspaceIndex : m_workspaceIndexes)
      if (!monitorSpecInfo.isMonitor(workspaceIndex))
        g_log.warning() << "The spectrum N: " << workspaceIndex << " in MonitorWorkspace does not refer to a monitor.\n"
                        << "Continuing with normalization regardless.";
  } catch (Kernel::Exception::NotFoundError &e) {
    g_log.warning("Unable to check if the spectrum provided relates to a "
                  "monitor - the instrument is not fully specified.\n "
                  "Continuing with normalization regardless.");
    g_log.warning() << "Error was: " << e.what() << "\n";
    if (m_scanInput)
      throw std::runtime_error("Can not continue, spectrum can not be obtained "
                               "for monitor workspace, but the input workspace "
                               "has a detector scan.");
  }
}

/** Checks and retrieves the requested spectrum out of the input workspace
 *
 *  @param inputWorkspace The input workspace
 *  @returns The unchanged input workspace (so that signature is the same as
 *getMonitorWorkspace)
 *  @throw std::runtime_error If the properties are invalid
 */
MatrixWorkspace_sptr NormaliseToMonitor::getInWSMonitorSpectrum(const MatrixWorkspace_sptr &inputWorkspace) {
  // this is the index of the spectra within the workspace and we need to
  // identify it either from DetID or from SpecID
  // size_t spectra_num(-1);
  // try monitor spectrum. If it is specified, it overrides everything
  int monitorSpec = getProperty("MonitorSpectrum");
  if (monitorSpec < 0) {
    // Get hold of the monitor spectrum through detector ID
    int monitorID = getProperty("MonitorID");
    if (monitorID < 0) {
      throw std::runtime_error("Both MonitorSpectrum and MonitorID can not be negative");
    }
    // set spectra of detector's ID of one selected monitor ID
    std::vector<detid_t> detID(1, monitorID);
    // got the index of correspondent spectra (should be only one).
    auto indexList = inputWorkspace->getIndicesFromDetectorIDs(detID);
    if (indexList.empty()) {
      throw std::runtime_error("Can not find spectra, corresponding to the requested monitor ID");
    }
    if (indexList.size() > 1 && !m_scanInput) {
      throw std::runtime_error("More then one spectrum corresponds to the "
                               "requested monitor ID. This is unexpected in a "
                               "non-scanning workspace.");
    }
    m_workspaceIndexes = indexList;
  } else { // monitor spectrum is specified.
    if (m_scanInput)
      throw std::runtime_error("For a scanning input workspace the monitor ID "
                               "must be provided. Normalisation can not be "
                               "performed to a spectrum.");
    const SpectraAxis *axis = dynamic_cast<const SpectraAxis *>(inputWorkspace->getAxis(1));
    if (!axis) {
      throw std::runtime_error("Cannot retrieve monitor spectrum - spectrum "
                               "numbers not attached to workspace");
    }
    auto specs = axis->getSpectraIndexMap();
    if (!specs.count(monitorSpec)) {
      throw std::runtime_error("Input workspace does not contain spectrum "
                               "number given for MonitorSpectrum");
    }
    m_workspaceIndexes = std::vector<size_t>(1, specs[monitorSpec]);
  }
  return inputWorkspace;
}

/** Checks and retrieves the monitor spectrum out of the input workspace
 *  @param inputWorkspace The input workspace
 *  @returns A workspace containing the monitor spectrum only
 */
MatrixWorkspace_sptr NormaliseToMonitor::getMonitorWorkspace(const MatrixWorkspace_sptr &inputWorkspace) {
  MatrixWorkspace_sptr monitorWS = getProperty("MonitorWorkspace");
  const int wsID = getProperty("MonitorWorkspaceIndex");
  m_workspaceIndexes = std::vector<size_t>(1, wsID);
  // In this case we need to test whether the bins in the monitor workspace
  // match
  m_commonBins = (m_commonBins && WorkspaceHelpers::matchingBins(*inputWorkspace, *monitorWS, true));
  // Copy the monitor spectrum because it will get changed
  return monitorWS;
}

/**
 *  @return True if the maximum or minimum values are set
 */

/**
 * Sets the maximum and minimum X values of the monitor spectrum to use for
 * integration
 *
 * @param isSingleCountWorkspace Whether or not the input workspace is point
 *data with single counts per spectrum
 * @return True if the maximum or minimum values are set, or it is a single
 *count workspace
 */
bool NormaliseToMonitor::setIntegrationProps(const bool isSingleCountWorkspace) {
  m_integrationMin = getProperty("IntegrationRangeMin");
  m_integrationMax = getProperty("IntegrationRangeMax");

  // Check if neither of these have been changed from their defaults
  // (EMPTY_DBL())
  if ((isEmpty(m_integrationMin) && isEmpty(m_integrationMax)) && !isSingleCountWorkspace) {
    // Nothing has been set so the user doesn't want to use integration so let's
    // move on
    return false;
  }
  // Yes integration is going to be used...

  // Now check the end X values are within the X value range of the workspace
  if ((isEmpty(m_integrationMin) || m_integrationMin < m_monitor->x(0).front()) && !isSingleCountWorkspace) {
    g_log.warning() << "Integration range minimum set to workspace min: " << m_integrationMin << '\n';
    m_integrationMin = m_monitor->x(0).front();
  }
  if ((isEmpty(m_integrationMax) || m_integrationMax > m_monitor->x(0).back()) && !isSingleCountWorkspace) {
    g_log.warning() << "Integration range maximum set to workspace max: " << m_integrationMax << '\n';
    m_integrationMax = m_monitor->x(0).back();
  }

  // Return indicating that these properties should be used
  return true;
}

/** Carries out a normalization based on the integrated count of the monitor
 * over a range
 * @param inputWorkspace The input workspace
 * @param outputWorkspace The result workspace
 * @param isSingleCountWorkspace Whether or not the input workspace is point
 *data with single counts per spectrum
 */
void NormaliseToMonitor::normaliseByIntegratedCount(const MatrixWorkspace_sptr &inputWorkspace,
                                                    MatrixWorkspace_sptr &outputWorkspace,
                                                    const bool isSingleCountWorkspace) {
  m_monitor = extractMonitorSpectra(m_monitor, m_workspaceIndexes);

  // If single counting no need to integrate, monitor already guaranteed to be a
  // single count
  if (!isSingleCountWorkspace) {
    // Add up all the bins so it's just effectively a series of values with
    // errors
    auto integrate = createChildAlgorithm("Integration");
    integrate->setProperty<MatrixWorkspace_sptr>("InputWorkspace", m_monitor);
    integrate->setProperty("RangeLower", m_integrationMin);
    integrate->setProperty("RangeUpper", m_integrationMax);
    integrate->setProperty<bool>("IncludePartialBins", getProperty("IncludePartialBins"));
    integrate->executeAsChildAlg();
    m_monitor = integrate->getProperty("OutputWorkspace");
  }

  EventWorkspace_sptr inputEvent = std::dynamic_pointer_cast<EventWorkspace>(inputWorkspace);

  if (inputEvent) {
    // Run the divide algorithm explicitly to enable progress reporting
    auto divide = createChildAlgorithm("Divide", 0.0, 1.0);
    divide->setProperty<MatrixWorkspace_sptr>("LHSWorkspace", inputWorkspace);
    divide->setProperty<MatrixWorkspace_sptr>("RHSWorkspace", m_monitor);
    divide->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWorkspace);
    divide->executeAsChildAlg();

    // Get back the result
    outputWorkspace = divide->getProperty("OutputWorkspace");
  } else {
    performHistogramDivision(inputWorkspace, outputWorkspace);
  }
}

/**
 * This performs a similar operation to divide, but is a separate algorithm so
 *that the correct spectra are used in the case of detector scans. This
 *currently does not support event workspaces properly, but should be made to in
 *the future.
 *
 * @param inputWorkspace The workspace with the spectra to divide by the monitor
 * @param outputWorkspace The resulting workspace
 */
void NormaliseToMonitor::performHistogramDivision(const MatrixWorkspace_sptr &inputWorkspace,
                                                  MatrixWorkspace_sptr &outputWorkspace) {
  if (outputWorkspace != inputWorkspace)
    outputWorkspace = inputWorkspace->clone();

  size_t monitorWorkspaceIndex = 0;

  Progress prog(this, 0.0, 1.0, m_workspaceIndexes.size());
  const auto &specInfo = inputWorkspace->spectrumInfo();
  for (const auto workspaceIndex : m_workspaceIndexes) {
    // Errors propagated according to
    // http://docs.mantidproject.org/nightly/concepts/ErrorPropagation.html#error-propagation
    // This is similar to that in MantidAlgorithms::Divide

    prog.report("Performing normalisation");

    size_t timeIndex = 0;
    if (m_scanInput)
      timeIndex = specInfo.spectrumDefinition(workspaceIndex)[0].second;

    const auto newYFactor = 1.0 / m_monitor->histogram(monitorWorkspaceIndex).y()[0];
    const auto divisorError = m_monitor->histogram(monitorWorkspaceIndex).e()[0];
    const double yErrorFactor = pow(divisorError * newYFactor, 2);
    monitorWorkspaceIndex++;

    PARALLEL_FOR_IF(Kernel::threadSafe(*outputWorkspace))
    for (int64_t i = 0; i < int64_t(outputWorkspace->getNumberHistograms()); ++i) {
      PARALLEL_START_INTERRUPT_REGION
      const auto &specDef = specInfo.spectrumDefinition(i);

      if (!spectrumDefinitionsMatchTimeIndex(specDef, timeIndex))
        continue;

      auto hist = outputWorkspace->histogram(i);
      auto &yValues = hist.mutableY();
      auto &eValues = hist.mutableE();

      for (size_t j = 0; j < yValues.size(); ++j) {
        eValues[j] = newYFactor * sqrt(eValues[j] * eValues[j] + yValues[j] * yValues[j] * yErrorFactor);
        yValues[j] *= newYFactor;
      }

      outputWorkspace->setHistogram(i, hist);
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  }
}

/** Carries out the bin-by-bin normalization
 *  @param inputWorkspace The input workspace
 *  @param outputWorkspace The result workspace
 */
void NormaliseToMonitor::normaliseBinByBin(const MatrixWorkspace_sptr &inputWorkspace,
                                           MatrixWorkspace_sptr &outputWorkspace) {
  EventWorkspace_sptr inputEvent = std::dynamic_pointer_cast<EventWorkspace>(inputWorkspace);

  // Only create output workspace if different to input one
  if (outputWorkspace != inputWorkspace) {
    if (inputEvent) {
      outputWorkspace = inputWorkspace->clone();
    } else
      outputWorkspace = create<MatrixWorkspace>(*inputWorkspace);
  }
  auto outputEvent = std::dynamic_pointer_cast<EventWorkspace>(outputWorkspace);

  const auto &inputSpecInfo = inputWorkspace->spectrumInfo();
  const auto &monitorSpecInfo = m_monitor->spectrumInfo();

  const auto specLength = inputWorkspace->blocksize();
  for (auto &workspaceIndex : m_workspaceIndexes) {
    // Get hold of the monitor spectrum
    const auto &monX = m_monitor->binEdges(workspaceIndex);
    auto monY = m_monitor->counts(workspaceIndex);
    auto monE = m_monitor->countStandardDeviations(workspaceIndex);
    size_t timeIndex = 0;
    if (m_scanInput)
      timeIndex = monitorSpecInfo.spectrumDefinition(workspaceIndex)[0].second;
    // Calculate the overall normalization just the once if bins are all
    // matching
    if (m_commonBins)
      this->normalisationFactor(monX, monY, monE);

    const size_t numHists = inputWorkspace->getNumberHistograms();
    // Flag set when a division by 0 is found
    bool hasZeroDivision = false;
    Progress prog(this, 0.0, 1.0, numHists);
    // Loop over spectra
    PARALLEL_FOR_IF(Kernel::threadSafe(*inputWorkspace, *outputWorkspace, *m_monitor))
    for (int64_t i = 0; i < int64_t(numHists); ++i) {
      PARALLEL_START_INTERRUPT_REGION
      prog.report();

      const auto &specDef = inputSpecInfo.spectrumDefinition(i);
      if (!spectrumDefinitionsMatchTimeIndex(specDef, timeIndex))
        continue;

      const auto &X = inputWorkspace->binEdges(i);
      // If not rebinning, just point to our monitor spectra, otherwise create
      // new vectors

      auto Y = (m_commonBins ? monY : Counts(specLength));
      auto E = (m_commonBins ? monE : CountStandardDeviations(specLength));

      if (!m_commonBins) {
        // ConvertUnits can give X vectors of all zeros - skip these, they
        // cause
        // problems
        if (X.back() == 0.0 && X.front() == 0.0)
          continue;
        // Rebin the monitor spectrum to match the binning of the current data
        // spectrum
        VectorHelper::rebinHistogram(monX.rawData(), monY.mutableRawData(), monE.mutableRawData(), X.rawData(),
                                     Y.mutableRawData(), E.mutableRawData(), false);
        // Recalculate the overall normalization factor
        this->normalisationFactor(X, Y, E);
      }

      if (inputEvent) {
        // --- EventWorkspace ---
        EventList &outEL = outputEvent->getSpectrum(i);
        outEL.divide(X.rawData(), Y.mutableRawData(), E.mutableRawData());
      } else {
        // --- Workspace2D ---
        auto &YOut = outputWorkspace->mutableY(i);
        auto &EOut = outputWorkspace->mutableE(i);
        const auto &inY = inputWorkspace->y(i);
        const auto &inE = inputWorkspace->e(i);
        outputWorkspace->setSharedX(i, inputWorkspace->sharedX(i));

        // The code below comes more or less straight out of Divide.cpp
        for (size_t k = 0; k < specLength; ++k) {
          // Get the input Y's
          const double leftY = inY[k];
          const double rightY = Y[k];

          if (rightY == 0.0) {
            hasZeroDivision = true;
          }

          // Calculate result and store in local variable to avoid overwriting
          // original data if output workspace is same as one of the input
          // ones
          const double newY = leftY / rightY;

          if (fabs(rightY) > 1.0e-12 && fabs(newY) > 1.0e-12) {
            const double lhsFactor = (inE[k] < 1.0e-12 || fabs(leftY) < 1.0e-12) ? 0.0 : pow((inE[k] / leftY), 2);
            const double rhsFactor = E[k] < 1.0e-12 ? 0.0 : pow((E[k] / rightY), 2);
            EOut[k] = std::abs(newY) * sqrt(lhsFactor + rhsFactor);
          }

          // Now store the result
          YOut[k] = newY;
        } // end Workspace2D case
      } // end loop over current spectrum

      PARALLEL_END_INTERRUPT_REGION
    } // end loop over spectra
    PARALLEL_CHECK_INTERRUPT_REGION

    if (hasZeroDivision) {
      g_log.warning() << "Division by zero in some of the bins.\n";
    }
    if (inputEvent)
      outputEvent->clearMRU();
  }
}

/** Calculates the overall normalization factor.
 *  This multiplies result by (bin width * sum of monitor counts) / total
 * frame
 * width.
 *  @param X The BinEdges of the workspace
 *  @param Y The Counts of the workspace
 *  @param E The CountStandardDeviations of the workspace
 */
void NormaliseToMonitor::normalisationFactor(const BinEdges &X, Counts &Y, CountStandardDeviations &E) {
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

} // namespace Mantid::Algorithms
