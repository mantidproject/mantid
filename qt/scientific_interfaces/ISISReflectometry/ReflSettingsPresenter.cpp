#include "ReflSettingsPresenter.h"
#include "IReflSettingsTabPresenter.h"
#include "IReflSettingsView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"
#include "First.h"
#include "GetInstrumentParameter.h"
#include "ExperimentOptionDefaults.h"
#include "InstrumentOptionDefaults.h"

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

/** Returns global options for 'CreateTransmissionWorkspaceAuto'. Note that
 * this must include all applicable options, even if they are empty, because
 * the GenericDataProcessorPresenter has no other way of knowing which options
 * are applicable to the preprocessing algorithm (e.g. for options that might
 * be specified on the Runs tab instead). We get around this by providing the
 * full list here and overriding them if they also exist on the Runs tab.
 * @todo This is not idea and really we should just be passing through the
 * non-preprocessed transmission runs to ReflectometryReductionOneAuto, which
 * would then run CreateTransmissionWorkspaceAuto as a child algorithm and do
 * all of this for us. However, the transmission runs would need to be loaded
 * prior to running the processing algorithm, which is not currently possible.
 * @return :: Global options for 'CreateTransmissionWorkspaceAuto'
 */
OptionsQMap ReflSettingsPresenter::getTransmissionOptions() const {

  OptionsQMap options;

  if (m_view->experimentSettingsEnabled()) {

    // Add analysis mode
    auto analysisMode = m_view->getAnalysisMode();
    options["AnalysisMode"] = QString::fromStdString(analysisMode);

    // Add start overlap
    auto startOv = m_view->getStartOverlap();
    options["StartOverlap"] = QString::fromStdString(startOv);

    // Add end overlap
    auto endOv = m_view->getEndOverlap();
    options["EndOverlap"] = QString::fromStdString(endOv);
  }

  if (m_view->instrumentSettingsEnabled()) {
    // Add monitor integral min
    auto monIntMin = m_view->getMonitorIntegralMin();
    options["MonitorIntegrationWavelengthMin"] =
        QString::fromStdString(monIntMin);

    // Add monitor integral max
    auto monIntMax = m_view->getMonitorIntegralMax();
    options["MonitorIntegrationWavelengthMax"] =
        QString::fromStdString(monIntMax);

    // Add monitor background min
    auto monBgMin = m_view->getMonitorBackgroundMin();
    options["MonitorBackgroundWavelengthMin"] =
        QString::fromStdString(monBgMin);

    // Add monitor background max
    auto monBgMax = m_view->getMonitorBackgroundMax();
    options["MonitorBackgroundWavelengthMax"] =
        QString::fromStdString(monBgMax);

    // Add lambda min
    auto lamMin = m_view->getLambdaMin();
    options["WavelengthMin"] = QString::fromStdString(lamMin);

    // Add lambda max
    auto lamMax = m_view->getLambdaMax();
    options["WavelengthMax"] = QString::fromStdString(lamMax);

    // Add I0MonitorIndex
    auto I0MonitorIndex = m_view->getI0MonitorIndex();
    options["I0MonitorIndex"] = QString::fromStdString(I0MonitorIndex);

    // Add detector limits
    auto procInst = m_view->getProcessingInstructions();
    options["ProcessingInstructions"] = QString::fromStdString(procInst);
  }

  return options;
}

/** Returns global options for 'ReflectometryReductionOneAuto'
 * @return :: Global options for 'ReflectometryReductionOneAuto'
 */
