// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "StretchPresenter.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"
#include <MantidQtWidgets/Common/ConfiguredAlgorithm.h>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::WorkspaceUtils;

namespace {
Mantid::Kernel::Logger g_log("Stretch");
struct PlotType {
  inline static const std::string ALL = "All";
  inline static const std::string SIGMA = "Sigma";
  inline static const std::string BETA = "Beta";
  inline static const std::string FWHM = "FWHM";
};
} // namespace

namespace MantidQt::CustomInterfaces {
StretchPresenter::StretchPresenter(QWidget *parent, StretchView *view, std::unique_ptr<StretchModel> model,
                                   std::unique_ptr<API::IAlgorithmRunner> algorithmRunner)
    : BayesFittingTab(parent, std::move(algorithmRunner)), m_previewSpec(0), m_save(false), m_view(view),
      m_model(std::move(model)) {
  m_view->subscribePresenter(this);

  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_view->getRunWidget()));
}

void StretchPresenter::handleValidation(IUserInputValidator *validator) const { m_view->validateUserInput(validator); }

void StretchPresenter::handleRun() {
  auto const saveDirectory = Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory");
  if (saveDirectory.empty()) {
    auto const result = m_view->displaySaveDirectoryMessage();
    if (result) {
      m_runPresenter->setRunEnabled(true);
      return;
    }
  }

  m_view->setPlotADSEnabled(false);

  StretchRunData algParams = m_view->getRunData();

  auto const cutIndex = algParams.sampleName.find_last_of("_");
  auto const baseName = algParams.sampleName.substr(0, cutIndex);
  m_fitWorkspaceName = baseName + "_Stretch_Fit";
  m_contourWorkspaceName = baseName + "_Stretch_Contour";

  auto stretch = m_model->stretchAlgorithm(algParams, m_fitWorkspaceName, m_contourWorkspaceName);
  m_algorithmRunner->execute(stretch);
}

void StretchPresenter::runComplete(IAlgorithm_sptr const &algorithm, bool const error) {
  (void)algorithm;

  m_view->setPlotResultEnabled(!error);
  m_view->setPlotContourEnabled(!error);
  m_view->setSaveResultEnabled(!error);
  if (!error) {
    if (doesExistInADS(m_contourWorkspaceName))
      resetPlotContourOptions();
    else
      m_view->setPlotContourEnabled(false);

    m_view->setPlotADSEnabled(false);
  }
}

void StretchPresenter::setButtonsEnabled(bool enabled) {
  m_runPresenter->setRunEnabled(enabled);
  m_view->setButtonsEnabled(enabled);
}

void StretchPresenter::setPlotResultIsPlotting(bool plotting) {
  m_view->setPlotResultIsPlotting(plotting);
  setButtonsEnabled(!plotting);
}

void StretchPresenter::setPlotContourIsPlotting(bool plotting) {
  m_view->setPlotContourIsPlotting(plotting);
  setButtonsEnabled(!plotting);
}

void StretchPresenter::resetPlotContourOptions() {
  auto const contourGroup = getADSWorkspace<WorkspaceGroup>(m_contourWorkspaceName);
  auto const contourNames = contourGroup->getNames();
  m_view->resetPlotContourOptions(contourNames);
}

void StretchPresenter::setFileExtensionsByName(bool filter) { m_view->setFileExtensionsByName(filter); }

void StretchPresenter::setLoadHistory(bool doLoadHistory) { m_view->setLoadHistory(doLoadHistory); }

void StretchPresenter::notifySaveClicked() {
  auto fitWorkspace = QString::fromStdString(m_fitWorkspaceName);
  auto contourWorkspace = QString::fromStdString(m_contourWorkspaceName);

  InterfaceUtils::checkADSForPlotSaveWorkspace(m_fitWorkspaceName, false);
  InterfaceUtils::checkADSForPlotSaveWorkspace(m_contourWorkspaceName, false);

  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algorithmQueue;

  algorithmQueue.push_back(m_model->setupSaveAlgorithm(fitWorkspace.toStdString()));
  algorithmQueue.push_back(m_model->setupSaveAlgorithm(contourWorkspace.toStdString()));

  m_algorithmRunner->execute(algorithmQueue);
}

void StretchPresenter::notifyPlotClicked() {
  setPlotResultIsPlotting(true);

  std::string const plotType = m_view->getPlotType();
  auto const plotErrors = SettingsHelper::externalPlotErrorBars();
  auto const plotSigma = (plotType == "All" || plotType == "Sigma");
  auto const plotBeta = (plotType == "All" || plotType == "Beta");

  auto const fitWorkspace = getADSWorkspace<WorkspaceGroup>(m_fitWorkspaceName);
  for (auto it = fitWorkspace->begin(); it < fitWorkspace->end(); ++it) {
    auto const name = (*it)->getName();
    if (plotSigma && name.substr(name.length() - 5) == "Sigma") {
      m_plotter->plotSpectra(name, "0", plotErrors);
    } else if (plotBeta && name.substr(name.length() - 4) == "Beta") {
      m_plotter->plotSpectra(name, "0", plotErrors);
    }
  }

  setPlotResultIsPlotting(false);
}

void StretchPresenter::notifyPlotContourClicked() {
  setPlotContourIsPlotting(true);

  auto const workspaceName = m_view->getPlotContour();
  if (checkADSForPlotSaveWorkspace(workspaceName, true))
    m_plotter->plotContour(workspaceName);

  setPlotContourIsPlotting(false);
}

void StretchPresenter::notifyPreviewSpecChanged(int specNum) { m_previewSpec = specNum; }

void StretchPresenter::notifyPlotCurrentPreviewClicked() {
  auto previewData = m_view->getCurrentPreviewData();
  if (previewData.hasSample) {
    m_plotter->plotSpectra(previewData.sampleName, std::to_string(m_previewSpec),
                           SettingsHelper::externalPlotErrorBars());
  }
}

void StretchPresenter::loadSettings(const QSettings &settings) { m_view->loadSettings(settings); }

void StretchPresenter::applySettings(std::map<std::string, QVariant> const &settings) {
  m_view->applySettings(settings);
}

} // namespace MantidQt::CustomInterfaces
