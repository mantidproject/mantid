#include "EnggDiffGSASFittingPresenter.h"
#include "EnggDiffGSASRefinementMethod.h"
#include "MantidQtWidgets/LegacyQwt/QwtHelper.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingPresenter::EnggDiffGSASFittingPresenter(
    std::unique_ptr<IEnggDiffGSASFittingModel> model,
    IEnggDiffGSASFittingView *view,
    boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> multiRunWidget)
    : m_model(std::move(model)), m_multiRunWidget(multiRunWidget), m_view(view),
      m_viewHasClosed(false) {
  auto addMultiRunWidget = m_multiRunWidget->getWidgetAdder();
  (*addMultiRunWidget)(*m_view);
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

  case IEnggDiffGSASFittingPresenter::Start:
    processStart();
    break;

  case IEnggDiffGSASFittingPresenter::ShutDown:
    processShutDown();
    break;
  }
}

void EnggDiffGSASFittingPresenter::displayFitResults(const RunLabel &runLabel) {
  const auto latticeParams = m_model->getLatticeParams(runLabel);
  const auto rwp = m_model->getRwp(runLabel);

  if (!latticeParams || !rwp) {
    m_view->userError("Invalid run identifier",
                      "Unexpectedly tried to display fit results for invalid "
                      "run, run number = " +
                          std::to_string(runLabel.runNumber) + ", bank ID = " +
                          std::to_string(runLabel.bank) +
                          ". Please contact the development team");
    return;
  }

  m_view->displayLatticeParams(*latticeParams);
  m_view->displayRwp(*rwp);
}

Mantid::API::MatrixWorkspace_sptr
EnggDiffGSASFittingPresenter::doPawleyRefinement(
    const Mantid::API::MatrixWorkspace_sptr inputWS, const RunLabel &runLabel,
    const std::string &instParamFile,
    const std::vector<std::string> &phaseFiles, const std::string &pathToGSASII,
    const std::string &GSASIIProjectFile) {
  const auto dMin = m_view->getPawleyDMin();
  const auto negativeWeight = m_view->getPawleyNegativeWeight();

  return m_model->doPawleyRefinement(inputWS, runLabel, instParamFile,
                                     phaseFiles, pathToGSASII,
                                     GSASIIProjectFile, dMin, negativeWeight);
}

Mantid::API::MatrixWorkspace_sptr
EnggDiffGSASFittingPresenter::doRietveldRefinement(
    const Mantid::API::MatrixWorkspace_sptr inputWS, const RunLabel &runLabel,
    const std::string &instParamFile,
    const std::vector<std::string> &phaseFiles, const std::string &pathToGSASII,
    const std::string &GSASIIProjectFile) {
  return m_model->doRietveldRefinement(inputWS, runLabel, instParamFile,
                                       phaseFiles, pathToGSASII,
                                       GSASIIProjectFile);
}

void EnggDiffGSASFittingPresenter::processDoRefinement() {
  const auto runLabel = m_multiRunWidget->getSelectedRunLabel();
  const auto inputWSOptional = m_multiRunWidget->getFocusedRun(runLabel);

  if (!inputWSOptional) {
    m_view->userError(
        "Invalid run selected for refinement",
        "Tried to run refinement on invalid focused run, run number " +
            std::to_string(runLabel.runNumber) + " and bank ID " +
            std::to_string(runLabel.bank) +
            ". Please contact the development team with this message");
    return;
  }

  const auto refinementMethod = m_view->getRefinementMethod();

  const auto instParamFile = m_view->getInstrumentFileName();
  const auto phaseFiles = m_view->getPhaseFileNames();
  const auto pathToGSASII = m_view->getPathToGSASII();
  const auto GSASIIProjectFile = m_view->getGSASIIProjectPath();

  try {
    Mantid::API::MatrixWorkspace_sptr fittedPeaks;

    switch (refinementMethod) {

    case GSASRefinementMethod::PAWLEY:
      fittedPeaks =
          doPawleyRefinement(*inputWSOptional, runLabel, instParamFile,
                             phaseFiles, pathToGSASII, GSASIIProjectFile);
      break;

    case GSASRefinementMethod::RIETVELD:
      fittedPeaks =
          doRietveldRefinement(*inputWSOptional, runLabel, instParamFile,
                               phaseFiles, pathToGSASII, GSASIIProjectFile);
      break;
    }

    m_multiRunWidget->addFittedPeaks(runLabel, fittedPeaks);
  } catch (const std::runtime_error &ex) {
    m_view->userError("Refinement failed", ex.what());
  }
}

void EnggDiffGSASFittingPresenter::processLoadRun() {
  const auto focusedFileNames = m_view->getFocusedFileNames();

  try {
    for (const auto fileName : focusedFileNames) {
      const auto focusedRun = m_model->loadFocusedRun(fileName);
      m_multiRunWidget->addFocusedRun(focusedRun);
    }
  } catch (const std::exception &ex) {
    m_view->userWarning("Could not load file", ex.what());
  }
}

void EnggDiffGSASFittingPresenter::processStart() {}

void EnggDiffGSASFittingPresenter::processShutDown() { m_viewHasClosed = true; }

} // MantidQt
} // CustomInterfaces
