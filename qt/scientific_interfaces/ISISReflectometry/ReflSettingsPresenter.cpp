// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReflSettingsPresenter.h"
#include "ExperimentOptionDefaults.h"
#include "IReflSettingsTabPresenter.h"
#include "IReflSettingsView.h"
#include "InstrumentOptionDefaults.h"
#include "InstrumentParameters.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"
#include "ValueOr.h"
#include <type_traits>

#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace Mantid::Geometry;
using namespace MantidQt::MantidWidgets::DataProcessor;

/** Constructor
 * @param view :: The view we are handling
 * @param group :: The number of the group this settings presenter's settings
 * correspond to.
 */
ReflSettingsPresenter::ReflSettingsPresenter(IReflSettingsView *view, int group)
    : m_view(view), m_group(group) {

  // Create the 'HintingLineEdits'
  createStitchHints();
}

/** Destructor
 */
ReflSettingsPresenter::~ReflSettingsPresenter() {}

/** Used by the view to tell the presenter something has changed
 *
 * @param flag :: A flag used by the view to tell the presenter what happened
 */
void ReflSettingsPresenter::notify(IReflSettingsPresenter::Flag flag) {
  switch (flag) {
  case IReflSettingsPresenter::ExpDefaultsFlag:
    getExpDefaults();
    break;
  case IReflSettingsPresenter::InstDefaultsFlag:
    getInstDefaults();
    break;
  case IReflSettingsPresenter::SettingsChangedFlag:
    handleSettingsChanged();
    break;
  case IReflSettingsPresenter::SummationTypeChanged:
    handleSummationTypeChange();
    break;
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

void ReflSettingsPresenter::handleSettingsChanged() {
  m_tabPresenter->settingsChanged(m_group);
}

void ReflSettingsPresenter::acceptTabPresenter(
    IReflSettingsTabPresenter *tabPresenter) {
  m_tabPresenter = tabPresenter;
}

bool ReflSettingsPresenter::hasReductionTypes(
    const std::string &summationType) const {
  return summationType == "SumInQ";
}

bool ReflSettingsPresenter::hasIncludePartialBinsOption(
    const std::string &summationType) const {
  return summationType == "SumInQ";
}

void ReflSettingsPresenter::handleSummationTypeChange() {
  auto summationType = m_view->getSummationType();
  m_view->setReductionTypeEnabled(hasReductionTypes(summationType));
  m_view->setIncludePartialBinsEnabled(
      hasIncludePartialBinsOption(summationType));
}

/** Sets the current instrument name and changes accessibility status of
 * the polarisation corrections option in the view accordingly
 * @param instName :: [input] The name of the instrument to set to
 */
void ReflSettingsPresenter::setInstrumentName(const std::string &instName) {
  m_currentInstrumentName = instName;
  bool enable = instName != "INTER" && instName != "SURF";
  m_view->setIsPolCorrEnabled(enable);
  m_view->setPolarisationOptionsEnabled(enable);
}

OptionsQMap ReflSettingsPresenter::transmissionOptionsMap() const {
  OptionsQMap options;
  addTransmissionOptions(options, {"AnalysisMode", "StartOverlap", "EndOverlap",
                                   "MonitorIntegrationWavelengthMin",
                                   "MonitorIntegrationWavelengthMax",
                                   "MonitorBackgroundWavelengthMin",
                                   "MonitorBackgroundWavelengthMax",
                                   "WavelengthMin", "WavelengthMax",
                                   "I0MonitorIndex", "ProcessingInstructions"});
  return options;
}

/** Get the processing instructinons.
 * @return : the processing instructions, or an empty string if not set
 */
QString ReflSettingsPresenter::getProcessingInstructions() const {
  // the processing instructions are set in the per-angle options table. We
  // only set them if there is a default (i.e. not linked to an angle).
  auto options = getDefaultOptions();
  if (options.count("ProcessingInstructions"))
    return options["ProcessingInstructions"].toString();

  return QString();
}

/** Returns global options for 'CreateTransmissionWorkspaceAuto'. Note that
 * this must include all applicable options, even if they are empty, because
 * the GenericDataProcessorPresenter has no other way of knowing which options
 * are applicable to the preprocessing algorithm (e.g. for options that might
 * be specified on the Runs tab instead). We get around this by providing the
 * full list here and overriding them if they also exist on the Runs tab.
 *
 * @todo This is not idea and really we should just be passing through the
 * non-preprocessed transmission runs to ReflectometryReductionOneAuto, which
 * would then run CreateTransmissionWorkspaceAuto as a child algorithm and do
 * all of this for us. However, the transmission runs would need to be loaded
 * prior to running the processing algorithm, which is not currently possible.
 * @return :: Global options for 'CreateTransmissionWorkspaceAuto'
 */
OptionsQMap ReflSettingsPresenter::getTransmissionOptions() const {

  // This step is necessessary until the issue above is addressed.
  // Otherwise either group of options may be missed out by
  // experimentSettingsEnabled or instrumentSettingsEnabled being disabled.
  OptionsQMap options = transmissionOptionsMap();

  if (m_view->experimentSettingsEnabled()) {
    setTransmissionOption(options, "AnalysisMode", m_view->getAnalysisMode());
    setTransmissionOption(options, "StartOverlap", m_view->getStartOverlap());
    setTransmissionOption(options, "EndOverlap", m_view->getEndOverlap());
    setTransmissionOption(options, "ProcessingInstructions",
                          getProcessingInstructions());
  }

  if (m_view->instrumentSettingsEnabled()) {
    setTransmissionOption(options, "NormalizeByIntegratedMonitors",
                          m_view->getIntMonCheck());
    setTransmissionOption(options, "MonitorIntegrationWavelengthMin",
                          m_view->getMonitorIntegralMin());
    setTransmissionOption(options, "MonitorIntegrationWavelengthMax",
                          m_view->getMonitorIntegralMax());
    setTransmissionOption(options, "MonitorBackgroundWavelengthMin",
                          m_view->getMonitorBackgroundMin());
    setTransmissionOption(options, "MonitorBackgroundWavelengthMax",
                          m_view->getMonitorBackgroundMax());
    setTransmissionOption(options, "WavelengthMin", m_view->getLambdaMin());
    setTransmissionOption(options, "WavelengthMax", m_view->getLambdaMax());
    setTransmissionOption(options, "I0MonitorIndex",
                          m_view->getI0MonitorIndex());
  }

  return options;
}

void ReflSettingsPresenter::setTransmissionOption(OptionsQMap &options,
                                                  const QString &key,
                                                  const QString &value) const {
  options[key] = value;
}

void ReflSettingsPresenter::setTransmissionOption(
    OptionsQMap &options, const QString &key, const std::string &value) const {
  options[key] = QString::fromStdString(value);
}

void ReflSettingsPresenter::addTransmissionOptions(
    OptionsQMap &options, std::initializer_list<QString> keys) const {
  for (auto &&key : keys)
    setTransmissionOption(options, key, QString());
}

void ReflSettingsPresenter::addIfNotEmpty(OptionsQMap &options,
                                          const QString &key,
                                          const QString &value) const {
  if (!value.isEmpty())
    setTransmissionOption(options, key, value);
}

void ReflSettingsPresenter::addIfNotEmpty(OptionsQMap &options,
                                          const QString &key,
                                          const std::string &value) const {
  if (!value.empty())
    setTransmissionOption(options, key, value);
}

QString ReflSettingsPresenter::asAlgorithmPropertyBool(bool value) {
  return value ? "1" : "0";
}

void ReflSettingsPresenter::onReductionResumed() { m_view->disableAll(); }

void ReflSettingsPresenter::onReductionPaused() { m_view->enableAll(); }

/** Returns global options for 'ReflectometryReductionOneAuto'
 * @return :: Global options for 'ReflectometryReductionOneAuto'
 * @throws :: if the settings the user entered are invalid
 */
OptionsQMap ReflSettingsPresenter::getReductionOptions() const {

  OptionsQMap options;

  if (m_view->experimentSettingsEnabled()) {
    addIfNotEmpty(options, "AnalysisMode", m_view->getAnalysisMode());
    auto const pa = m_view->getPolarisationCorrections();
    addIfNotEmpty(options, "PolarizationAnalysis", pa);
    if (pa == "PA") {
      addIfNotEmpty(options, "CRho", m_view->getCRho());
      addIfNotEmpty(options, "CAlpha", m_view->getCAlpha());
      addIfNotEmpty(options, "CAp", m_view->getCAp());
      addIfNotEmpty(options, "CPp", m_view->getCPp());
    } else if (pa == "PNR") {
      addIfNotEmpty(options, "CAp", m_view->getCAp());
      addIfNotEmpty(options, "CPp", m_view->getCPp());
    }
    addIfNotEmpty(options, "FloodCorrection", m_view->getFloodCorrection());
    addIfNotEmpty(options, "FloodWorkspace", m_view->getFloodWorkspace());
    addIfNotEmpty(options, "StartOverlap", m_view->getStartOverlap());
    addIfNotEmpty(options, "EndOverlap", m_view->getEndOverlap());

    auto summationType = m_view->getSummationType();
    addIfNotEmpty(options, "SummationType", summationType);

    if (hasReductionTypes(summationType))
      addIfNotEmpty(options, "ReductionType", m_view->getReductionType());

    setTransmissionOption(options, "Debug",
                          asAlgorithmPropertyBool(m_view->getDebugOption()));
    auto const includePartialBins =
        asAlgorithmPropertyBool(m_view->getIncludePartialBins());
    options["IncludePartialBins"] = includePartialBins;

    auto defaultOptions = getDefaultOptions();
    for (auto iter = defaultOptions.begin(); iter != defaultOptions.end();
         ++iter)
      addIfNotEmpty(options, iter.key(), iter.value().toString());
  }

  if (m_view->instrumentSettingsEnabled()) {
    addIfNotEmpty(options, "NormalizeByIntegratedMonitors",
                  m_view->getIntMonCheck());
    addIfNotEmpty(options, "MonitorIntegrationWavelengthMin",
                  m_view->getMonitorIntegralMin());
    addIfNotEmpty(options, "MonitorIntegrationWavelengthMax",
                  m_view->getMonitorIntegralMax());
    addIfNotEmpty(options, "MonitorBackgroundWavelengthMin",
                  m_view->getMonitorBackgroundMin());
    addIfNotEmpty(options, "MonitorBackgroundWavelengthMax",
                  m_view->getMonitorBackgroundMax());
    addIfNotEmpty(options, "WavelengthMin", m_view->getLambdaMin());
    addIfNotEmpty(options, "WavelengthMax", m_view->getLambdaMax());
    addIfNotEmpty(options, "I0MonitorIndex", m_view->getI0MonitorIndex());
    addIfNotEmpty(options, "DetectorCorrectionType",
                  m_view->getDetectorCorrectionType());
    auto const correctDetectors =
        asAlgorithmPropertyBool(m_view->detectorCorrectionEnabled());
    options["CorrectDetectors"] = correctDetectors;
  }

  return options;
}

/** Check whether per-angle transmission runs are specified
 */
bool ReflSettingsPresenter::hasPerAngleOptions() const {
  // Check the setting is enabled
  if (!m_view->experimentSettingsEnabled())
    return false;

  // Check we have some entries in the table
  auto runsPerAngle = m_view->getPerAngleOptions();
  if (runsPerAngle.empty())
    return false;

  // To save confusion, we only allow EITHER a default transmission runs string
  // OR multiple per-angle strings. Therefore if there is a default set there
  // cannot be per-angle runs.
  if (!getDefaultOptions().empty())
    return false;

  // Ok, we have some entries and they're not defaults, so assume they're valid
  // per-angle settings (we'll check that the angles parse ok when we come
  // to use them).
  return true;
}

/** Gets the default user-specified transmission runs from the view. Default
 * runs are those without an angle (i.e. the angle is blank)
 * @return :: the transmission run(s) as a string of comma-separated values
 * @throws :: if the settings the user entered are invalid
 */
OptionsQMap ReflSettingsPresenter::getDefaultOptions() const {
  if (!m_view->experimentSettingsEnabled())
    return OptionsQMap();

  // Values are entered as a map of angle to transmission runs. Loop
  // through them, checking for the required angle
  auto runsPerAngle = m_view->getPerAngleOptions();
  auto iter = runsPerAngle.find("");
  if (iter != runsPerAngle.end()) {
    // We found an empty angle. Check there is only one entry in the
    // table (to save confusion, if the user specifies a default, it
    // must be the only item in the table).
    if (runsPerAngle.size() > 1)
      throw std::runtime_error("Please only specify one set of transmission "
                               "runs when using a default (i.e. where the "
                               "angle is empty)");
    else
      return runsPerAngle.begin()->second;
  }

  // If not found, return an empty string
  return OptionsQMap();
}

/** Gets the user-specified transmission runs from the view
 * @param angleToFind :: the run angle that transmission runs are valid for
 * @return :: the transmission run(s) as a string of comma-separated values
 */
OptionsQMap
ReflSettingsPresenter::getOptionsForAngle(const double angleToFind) const {
  OptionsQMap result;

  if (!hasPerAngleOptions())
    return result;

  // Values are entered as a map of angle to transmission runs
  auto runsPerAngle = m_view->getPerAngleOptions();

  // We use a generous tolerance to check the angle because values
  // from the log are not that accurate
  const double tolerance = 0.01 + Mantid::Kernel::Tolerance;

  // Loop through all values and find the closest that is within the tolerance
  double smallestDist = std::numeric_limits<double>::max();
  for (auto kvp : runsPerAngle) {
    auto angleStr = kvp.first;
    auto values = kvp.second;

    // Convert the angle to a double
    double angle = 0.0;
    try {
      angle = std::stod(angleStr);
    } catch (std::invalid_argument &e) {
      throw std::runtime_error(std::string("Error parsing angle: ") + e.what());
    }

    // Use the closest result to the required angle that meets the given
    // tolerance
    double dist = std::abs(angle - angleToFind);
    if (dist <= tolerance) {
      if (dist < smallestDist) {
        result = values;
        smallestDist = dist;
      }
    }
  }

  // If the angle was not found, return the default (which may be empty
  // if no default was set in the table).
  return result;
}

/** Returns global options for 'Stitch1DMany'
 * @return :: Global options for 'Stitch1DMany'
 */
std::string ReflSettingsPresenter::getStitchOptions() const {

  if (m_view->experimentSettingsEnabled())
    return m_view->getStitchOptions();
  else
    return "";
}

/** Creates hints for 'Stitch1DMany'
 */
void ReflSettingsPresenter::createStitchHints() {
  auto blacklist =
      std::vector<std::string>({"InputWorkspaces", "OutputWorkspace"});
  AlgorithmHintStrategy strategy("Stitch1DMany", blacklist);
  m_view->createStitchHints(strategy.createHints());
}

/** Fills experiment settings with default values
 */
void ReflSettingsPresenter::getExpDefaults() {
  auto alg = createReductionAlg();
  auto instrument = createEmptyInstrument(m_currentInstrumentName);
  auto parameters = InstrumentParameters(instrument);

  auto defaults = ExperimentOptionDefaults();

  defaults.AnalysisMode =
      value_or(parameters.optional<std::string>("AnalysisMode"),
               alg->getPropertyValue("AnalysisMode"));
  defaults.PolarizationAnalysis =
      value_or(parameters.optional<std::string>("PolarizationAnalysis"),
               alg->getPropertyValue("PolarizationAnalysis"));

  defaults.SummationType =
      value_or(parameters.optional<std::string>("SummationType"),
               alg->getPropertyValue("SummationType"));

  defaults.ReductionType =
      value_or(parameters.optional<std::string>("ReductionType"),
               alg->getPropertyValue("ReductionType"));

  defaults.IncludePartialBins =
      value_or(parameters.optional<bool>("IncludePartialBins"),
               alg->getProperty("IncludePartialBins"));

  defaults.CRho = value_or(parameters.optional<std::string>("crho"), "1");
  defaults.CAlpha = value_or(parameters.optional<std::string>("calpha"), "1");
  defaults.CAp = value_or(parameters.optional<std::string>("cAp"), "1");
  defaults.CPp = value_or(parameters.optional<std::string>("cPp"), "1");

  defaults.MomentumTransferMin = parameters.optional<double>("Q min");
  defaults.MomentumTransferMax = parameters.optional<double>("Q max");
  defaults.MomentumTransferStep = parameters.optional<double>("dQ/Q");
  defaults.ScaleFactor = parameters.optional<double>("Scale");
  defaults.ProcessingInstructions =
      parameters.optional<std::string>("ProcessingInstructions");
  defaults.StitchParams = parameters.optional<std::string>("Stitch1DMany");

  if (m_currentInstrumentName != "SURF" && m_currentInstrumentName != "CRISP") {
    defaults.TransRunStartOverlap =
        parameters.mandatory<double>("TransRunStartOverlap");
    defaults.TransRunEndOverlap =
        parameters.mandatory<double>("TransRunEndOverlap");
  } else {
    defaults.TransRunStartOverlap =
        parameters.optional<double>("TransRunStartOverlap");
    defaults.TransRunEndOverlap =
        parameters.optional<double>("TransRunEndOverlap");
  }

  m_view->setExpDefaults(std::move(defaults));

  if (parameters.hasTypeErrors() || parameters.hasMissingValues()) {
    m_view->showOptionLoadErrors(parameters.typeErrors(),
                                 parameters.missingValues());
  }
}

/** Fills instrument settings with default values
 */
void ReflSettingsPresenter::getInstDefaults() {
  auto alg = createReductionAlg();
  auto instrument = createEmptyInstrument(m_currentInstrumentName);
  auto parameters = InstrumentParameters(instrument);
  auto defaults = InstrumentOptionDefaults();

  defaults.NormalizeByIntegratedMonitors =
      value_or(parameters.optional<bool>("IntegratedMonitors"),
               alg->getProperty("NormalizeByIntegratedMonitors"));
  defaults.MonitorIntegralMin =
      parameters.mandatory<double>("MonitorIntegralMin");
  defaults.MonitorIntegralMax =
      parameters.mandatory<double>("MonitorIntegralMax");
  defaults.MonitorBackgroundMin =
      parameters.mandatory<double>("MonitorBackgroundMin");
  defaults.MonitorBackgroundMax =
      parameters.mandatory<double>("MonitorBackgroundMax");
  defaults.LambdaMin = parameters.mandatory<double>("LambdaMin");
  defaults.LambdaMax = parameters.mandatory<double>("LambdaMax");
  defaults.I0MonitorIndex =
      parameters.mandatoryVariant<int, double>("I0MonitorIndex");
  defaults.CorrectDetectors =
      value_or(parameters.optional<bool>("CorrectDetectors"),
               alg->getProperty("CorrectDetectors"));
  defaults.DetectorCorrectionType =
      value_or(parameters.optional<std::string>("DetectorCorrectionType"),
               alg->getPropertyValue("DetectorCorrectionType"));

  m_view->setInstDefaults(std::move(defaults));

  if (parameters.hasTypeErrors() || parameters.hasMissingValues()) {
    m_view->showOptionLoadErrors(parameters.typeErrors(),
                                 parameters.missingValues());
  }
}

/** Generates and returns an instance of the ReflectometryReductionOneAuto
 * algorithm
 * @return :: ReflectometryReductionOneAuto algorithm
 */
IAlgorithm_sptr ReflSettingsPresenter::createReductionAlg() {
  return AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
}

/** Creates and returns an example empty instrument given an instrument name
 * @return :: Empty instrument of a name
 */
Instrument_const_sptr
ReflSettingsPresenter::createEmptyInstrument(const std::string &instName) {
  IAlgorithm_sptr loadInst =
      AlgorithmManager::Instance().create("LoadEmptyInstrument");
  loadInst->setChild(true);
  loadInst->setProperty("OutputWorkspace", "outWs");
  loadInst->setProperty("InstrumentName", instName);
  loadInst->execute();
  MatrixWorkspace_const_sptr ws = loadInst->getProperty("OutputWorkspace");
  return ws->getInstrument();
}
} // namespace CustomInterfaces
} // namespace MantidQt
