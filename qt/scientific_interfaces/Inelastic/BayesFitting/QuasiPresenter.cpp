// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QuasiPresenter.h"

#include "QuasiModel.h"
#include "QuasiView.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

namespace {
Mantid::Kernel::Logger g_log("Quasi");

auto &config = Mantid::Kernel::ConfigService::Instance();
} // namespace

namespace MantidQt::CustomInterfaces {

QuasiPresenter::QuasiPresenter(QWidget *parent, std::unique_ptr<API::IAlgorithmRunner> algorithmRunner,
                               std::unique_ptr<IQuasiModel> model, IQuasiView *view)
    : BayesFittingTab(parent, std::move(algorithmRunner)), m_model(std::move(model)), m_view(view) {
  m_view->subscribe(this);

  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_view->getRunView()));
}

void QuasiPresenter::handleSampleInputReady(std::string const &workspaceName) {
  m_view->enableView(true);
  m_model->setSample(workspaceName);
  auto const sampleWorkspace = m_model->sample();
  if (!sampleWorkspace) {
    return;
  }
  m_view->setPreviewSpectrumMax(sampleWorkspace->getNumberHistograms() - 1u);

  updateMiniPlot();

  m_view->setXRange(MantidWidgets::WorkspaceUtils::getXRangeFromWorkspace(sampleWorkspace));
}

void QuasiPresenter::handleResolutionInputReady(std::string const &workspaceName) {
  m_view->enableView(true);
  m_view->enableUseResolution(m_model->isResolution(workspaceName));
  m_model->setResolution(workspaceName);
}

void QuasiPresenter::handleFileAutoLoaded() {
  m_view->enableView(true);
  m_runPresenter->setRunText("Run");
}

void QuasiPresenter::handlePreviewSpectrumChanged() { updateMiniPlot(); }

void QuasiPresenter::handlePlotCurrentPreview() {
  auto const errorBars = SettingsHelper::externalPlotErrorBars();
  auto const previewSpectrum = m_view->previewSpectrum();

  if (m_view->hasSpectrum("fit 1")) {
    auto const fitGroup = m_model->outputFitGroup();
    if (!fitGroup) {
      return;
    }
    m_plotter->plotSpectra(fitGroup->getNames().at(previewSpectrum),
                           m_view->programName() == "Lorentzians" ? "0-4" : "0-2", errorBars);
  } else if (m_view->hasSpectrum("Sample")) {
    m_plotter->plotSpectra(m_view->sampleName(), std::to_string(previewSpectrum), errorBars);
  }
}

void QuasiPresenter::handleSaveClicked() {
  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algorithmQueue;

  if (auto const fitGroup = m_model->outputFitGroup()) {
    algorithmQueue.push_back(m_model->setupSaveAlgorithm(fitGroup));
  }
  if (auto const result = m_model->outputResult()) {
    algorithmQueue.push_back(m_model->setupSaveAlgorithm(result));
  }
  if (auto const probability = m_model->outputProbability()) {
    algorithmQueue.push_back(m_model->setupSaveAlgorithm(probability));
  }

  m_algorithmRunner->execute(algorithmQueue);
}

void QuasiPresenter::handlePlotClicked() {
  auto const errorBars = SettingsHelper::externalPlotErrorBars();

  auto const plotName = m_view->plotName();
  auto const programName = m_view->programName();

  if ((plotName == "all" || plotName == "prob") && programName == "Lorentzians") {
    if (auto const probabilityWorkspace = m_model->outputProbability()) {
      m_plotter->plotSpectra(probabilityWorkspace->getName(), "1-2", errorBars);
    }
  }

  auto const resultWorkspace = m_model->outputResult();
  if (!resultWorkspace) {
    return;
  }

  auto const numSpectra = resultWorkspace->getNumberHistograms();
  for (auto const &paramName : {"amplitude", "fwhm", "beta", "gamma"}) {
    if (plotName != paramName && plotName != "all") {
      continue;
    }
    std::vector<std::size_t> spectraIndices = {};
    for (auto i = 0u; i < numSpectra; ++i) {
      auto const axisLabel = Mantid::Kernel::Strings::toLower(resultWorkspace->getAxis(1)->label(i));
      if (axisLabel.find(paramName) == std::string::npos) {
        continue;
      }
      if (programName != "Lorentzians") {
        m_plotter->plotSpectra(resultWorkspace->getName(), std::to_string(i), errorBars);
        continue;
      }
      spectraIndices.emplace_back(i);
      if (spectraIndices.size() != 3) {
        continue;
      }
      auto const workspaceIndices = std::to_string(spectraIndices[0]) + "," + std::to_string(spectraIndices[1]) + "," +
                                    std::to_string(spectraIndices[2]);
      m_plotter->plotSpectra(resultWorkspace->getName(), workspaceIndices, errorBars);
    }
  }
}

