#include "EnggDiffGSASFittingPresenter.h"
#include "EnggDiffGSASRefinementMethod.h"
#include "MantidQtWidgets/LegacyQwt/QwtHelper.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingPresenter::EnggDiffGSASFittingPresenter(
    std::unique_ptr<IEnggDiffGSASFittingModel> model,
    IEnggDiffGSASFittingView *view)
    : m_model(std::move(model)), m_view(view), m_viewHasClosed(false) {}

EnggDiffGSASFittingPresenter::EnggDiffGSASFittingPresenter(
    EnggDiffGSASFittingPresenter &&other)
    : m_model(std::move(other.m_model)), m_view(other.m_view),
      m_viewHasClosed(other.m_viewHasClosed) {}

EnggDiffGSASFittingPresenter &EnggDiffGSASFittingPresenter::
operator=(EnggDiffGSASFittingPresenter &&other) {
  m_model = std::move(other.m_model);
  m_view = std::move(other.m_view);
  return *this;
}

EnggDiffGSASFittingPresenter::~EnggDiffGSASFittingPresenter() {}

void EnggDiffGSASFittingPresenter::notify(
    IEnggDiffGSASFittingPresenter::Notification notif) {

  if (m_viewHasClosed) {
    return;
  }

  switch (notif) {

  case IEnggDiffGSASFittingPresenter::DoRefinement:
    processDoRefinement();
    break;

  case IEnggDiffGSASFittingPresenter::LoadRun:
    processLoadRun();
    break;

  case IEnggDiffGSASFittingPresenter::SelectRun:
    processSelectRun();
    break;

  case IEnggDiffGSASFittingPresenter::Start:
    processStart();
    break;

  case IEnggDiffGSASFittingPresenter::ShutDown:
    processShutDown();
    break;
  }
}

void EnggDiffGSASFittingPresenter::displayFitResults(const RunLabel &runLabel) {
  const auto fittedPeaks = m_model->getFittedPeaks(runLabel);
  const auto latticeParams = m_model->getLatticeParams(runLabel);
  const auto rwp = m_model->getRwp(runLabel);

  if (!fittedPeaks || !latticeParams || !rwp) {
    m_view->userError("Invalid run identifier",
                      "Unexpectedly tried to plot fit results for invalid "
                      "run, run number = " +
                          std::to_string(runLabel.runNumber) + ", bank ID = " +
                          std::to_string(runLabel.bank) +
                          ". Please contact the development team");
    return;
  }

  const auto plottablePeaks = API::QwtHelper::curveDataFromWs(*fittedPeaks);
  m_view->plotCurve(plottablePeaks);

  m_view->displayLatticeParams(*latticeParams);
  m_view->displayRwp(*rwp);
}

boost::optional<std::string> EnggDiffGSASFittingPresenter::doPawleyRefinement(
    const RunLabel &runLabel, const std::string &instParamFile,
    const std::vector<std::string> &phaseFiles, const std::string &pathToGSASII,
    const std::string &GSASIIProjectFile) {
  const auto dMin = m_view->getPawleyDMin();
  const auto negativeWeight = m_view->getPawleyNegativeWeight();

  return m_model->doPawleyRefinement(runLabel, instParamFile, phaseFiles,
                                     pathToGSASII, GSASIIProjectFile, dMin,
                                     negativeWeight);
}

boost::optional<std::string> EnggDiffGSASFittingPresenter::doRietveldRefinement(
    const RunLabel &runLabel, const std::string &instParamFile,
    const std::vector<std::string> &phaseFiles, const std::string &pathToGSASII,
    const std::string &GSASIIProjectFile) {
  return m_model->doRietveldRefinement(runLabel, instParamFile, phaseFiles,
                                       pathToGSASII, GSASIIProjectFile);
}

void EnggDiffGSASFittingPresenter::processDoRefinement() {
  const auto runLabel = m_view->getSelectedRunLabel();

  const auto refinementMethod = m_view->getRefinementMethod();

  const auto instParamFile = m_view->getInstrumentFileName();
  const auto phaseFiles = m_view->getPhaseFileNames();
  const auto pathToGSASII = m_view->getPathToGSASII();
  const auto GSASIIProjectFile = m_view->getGSASIIProjectPath();

  boost::optional<std::string> refinementFailure(boost::none);

  switch (refinementMethod) {

  case GSASRefinementMethod::PAWLEY:
    refinementFailure = doPawleyRefinement(runLabel, instParamFile, phaseFiles,
                                           pathToGSASII, GSASIIProjectFile);
    break;

  case GSASRefinementMethod::RIETVELD:
    refinementFailure = doRietveldRefinement(
        runLabel, instParamFile, phaseFiles, pathToGSASII, GSASIIProjectFile);
    break;
  }

  if (refinementFailure) {
    m_view->userWarning("Refinement failed", *refinementFailure);
  } else {
    updatePlot(runLabel);
  }
}

void EnggDiffGSASFittingPresenter::processLoadRun() {
  const auto focusedFileNames = m_view->getFocusedFileNames();

  for (const auto fileName : focusedFileNames) {
    const auto loadFailure = m_model->loadFocusedRun(fileName);

    if (loadFailure) {
      m_view->userWarning("Load failed", *loadFailure);
    } else {
      const auto runLabels = m_model->getRunLabels();
      m_view->updateRunList(runLabels);
    }
  }
}

void EnggDiffGSASFittingPresenter::processSelectRun() {
  const auto runLabel = m_view->getSelectedRunLabel();
  updatePlot(runLabel);
}

void EnggDiffGSASFittingPresenter::processStart() {}

void EnggDiffGSASFittingPresenter::processShutDown() { m_viewHasClosed = true; }

void EnggDiffGSASFittingPresenter::updatePlot(const RunLabel &runLabel) {
  const auto focusedWSOptional = m_model->getFocusedWorkspace(runLabel);
  if (!focusedWSOptional) {
    m_view->userError("Invalid run identifier",
                      "Tried to access invalid run, runNumber " +
                          std::to_string(runLabel.runNumber) + " and bank ID " +
                          std::to_string(runLabel.bank) +
                          ". Please contact the development team");
    return;
  }
  const auto focusedWS = *focusedWSOptional;

  const auto plottableCurve = API::QwtHelper::curveDataFromWs(focusedWS);

  m_view->resetCanvas();
  m_view->plotCurve(plottableCurve);

  const auto showRefinementResults = m_view->showRefinementResultsSelected();

  if (showRefinementResults && m_model->hasFittedPeaksForRun(runLabel)) {
    displayFitResults(runLabel);
  }
}

} // MantidQt
} // CustomInterfaces
