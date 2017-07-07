#include "MantidQtCustomInterfaces/Reflectometry/ReflSettingsPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsView.h"
#include "MantidQtMantidWidgets/AlgorithmHintStrategy.h"

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
std::string ReflSettingsPresenter::getTransmissionOptions() const {

  std::vector<std::string> options;

  if (m_view->experimentSettingsEnabled()) {

    // Add analysis mode
    auto analysisMode = m_view->getAnalysisMode();
    if (!analysisMode.empty())
      options.push_back("AnalysisMode=" + analysisMode);

    // Add start overlap
    auto startOv = m_view->getStartOverlap();
    if (!startOv.empty())
      options.push_back("StartOverlap=" + startOv);

    // Add end overlap
    auto endOv = m_view->getEndOverlap();
    if (!endOv.empty())
      options.push_back("EndOverlap=" + endOv);
  }

  if (m_view->instrumentSettingsEnabled()) {
    // Add monitor integral min
    auto monIntMin = m_view->getMonitorIntegralMin();
    if (!monIntMin.empty())
      options.push_back("MonitorIntegrationWavelengthMin=" + monIntMin);

    // Add monitor integral max
    auto monIntMax = m_view->getMonitorIntegralMax();
    if (!monIntMax.empty())
      options.push_back("MonitorIntegrationWavelengthMax=" + monIntMax);

    // Add monitor background min
    auto monBgMin = m_view->getMonitorBackgroundMin();
    if (!monBgMin.empty())
      options.push_back("MonitorBackgroundWavelengthMin=" + monBgMin);

    // Add monitor background max
    auto monBgMax = m_view->getMonitorBackgroundMax();
    if (!monBgMax.empty())
      options.push_back("MonitorBackgroundWavelengthMax=" + monBgMax);

    // Add lambda min
    auto lamMin = m_view->getLambdaMin();
    if (!lamMin.empty())
      options.push_back("WavelengthMin=" + lamMin);

    // Add lambda max
    auto lamMax = m_view->getLambdaMax();
    if (!lamMax.empty())
      options.push_back("WavelengthMax=" + lamMax);

    // Add I0MonitorIndex
    auto I0MonitorIndex = m_view->getI0MonitorIndex();
    if (!I0MonitorIndex.empty())
      options.push_back("I0MonitorIndex=" + I0MonitorIndex);

    // Add detector limits
    auto procInst = m_view->getProcessingInstructions();
    if (!procInst.empty()) {
      wrapWithQuotes(procInst);
      options.push_back("ProcessingInstructions=" + procInst);
    }
  }

  return boost::algorithm::join(options, ",");
}

/** Returns global options for 'ReflectometryReductionOneAuto'
 * @return :: Global options for 'ReflectometryReductionOneAuto'
 */