void QuasiPresenter::handleValidation(IUserInputValidator *validator) const {
  validator->checkDataSelectorIsValid("Sample", m_view->sampleSelector());
  validator->checkDataSelectorIsValid("Resolution", m_view->resolutionSelector());

  // check that the ResNorm file is valid if we are using it
  if (m_view->useResolution()) {
    validator->checkDataSelectorIsValid("ResNorm", m_view->resNormSelector());
  }

  // check fixed width file exists
  auto const *fixWidthFinder = m_view->fixWidthFileFinder();
  if (m_view->fixWidth() && !fixWidthFinder->isValid()) {
    validator->checkFileFinderWidgetIsValid("Width", fixWidthFinder);
  }

  // check eMin and eMax values
  if (m_view->eMin() >= m_view->eMax())
    validator->addErrorMessage("EMin must be strictly less than EMax.\n");

  // Validate program
  if (m_view->programName() != "Stretched Exponential") {
    return;
  }
  if (!m_model->isResolution(m_view->resolutionName())) {
    validator->addErrorMessage("Stretched Exponential program can only be used with "
                               "a resolution file.");
  }
}

void QuasiPresenter::handleRun() {
  auto const saveDirectory = config.getString("defaultsave.directory");
  if (saveDirectory.empty() && m_view->displaySaveDirectoryMessage()) {
    m_runPresenter->setRunEnabled(true);
    return;
  }

  m_view->watchADS(false);

  auto const sampleName = m_view->sampleName();
  auto const resolutionName = m_view->resolutionName();
  auto const background = m_view->backgroundName();

  auto program = m_view->programName();
  program = program == "Lorentzians" ? "QL" : "QSe";

  auto const eMin = m_view->eMin();
  auto const eMax = m_view->eMax();

  // Temporary developer flag to allow the testing of quickBayes in the Bayes fitting interface
  auto const useQuickBayes = SettingsHelper::hasDevelopmentFlag("quickbayes");

  // Construct an output base name for the output workspaces
  auto const resType = resolutionName.substr(resolutionName.length() - 3);
  auto const programName = program == "QL" ? resType == "res" ? "QLr" : "QLd" : program;
  auto const algoType = useQuickBayes ? "_quickbayes" : "_quasielasticbayes";
  auto const baseName = sampleName.substr(0, sampleName.length() - 3) + programName + algoType;

  API::IConfiguredAlgorithm_sptr bayesQuasiAlgorithm;
  if (useQuickBayes) {
    bayesQuasiAlgorithm =
        m_model->setupBayesQuasi2Algorithm(program, baseName, background, eMin, eMax, m_view->elasticPeak());
  } else {
    bayesQuasiAlgorithm = m_model->setupBayesQuasiAlgorithm(
        m_view->resNormName(), m_view->fixWidthName(), program, baseName, background, eMin, eMax,
        m_view->sampleBinning(), m_view->resolutionBinning(), m_view->elasticPeak(), m_view->fixWidth(),
        m_view->useResolution(), m_view->sequentialFit());
  }

  m_algorithmRunner->execute(bayesQuasiAlgorithm);
}

void QuasiPresenter::runComplete(IAlgorithm_sptr const &algorithm, bool const error) {
  m_view->setPlotResultEnabled(!error);
  m_view->setSaveResultEnabled(!error);
  if (error) {
    return;
  }

  m_model->setOutputFitGroup(algorithm->getPropertyValue("OutputWorkspaceFit"));
  m_model->setOutputResult(algorithm->getPropertyValue("OutputWorkspaceResult"));
  m_model->setOutputProbability(algorithm->getPropertyValue("OutputWorkspaceProb"));

  updateMiniPlot();
  m_view->watchADS(true);
}

void QuasiPresenter::updateMiniPlot() {
  auto const sampleWorkspace = m_model->sample();
  if (!sampleWorkspace) {
    return;
  }

  m_view->clearPlot();
  auto const previewSpectrum = m_view->previewSpectrum();
  addSpectrum("Sample", sampleWorkspace, previewSpectrum);

  auto const outputWorkspace = m_model->outputFit(previewSpectrum);
  if (!outputWorkspace) {
    return;
  }

  auto const *axis = dynamic_cast<TextAxis *>(outputWorkspace->getAxis(1));
  for (auto index = 0u; index < outputWorkspace->getNumberHistograms(); ++index) {
    auto const label = axis->label(index);
    if (auto const colour = m_model->curveColour(label)) {
      addSpectrum(label, outputWorkspace, index, *colour);
    }
  }
}

void QuasiPresenter::addSpectrum(std::string const &label, Mantid::API::MatrixWorkspace_sptr const &workspace,
                                 std::size_t const spectrumIndex, std::string const &colour) {
  try {
    m_view->addSpectrum(label, workspace, spectrumIndex, colour);
  } catch (std::exception const &ex) {
    g_log.warning(ex.what());
  }
}

void QuasiPresenter::setFileExtensionsByName(bool const filter) { m_view->setFileExtensionsByName(filter); }

void QuasiPresenter::setLoadHistory(bool const loadHistory) { m_view->setLoadHistory(loadHistory); }

void QuasiPresenter::loadSettings(const QSettings &settings) { m_view->loadSettings(settings); }

} // namespace MantidQt::CustomInterfaces
