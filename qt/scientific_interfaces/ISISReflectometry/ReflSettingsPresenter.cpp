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
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
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
OptionsMap ReflSettingsPresenter::getTransmissionOptions() const {

  OptionsMap options;

  if (m_view->experimentSettingsEnabled()) {

    // Add analysis mode
    auto analysisMode = m_view->getAnalysisMode();
    if (!analysisMode.empty())
      options["AnalysisMode"] = QString::fromStdString(analysisMode);

    // Add start overlap
    auto startOv = m_view->getStartOverlap();
    if (!startOv.empty())
      options["StartOverlap"] = QString::fromStdString(startOv);

    // Add end overlap
    auto endOv = m_view->getEndOverlap();
    if (!endOv.empty())
      options["EndOverlap"] = QString::fromStdString(endOv);

    // Add transmission runs
    auto transRuns = this->getTransmissionRuns();
    if (!transRuns.empty())
      options["FirstTransmissionRun"] = QString::fromStdString(transRuns);
  }

  if (m_view->instrumentSettingsEnabled()) {
    // Add monitor integral min
    auto monIntMin = m_view->getMonitorIntegralMin();
    if (!monIntMin.empty())
      options["MonitorIntegrationWavelengthMin"] =
          QString::fromStdString(monIntMin);

    // Add monitor integral max
    auto monIntMax = m_view->getMonitorIntegralMax();
    if (!monIntMax.empty())
      options["MonitorIntegrationWavelengthMax"] =
          QString::fromStdString(monIntMax);

    // Add monitor background min
    auto monBgMin = m_view->getMonitorBackgroundMin();
    if (!monBgMin.empty())
      options["MonitorBackgroundWavelengthMin"] =
          QString::fromStdString(monBgMin);

    // Add monitor background max
    auto monBgMax = m_view->getMonitorBackgroundMax();
    if (!monBgMax.empty())
      options["MonitorBackgroundWavelengthMax"] =
          QString::fromStdString(monBgMax);

    // Add lambda min
    auto lamMin = m_view->getLambdaMin();
    if (!lamMin.empty())
      options["WavelengthMin"] = QString::fromStdString(lamMin);

    // Add lambda max
    auto lamMax = m_view->getLambdaMax();
    if (!lamMax.empty())
      options["WavelengthMax"] = QString::fromStdString(lamMax);

    // Add I0MonitorIndex
    auto I0MonitorIndex = m_view->getI0MonitorIndex();
    if (!I0MonitorIndex.empty())
      options["I0MonitorIndex"] = QString::fromStdString(I0MonitorIndex);

    // Add detector limits
    auto procInst = m_view->getProcessingInstructions();
    if (!procInst.empty()) {
      wrapWithQuotes(procInst);
      options["ProcessingInstructions"] = QString::fromStdString(procInst);
    }
  }

  return options;
}

/** Returns global options for 'ReflectometryReductionOneAuto'
 * @return :: Global options for 'ReflectometryReductionOneAuto'
 */