std::string ReflSettingsPresenter::getReductionOptions() const {

  std::vector<std::string> options;

  if (m_view->experimentSettingsEnabled()) {

    // Add analysis mode
    auto analysisMode = m_view->getAnalysisMode();
    if (!analysisMode.empty())
      options.push_back("AnalysisMode=" + analysisMode);

    // Add CRho
    auto crho = m_view->getCRho();
    if (!crho.empty()) {
      wrapWithQuotes(crho);
      options.push_back("CRho=" + crho);
    }

    // Add CAlpha
    auto calpha = m_view->getCAlpha();
    if (!calpha.empty()) {
      wrapWithQuotes(calpha);
      options.push_back("CAlpha=" + calpha);
    }

    // Add CAp
    auto cap = m_view->getCAp();
    if (!cap.empty()) {
      wrapWithQuotes(cap);
      options.push_back("CAp=" + cap);
    }

    // Add CPp
    auto cpp = m_view->getCPp();
    if (!cpp.empty()) {
      wrapWithQuotes(cpp);
      options.push_back("CPp=" + cpp);
    }

    // Add direct beam
    auto dbnr = m_view->getDirectBeam();
    if (!dbnr.empty()) {
      wrapWithQuotes(dbnr);
      options.push_back("RegionOfDirectBeam=" + dbnr);
    }

    // Add polarisation corrections
    auto polCorr = m_view->getPolarisationCorrections();
    if (!polCorr.empty())
      options.push_back("PolarizationAnalysis=" + polCorr);

    // Add scale factor
    auto scaleFactor = m_view->getScaleFactor();
    if (!scaleFactor.empty())
      options.push_back("ScaleFactor=" + scaleFactor);

    // Add momentum transfer limits
    auto qTransStep = m_view->getMomentumTransferStep();
    if (!qTransStep.empty()) {
      options.push_back("MomentumTransferStep=" + qTransStep);
    }

    // Add start overlap
    auto startOv = m_view->getStartOverlap();
    if (!startOv.empty())
      options.push_back("StartOverlap=" + startOv);

    // Add end overlap
    auto endOv = m_view->getEndOverlap();
    if (!endOv.empty())
      options.push_back("EndOverlap=" + endOv);

    // Add transmission runs
    auto transRuns = this->getTransmissionRuns(true);
    if (!transRuns.empty()) {
      std::vector<std::string> splitRuns;
      boost::split(splitRuns, transRuns, boost::is_any_of(","));
      options.push_back("FirstTransmissionRun=TRANS_" + splitRuns[0]);
      if (splitRuns.size() > 1)
        options.push_back("SecondTransmissionRun=TRANS_" + splitRuns[1]);
    }
  }

  if (m_view->instrumentSettingsEnabled()) {

    // Add integrated monitors option
    auto intMonCheck = m_view->getIntMonCheck();
    if (!intMonCheck.empty())
      options.push_back("NormalizeByIntegratedMonitors=" + intMonCheck);

    // Add monitor integral min
    auto monIntMin = m_view->getMonitorIntegralMin();
    if (!monIntMin.empty())
      options.push_back("MonitorIntegrationWavelengthMin=" + monIntMin);

    // Add monitor integral max
    auto monIntMax = m_view->getMonitorIntegralMax();
    if (!monIntMax.empty())
      options.push_back("MonitorIntegrationWavelengthMax=" + monIntMax);

    // Add monitor background min
    auto monBgMin = m_view->getMonitorBackgroundMin();
    if (!monBgMin.empty())
      options.push_back("MonitorBackgroundWavelengthMin=" + monBgMin);

    // Add monitor background max
    auto monBgMax = m_view->getMonitorBackgroundMax();
    if (!monBgMax.empty())
      options.push_back("MonitorBackgroundWavelengthMax=" + monBgMax);

    // Add lambda min
    auto lamMin = m_view->getLambdaMin();
    if (!lamMin.empty())
      options.push_back("WavelengthMin=" + lamMin);

    // Add lambda max
    auto lamMax = m_view->getLambdaMax();
    if (!lamMax.empty())
      options.push_back("WavelengthMax=" + lamMax);

    // Add I0MonitorIndex
    auto I0MonitorIndex = m_view->getI0MonitorIndex();
    if (!I0MonitorIndex.empty())
      options.push_back("I0MonitorIndex=" + I0MonitorIndex);

    // Add detector limits
    auto procInst = m_view->getProcessingInstructions();
    if (!procInst.empty()) {
      wrapWithQuotes(procInst);
      options.push_back("ProcessingInstructions=" + procInst);
    }

    // Add correction type
    auto correctionType = m_view->getDetectorCorrectionType();
    if (!correctionType.empty())
      options.push_back("DetectorCorrectionType=" + correctionType);
  }

  return boost::algorithm::join(options, ",");
}

/** Receives specified transmission runs from the view and loads them into the
*ADS. Returns a string with transmission runs so that they are considered in the
*reduction
*
* @param loadRuns :: If true, will try to load transmission runs as well
* @return :: transmission run(s) as a string that will be used for the reduction
*/
std::string ReflSettingsPresenter::getTransmissionRuns(bool loadRuns) const {

  auto runs = m_view->getTransmissionRuns();
  if (runs.empty())
    return "";

  std::vector<std::string> transRuns;
  boost::split(transRuns, runs, boost::is_any_of(","));

  if (transRuns.size() > 2)
    throw std::invalid_argument("Only one transmission run or two "
                                "transmission runs separated by ',' "
                                "are allowed.");

  if (loadRuns) {
    for (const auto &run : transRuns) {
      if (AnalysisDataService::Instance().doesExist("TRANS_" + run))
        continue;
      // Load transmission runs and put them in the ADS
      IAlgorithm_sptr alg =
          AlgorithmManager::Instance().create("LoadISISNexus");
      alg->setProperty("Filename", run);
      alg->setPropertyValue("OutputWorkspace", "TRANS_" + run);
      alg->execute();
    }
  }

  return runs;
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