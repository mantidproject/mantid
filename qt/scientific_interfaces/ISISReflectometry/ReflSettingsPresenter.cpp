#include "ReflSettingsPresenter.h"
#include "IReflSettingsTabPresenter.h"
#include "IReflSettingsView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace Mantid::Geometry;
using namespace MantidQt::MantidWidgets::DataProcessor;

/** Constructor
* @param view :: The view we are handling
*/
ReflSettingsPresenter::ReflSettingsPresenter(IReflSettingsView *view)
    : m_view(view) {

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
  case IReflSettingsPresenter::SummationTypeChanged:
    handleSummationTypeChange();
    break;
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

bool ReflSettingsPresenter::hasReductionTypes(
    const std::string &summationType) const {
  return summationType == "SumInQ";
}

void ReflSettingsPresenter::handleSummationTypeChange() {
  auto summationType = m_view->getSummationType();
  m_view->setReductionTypeEnabled(hasReductionTypes(summationType));
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

/** Returns global options for 'CreateTransmissionWorkspaceAuto'
 * @return :: Global options for 'CreateTransmissionWorkspaceAuto'
 */
OptionsQMap ReflSettingsPresenter::getTransmissionOptions() const {

  OptionsQMap options;

  if (m_view->experimentSettingsEnabled()) {
    addIfNotEmpty(options, "AnalysisMode", m_view->getAnalysisMode());
    addIfNotEmpty(options, "StartOverlap", m_view->getStartOverlap());
    addIfNotEmpty(options, "EndOverlap", m_view->getEndOverlap());
    addIfNotEmpty(options, "FirstTransmissionRun", this->getTransmissionRuns());
  }

  if (m_view->instrumentSettingsEnabled()) {
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
    addIfNotEmpty(options, "ProcessingInstructions",
                  m_view->getProcessingInstructions());
  }

  return options;
}

void ReflSettingsPresenter::addIfNotEmpty(OptionsQMap &options,
                                          const QString &key,
                                          const QString &value) const {
  if (!key.isEmpty())
    options[key] = value;
}

void ReflSettingsPresenter::addIfNotEmpty(OptionsQMap &options,
                                          const QString &key,
                                          const std::string &value) const {
  if (!key.isEmpty())
    options[key] = QString::fromStdString(value);
}

/** Returns global options for 'ReflectometryReductionOneAuto'
 * @return :: Global options for 'ReflectometryReductionOneAuto'
 */
OptionsQMap ReflSettingsPresenter::getReductionOptions() const {

  OptionsQMap options;

  if (m_view->experimentSettingsEnabled()) {
    addIfNotEmpty(options, "AnalysisMode", m_view->getAnalysisMode());
    addIfNotEmpty(options, "CRho", m_view->getCRho());
    addIfNotEmpty(options, "CAlpha", m_view->getCAlpha());
    addIfNotEmpty(options, "CAp", m_view->getCAp());
    addIfNotEmpty(options, "CPp", m_view->getCPp());
    addIfNotEmpty(options, "PolarizationAnalysis",
                  m_view->getPolarisationCorrections());
    addIfNotEmpty(options, "ScaleFactor", m_view->getScaleFactor());
    addIfNotEmpty(options, "MomentumTransferStep",
                  m_view->getMomentumTransferStep());
    addIfNotEmpty(options, "StartOverlap", m_view->getStartOverlap());
    addIfNotEmpty(options, "EndOverlap", m_view->getEndOverlap());
    addIfNotEmpty(options, "FirstTransmissionRun", this->getTransmissionRuns());

    auto summationType = m_view->getSummationType();
    addIfNotEmpty(options, "SummationType", summationType);

    if (hasReductionTypes(summationType))
      addIfNotEmpty(options, "ReductionType", m_view->getReductionType());
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
    addIfNotEmpty(options, "ProcessingInstructions",
                  m_view->getProcessingInstructions());
    addIfNotEmpty(options, "DetectorCorrectionType",
                  m_view->getDetectorCorrectionType());
  }

  return options;
}

/** Gets the user-specified transmission runs from the view
*
* @return :: the transmission runs string
*/
std::string ReflSettingsPresenter::getTransmissionRuns() const {
  return m_view->getTransmissionRuns();
}

/** Returns global options for 'Stitch1DMany'
 * @return :: Global options for 'Stitch1DMany'
 */
std::string ReflSettingsPresenter::getStitchOptions() const {

  if (m_view->experimentSettingsEnabled())
    return m_view->getStitchOptions();

  return std::string();
}

/** Creates hints for 'Stitch1DMany'
*/
void ReflSettingsPresenter::createStitchHints() {

  // The algorithm
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Stitch1DMany");
  // The blacklist
  std::set<std::string> blacklist = {"InputWorkspaces", "OutputWorkspace",
                                     "OutputWorkspace"};
  AlgorithmHintStrategy strategy(alg, blacklist);

  m_view->createStitchHints(strategy.createHints());
}

/** Fills experiment settings with default values
*/
void ReflSettingsPresenter::getExpDefaults() {
  // Algorithm and instrument
  auto alg = createReductionAlg();
  auto inst = createEmptyInstrument(m_currentInstrumentName);

  // Collect all default values and set them in view
  std::vector<std::string> defaults(8);
  defaults[0] = alg->getPropertyValue("AnalysisMode");
  defaults[1] = alg->getPropertyValue("PolarizationAnalysis");

  auto cRho = inst->getStringParameter("crho");
  if (!cRho.empty())
    defaults[2] = cRho[0];

  auto cAlpha = inst->getStringParameter("calpha");
  if (!cAlpha.empty())
    defaults[3] = cAlpha[0];

  auto cAp = inst->getStringParameter("cAp");
  if (!cAp.empty())
    defaults[4] = cAp[0];

  auto cPp = inst->getStringParameter("cPp");
  if (!cPp.empty())
    defaults[5] = cPp[0];

  if (m_currentInstrumentName != "SURF" && m_currentInstrumentName != "CRISP") {
    defaults[6] = boost::lexical_cast<std::string>(
        inst->getNumberParameter("TransRunStartOverlap")[0]);

    defaults[7] = boost::lexical_cast<std::string>(
        inst->getNumberParameter("TransRunEndOverlap")[0]);
  }

  m_view->setExpDefaults(defaults);
}

/** Fills instrument settings with default values
*/
void ReflSettingsPresenter::getInstDefaults() {
  // Algorithm and instrument
  auto alg = createReductionAlg();
  auto inst = createEmptyInstrument(m_currentInstrumentName);

  // Collect all default values
  std::vector<double> defaults_double(8);
  defaults_double[0] = boost::lexical_cast<double>(
      alg->getPropertyValue("NormalizeByIntegratedMonitors"));
  defaults_double[1] = inst->getNumberParameter("MonitorIntegralMin")[0];
  defaults_double[2] = inst->getNumberParameter("MonitorIntegralMax")[0];
  defaults_double[3] = inst->getNumberParameter("MonitorBackgroundMin")[0];
  defaults_double[4] = inst->getNumberParameter("MonitorBackgroundMax")[0];
  defaults_double[5] = inst->getNumberParameter("LambdaMin")[0];
  defaults_double[6] = inst->getNumberParameter("LambdaMax")[0];
  defaults_double[7] = inst->getNumberParameter("I0MonitorIndex")[0];

  std::vector<std::string> defaults_str(1);
  defaults_str[0] = alg->getPropertyValue("DetectorCorrectionType");

  m_view->setInstDefaults(defaults_double, defaults_str);
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
}
}
