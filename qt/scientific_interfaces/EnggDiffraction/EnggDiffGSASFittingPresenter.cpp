// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "EnggDiffGSASFittingPresenter.h"
#include "EnggDiffGSASRefinementMethod.h"
#include "MantidQtWidgets/Plotting/Qwt/QwtHelper.h"

namespace {

std::string addRunNumberToGSASIIProjectFile(
    const std::string &filename,
    const MantidQt::CustomInterfaces::RunLabel &runLabel) {
  const auto dotPosition = filename.find_last_of(".");
  return filename.substr(0, dotPosition) + "_" +
         runLabel.runNumber + "_" +
         std::to_string(runLabel.bank) +
         filename.substr(dotPosition, filename.length());
}

} // anonymous namespace

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffGSASFittingPresenter::EnggDiffGSASFittingPresenter(
    std::unique_ptr<IEnggDiffGSASFittingModel> model,
    IEnggDiffGSASFittingView *view,
    boost::shared_ptr<IEnggDiffMultiRunFittingWidgetPresenter> multiRunWidget,
    boost::shared_ptr<IEnggDiffractionParam> mainSettings)
    : m_model(std::move(model)), m_multiRunWidget(multiRunWidget),
      m_mainSettings(mainSettings), m_view(view), m_viewHasClosed(false) {}

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

  case IEnggDiffGSASFittingPresenter::RefineAll:
    processRefineAll();
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

std::vector<GSASIIRefineFitPeaksParameters>
EnggDiffGSASFittingPresenter::collectAllInputParameters() const {
  const auto runLabels = m_multiRunWidget->getAllRunLabels();
  std::vector<GSASIIRefineFitPeaksParameters> inputParams;
  std::vector<std::string> GSASIIProjectFiles;
  inputParams.reserve(runLabels.size());

  const auto refinementMethod = m_view->getRefinementMethod();
  const auto instParamFile = m_view->getInstrumentFileName();
  const auto phaseFiles = m_view->getPhaseFileNames();
  const auto pathToGSASII = m_view->getPathToGSASII();
  const auto GSASIIProjectFile = m_view->getGSASIIProjectPath();
  if (runLabels.size() == 1) {
    GSASIIProjectFiles = std::vector<std::string>({GSASIIProjectFile});
  } else {
    GSASIIProjectFiles.reserve(runLabels.size());
    for (const auto &runLabel : runLabels) {
      GSASIIProjectFiles.emplace_back(
          addRunNumberToGSASIIProjectFile(GSASIIProjectFile, runLabel));
    }
  }

  const auto dMin = m_view->getPawleyDMin();
  const auto negativeWeight = m_view->getPawleyNegativeWeight();
  const auto xMin = m_view->getXMin();
  const auto xMax = m_view->getXMax();
  const auto refineSigma = m_view->getRefineSigma();
  const auto refineGamma = m_view->getRefineGamma();

  for (size_t i = 0; i < runLabels.size(); i++) {
    const auto &runLabel = runLabels[i];
    const auto inputWS = *(m_multiRunWidget->getFocusedRun(runLabel));

    inputParams.emplace_back(inputWS, runLabel, refinementMethod, instParamFile,
                             phaseFiles, pathToGSASII, GSASIIProjectFiles[i],
                             dMin, negativeWeight, xMin, xMax, refineSigma,
                             refineGamma);
  }
  return inputParams;
}

GSASIIRefineFitPeaksParameters
EnggDiffGSASFittingPresenter::collectInputParameters(
    const RunLabel &runLabel,
    const Mantid::API::MatrixWorkspace_sptr inputWS) const {
  const auto refinementMethod = m_view->getRefinementMethod();
  const auto instParamFile = m_view->getInstrumentFileName();
  const auto phaseFiles = m_view->getPhaseFileNames();
  const auto pathToGSASII = m_view->getPathToGSASII();
  const auto GSASIIProjectFile = m_view->getGSASIIProjectPath();

  const auto dMin = m_view->getPawleyDMin();
  const auto negativeWeight = m_view->getPawleyNegativeWeight();
  const auto xMin = m_view->getXMin();
  const auto xMax = m_view->getXMax();
  const auto refineSigma = m_view->getRefineSigma();
  const auto refineGamma = m_view->getRefineGamma();

  return GSASIIRefineFitPeaksParameters(inputWS, runLabel, refinementMethod,
                                        instParamFile, phaseFiles, pathToGSASII,
                                        GSASIIProjectFile, dMin, negativeWeight,
                                        xMin, xMax, refineSigma, refineGamma);
}