OptionsQMap ReflSettingsPresenter::getReductionOptions() const {

  OptionsQMap options;

  if (m_view->experimentSettingsEnabled()) {

    // Add analysis mode
    auto analysisMode = m_view->getAnalysisMode();
    if (!analysisMode.empty())
      options["AnalysisMode"] = QString::fromStdString(analysisMode);

    // Add CRho
    auto crho = m_view->getCRho();
    if (!crho.empty())
      options["CRho"] = QString::fromStdString(crho);

    // Add CAlpha
    auto calpha = m_view->getCAlpha();
    if (!calpha.empty())
      options["CAlpha"] = QString::fromStdString(calpha);

    // Add CAp
    auto cap = m_view->getCAp();
    if (!cap.empty())
      options["CAp"] = QString::fromStdString(cap);

    // Add CPp
    auto cpp = m_view->getCPp();
    if (!cpp.empty())
      options["CPp"] = QString::fromStdString(cpp);

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
    if (!procInst.empty())
      options["ProcessingInstructions"] = QString::fromStdString(procInst);

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

template <typename T>
T firstFromParameterFileOr(Instrument_const_sptr instrument,
                           std::string const &parameterName,
                           T ifEmptyOrWrongType) {
  return first(getInstrumentParameter<T>(instrument, parameterName))
      .value_or(ifEmptyOrWrongType);
}

/** Fills experiment settings with default values
*/
void ReflSettingsPresenter::getExpDefaults() {
  auto alg = createReductionAlg();
  auto instrument = createEmptyInstrument(m_currentInstrumentName);

  auto experimentDefaults = ExperimentOptionDefaults();

  experimentDefaults.AnalysisMode = firstFromParameterFileOr<std::string>(
      instrument, "AnalysisMode", alg->getPropertyValue("AnalysisMode"));
  experimentDefaults.PolarizationAnalysis =
      alg->getPropertyValue("PolarizationAnalysis");
  experimentDefaults.CRho =
      firstFromParameterFileOr<std::string>(instrument, "crho", "");
  experimentDefaults.CAlpha =
      firstFromParameterFileOr<std::string>(instrument, "calpha", "");
  experimentDefaults.CAp =
      firstFromParameterFileOr<std::string>(instrument, "cAp", "");
  experimentDefaults.CPp =
      firstFromParameterFileOr<std::string>(instrument, "cPp", "");
  experimentDefaults.MomentumTransferStep =
      firstFromParameterFileOr(instrument, "dQ/Q", 0.0);
  experimentDefaults.ScaleFactor =
      firstFromParameterFileOr(instrument, "Scale", 0.0);
  experimentDefaults.StitchParams =
      firstFromParameterFileOr<std::string>(instrument, "Params", "");

  if (m_currentInstrumentName != "SURF" && m_currentInstrumentName != "CRISP") {
    experimentDefaults.TransRunStartOverlap =
        firstFromParameterFileOr(instrument, "TransRunStartOverlap", 0.0);
    experimentDefaults.TransRunEndOverlap =
        firstFromParameterFileOr(instrument, "TransRunEndOverlap", 0.0);
  }

  m_view->setExpDefaults(experimentDefaults);
}

/** Fills instrument settings with default values
*/
void ReflSettingsPresenter::getInstDefaults() {
  auto alg = createReductionAlg();
  auto instrument = createEmptyInstrument(m_currentInstrumentName);

  auto instrumentDefaults = InstrumentOptionDefaults();
  instrumentDefaults.NormalizeByIntegratedMonitors =
      firstFromParameterFileOr(instrument, "IntegratedMonitors",
                               boost::lexical_cast<bool>(alg->getPropertyValue(
                                   "NormalizeByIntegratedMonitors")));
  instrumentDefaults.MonitorIntegralMin =
      firstFromParameterFileOr(instrument, "MonitorIntegralMin", 0.0);

  instrumentDefaults.MonitorIntegralMax =
      firstFromParameterFileOr(instrument, "MonitorIntegralMax", 0.0);

  instrumentDefaults.MonitorBackgroundMin =
      firstFromParameterFileOr(instrument, "MonitorBackgroundMin", 0.0);

  instrumentDefaults.MonitorBackgroundMax =
      firstFromParameterFileOr(instrument, "MonitorBackgroundMax", 0.0);

  instrumentDefaults.LambdaMin =
      firstFromParameterFileOr(instrument, "LambdaMin", 0.0);

  instrumentDefaults.LambdaMax =
      firstFromParameterFileOr(instrument, "LambdaMax", 0.0);

  instrumentDefaults.I0MonitorIndex =
      firstFromParameterFileOr(instrument, "I0MonitorIndex", 0);

  instrumentDefaults.ProcessingInstructions =
      firstFromParameterFileOr<std::string>(instrument,
                                            "ProcessingInstructions", "");

  instrumentDefaults.DetectorCorrectionType =
      firstFromParameterFileOr<std::string>(instrument,
                                            "DetectorCorrectionType", "");

  m_view->setInstDefaults(instrumentDefaults);
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