OptionsMap ReflSettingsPresenter::getReductionOptions() const {

  OptionsMap options;

  if (m_view->experimentSettingsEnabled()) {

    // Add analysis mode
    auto analysisMode = m_view->getAnalysisMode();
    if (!analysisMode.empty())
      options["AnalysisMode"] = QString::fromStdString(analysisMode);

    // Add CRho
    auto crho = m_view->getCRho();
    if (!crho.empty()) {
      wrapWithQuotes(crho);
      options["CRho"] = QString::fromStdString(crho);
    }

    // Add CAlpha
    auto calpha = m_view->getCAlpha();
    if (!calpha.empty()) {
      wrapWithQuotes(calpha);
      options["CAlpha"] = QString::fromStdString(calpha);
    }

    // Add CAp
    auto cap = m_view->getCAp();
    if (!cap.empty()) {
      wrapWithQuotes(cap);
      options["CAp"] = QString::fromStdString(cap);
    }

    // Add CPp
    auto cpp = m_view->getCPp();
    if (!cpp.empty()) {
      wrapWithQuotes(cpp);
      options["CPp"] = QString::fromStdString(cpp);
    }

    // Add direct beam
    auto dbnr = m_view->getDirectBeam();
    if (!dbnr.empty()) {
      wrapWithQuotes(dbnr);
      options["RegionOfDirectBeam"] = QString::fromStdString(dbnr);
    }

    // Add polarisation corrections
    auto polCorr = m_view->getPolarisationCorrections();
    if (!polCorr.empty())
      options["PolarizationAnalysis"] = QString::fromStdString(polCorr);

    // Add scale factor
    auto scaleFactor = m_view->getScaleFactor();
    if (!scaleFactor.empty())
      options["ScaleFactor"] = QString::fromStdString(scaleFactor);

    // Add momentum transfer limits
    auto qTransStep = m_view->getMomentumTransferStep();
    if (!qTransStep.empty()) {
      options["MomentumTransferStep"] = QString::fromStdString(qTransStep);
    }

    // Add start overlap
    auto startOv = m_view->getStartOverlap();
    if (!startOv.empty())
      options["StartOverlap"] = QString::fromStdString(startOv);

    // Add end overlap
    auto endOv = m_view->getEndOverlap();
    if (!endOv.empty())
      options["EndOverlap"] = QString::fromStdString(endOv);

    // Add transmission runs
    auto transRuns = this->getTransmissionRuns();
    if (!transRuns.empty())
      options["FirstTransmissionRun"] = QString::fromStdString(transRuns);
  }

  if (m_view->instrumentSettingsEnabled()) {

    // Add integrated monitors option
    auto intMonCheck = m_view->getIntMonCheck();
    if (!intMonCheck.empty())
      options["NormalizeByIntegratedMonitors"] =
          QString::fromStdString(intMonCheck);

    // Add monitor integral min
    auto monIntMin = m_view->getMonitorIntegralMin();
    if (!monIntMin.empty())
      options["MonitorIntegrationWavelengthMin"] =
          QString::fromStdString(monIntMin);

    // Add monitor integral max
    auto monIntMax = m_view->getMonitorIntegralMax();
    if (!monIntMax.empty())
      options["MonitorIntegrationWavelengthMax"] =
          QString::fromStdString(monIntMax);

    // Add monitor background min
    auto monBgMin = m_view->getMonitorBackgroundMin();
    if (!monBgMin.empty())
      options["MonitorBackgroundWavelengthMin"] =
          QString::fromStdString(monBgMin);

    // Add monitor background max
    auto monBgMax = m_view->getMonitorBackgroundMax();
    if (!monBgMax.empty())
      options["MonitorBackgroundWavelengthMax"] =
          QString::fromStdString(monBgMax);

    // Add lambda min
    auto lamMin = m_view->getLambdaMin();
    if (!lamMin.empty())
      options["WavelengthMin"] = QString::fromStdString(lamMin);

    // Add lambda max
    auto lamMax = m_view->getLambdaMax();
    if (!lamMax.empty())
      options["WavelengthMax"] = QString::fromStdString(lamMax);

    // Add I0MonitorIndex
    auto I0MonitorIndex = m_view->getI0MonitorIndex();
    if (!I0MonitorIndex.empty())
      options["I0MonitorIndex"] = QString::fromStdString(I0MonitorIndex);

    // Add detector limits
    auto procInst = m_view->getProcessingInstructions();
    if (!procInst.empty()) {
      wrapWithQuotes(procInst);
      options["ProcessingInstructions"] = QString::fromStdString(procInst);
    }

    // Add correction type
    auto correctionType = m_view->getDetectorCorrectionType();
    if (!correctionType.empty())
      options["DetectorCorrectionType"] =
          QString::fromStdString(correctionType);
  }

  return options;
}

/** Gets the user-specified transmission runs from the view
*
* @return :: the transmission runs string
*/
std::string ReflSettingsPresenter::getTransmissionRuns() const {
  if (m_view->experimentSettingsEnabled())
    return m_view->getTransmissionRuns();

  return std::string();
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

/** Wraps string with quote marks if it does not already have them
* @param str :: [input] The string to be wrapped
*/
void ReflSettingsPresenter::wrapWithQuotes(std::string &str) const {
  if (str.front() != '\"')
    str = "\"" + str;
  if (str.back() != '\"')
    str = str + "\"";
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