void EnggDiffGSASFittingPresenter::displayFitResults(const RunLabel &runLabel) {
  const auto latticeParams = m_model->getLatticeParams(runLabel);
  const auto rwp = m_model->getRwp(runLabel);
  const auto sigma = m_model->getSigma(runLabel);
  const auto gamma = m_model->getGamma(runLabel);

  if (!latticeParams || !rwp || !sigma || !gamma) {
    m_view->userError("Invalid run identifier",
                      "Unexpectedly tried to display fit results for invalid "
                      "run, run number = " +
                          runLabel.runNumber +
                          ", bank ID = " + std::to_string(runLabel.bank) +
                          ". Please contact the development team");
    return;
  }

  m_view->displayLatticeParams(*latticeParams);
  m_view->displayRwp(*rwp);
  m_view->displaySigma(*sigma);
  m_view->displayGamma(*gamma);
}

void EnggDiffGSASFittingPresenter::doRefinements(
    const std::vector<GSASIIRefineFitPeaksParameters> &params) {
  m_model->doRefinements(params);
}

void EnggDiffGSASFittingPresenter::notifyRefinementsComplete(
    Mantid::API::IAlgorithm_sptr alg,
    const std::vector<GSASIIRefineFitPeaksOutputProperties>
        &refinementResultSets) {
  if (!m_viewHasClosed) {
    const auto numRuns = refinementResultSets.size();

    if (numRuns > 1) {
      std::vector<RunLabel> runLabels;
      runLabels.reserve(numRuns);
      for (const auto &refinementResults : refinementResultSets) {
        runLabels.emplace_back(refinementResults.runLabel);
      }
      m_model->saveRefinementResultsToHDF5(
          alg, refinementResultSets,
          m_mainSettings->userHDFMultiRunFilename(runLabels));
    }

    m_view->setEnabled(true);
    m_view->showStatus("Ready");
  }
}

void EnggDiffGSASFittingPresenter::notifyRefinementCancelled() {
  if (!m_viewHasClosed) {
    m_view->setEnabled(true);
    m_view->showStatus("Ready");
  }
}

void EnggDiffGSASFittingPresenter::notifyRefinementFailed(
    const std::string &failureMessage) {
  if (!m_viewHasClosed) {
    m_view->setEnabled(true);
    m_view->userWarning("Refinement failed", failureMessage);
    m_view->showStatus("Refinement failed");
  }
}

void EnggDiffGSASFittingPresenter::notifyRefinementSuccessful(
    const Mantid::API::IAlgorithm_sptr successfulAlgorithm,
    const GSASIIRefineFitPeaksOutputProperties &refinementResults) {
  if (!m_viewHasClosed) {
    m_view->showStatus("Saving refinement results");
    const auto filename = m_mainSettings->userHDFRunFilename(
        refinementResults.runLabel.runNumber);

    try {
      m_model->saveRefinementResultsToHDF5(successfulAlgorithm,
                                           {refinementResults}, filename);
    } catch (std::exception &e) {
      m_view->userWarning(
          "Could not save refinement results",
          std::string("Refinement was successful but saving results to "
                      "HDF5 failed for the following reason:\n") +
              e.what());
    }
    m_view->setEnabled(true);
    m_view->showStatus("Ready");

    m_multiRunWidget->addFittedPeaks(refinementResults.runLabel,
                                     refinementResults.fittedPeaksWS);
    displayFitResults(refinementResults.runLabel);
  }
}

void EnggDiffGSASFittingPresenter::processDoRefinement() {
  const auto runLabel = m_multiRunWidget->getSelectedRunLabel();
  if (!runLabel) {
    m_view->userWarning("No run selected",
                        "Please select a run to do refinement on");
    return;
  }

  const auto inputWSOptional = m_multiRunWidget->getFocusedRun(*runLabel);
  if (!inputWSOptional) {
    m_view->userError(
        "Invalid run selected for refinement",
        "Tried to run refinement on invalid focused run, run number " +
            runLabel->runNumber + " and bank ID " +
            std::to_string(runLabel->bank) +
            ". Please contact the development team with this message");
    return;
  }

  m_view->showStatus("Refining run");
  const auto refinementParams =
      collectInputParameters(*runLabel, *inputWSOptional);

  m_view->setEnabled(false);
  doRefinements({refinementParams});
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

void EnggDiffGSASFittingPresenter::processRefineAll() {
  const auto refinementParams = collectAllInputParameters();
  if (refinementParams.size() == 0) {
    m_view->userWarning("No runs loaded",
                        "Please load at least one run before refining");
    return;
  }
  m_view->showStatus("Refining run");
  m_view->setEnabled(false);
  doRefinements(refinementParams);
}

void EnggDiffGSASFittingPresenter::processSelectRun() {
  const auto runLabel = m_multiRunWidget->getSelectedRunLabel();
  if (runLabel && m_model->hasFitResultsForRun(*runLabel)) {
    displayFitResults(*runLabel);
  }
}

void EnggDiffGSASFittingPresenter::processStart() {
  auto addMultiRunWidget = m_multiRunWidget->getWidgetAdder();
  (*addMultiRunWidget)(*m_view);
  m_view->showStatus("Ready");
}

void EnggDiffGSASFittingPresenter::processShutDown() { m_viewHasClosed = true; }

} // namespace CustomInterfaces
} // namespace MantidQt
