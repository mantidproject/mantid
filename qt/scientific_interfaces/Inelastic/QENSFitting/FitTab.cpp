// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitTab.h"

#include "FitPlotView.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"

#include <QString>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

FitTab::FitTab(QWidget *parent, std::string const &tabName)
    : InelasticTab(parent), m_uiForm(new Ui::FitTab), m_dataPresenter(), m_fittingPresenter(), m_plotPresenter(),
      m_outOptionsPresenter() {
  m_uiForm->setupUi(parent);
  parent->setWindowTitle(QString::fromStdString(tabName));
  m_runPresenter = std::make_unique<RunPresenter>(this, m_uiForm->runWidget);
}

void FitTab::setupOutputOptionsPresenter(bool const editResults) {
  auto model = std::make_unique<FitOutputOptionsModel>();
  auto plotter = std::make_unique<Widgets::MplCpp::ExternalPlotter>();
  m_outOptionsPresenter =
      std::make_unique<FitOutputOptionsPresenter>(m_uiForm->ovOutputOptionsView, std::move(model), std::move(plotter));
  m_outOptionsPresenter->setEditResultVisible(editResults);
}

void FitTab::setupPlotView(std::optional<std::pair<double, double>> const &xPlotBounds) {
  m_plotPresenter = std::make_unique<FitPlotPresenter>(this, m_uiForm->dockArea->m_fitPlotView,
                                                       m_fittingPresenter->getFitPlotModel());
  if (xPlotBounds) {
    m_plotPresenter->setXBounds(*xPlotBounds);
  }
  m_plotPresenter->updatePlots();
}

std::string FitTab::tabName() const { return m_parentWidget->windowTitle().toStdString(); }

void FitTab::handleTableStartXChanged(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_plotPresenter->isCurrentlySelected(workspaceID, spectrum)) {
    m_plotPresenter->setStartX(startX);
    m_plotPresenter->updateGuess();
  }
}

void FitTab::handleTableEndXChanged(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (m_plotPresenter->isCurrentlySelected(workspaceID, spectrum)) {
    m_plotPresenter->setEndX(endX);
    m_plotPresenter->updateGuess();
  }
}

void FitTab::handleFunctionListChanged(const std::map<std::string, std::string> &functionStrings) {
  m_fittingPresenter->updateFunctionListInBrowser(functionStrings);
}

void FitTab::handleStartXChanged(double startX) {
  m_plotPresenter->setStartX(startX);
  m_dataPresenter->setStartX(startX, m_plotPresenter->getActiveWorkspaceID());
  updateParameterEstimationData();
  m_plotPresenter->updateGuess();
  m_dataPresenter->updateTableFromModel();
}

void FitTab::handleEndXChanged(double endX) {
  m_plotPresenter->setEndX(endX);
  m_dataPresenter->setEndX(endX, m_plotPresenter->getActiveWorkspaceID());
  updateParameterEstimationData();
  m_plotPresenter->updateGuess();
  m_dataPresenter->updateTableFromModel();
}

void FitTab::handleSingleFitClicked() {
  if (m_runPresenter->validate()) {
    m_plotPresenter->setFitSingleSpectrumIsFitting(true);
    updateFitButtons(false);
    updateOutputOptions(false);
    m_fittingPresenter->runSingleFit();
  }
}

void FitTab::handleDataChanged() {
  updateDataReferences();
  m_fittingPresenter->removeFittingData();
  m_plotPresenter->updateAvailableSpectra();
  m_plotPresenter->updatePlots();
  m_plotPresenter->updateGuessAvailability();
  updateParameterEstimationData();
  updateOutputOptions(true);
}

void FitTab::handleDataAdded(IAddWorkspaceDialog const *dialog) {
  if (m_dataPresenter->addWorkspaceFromDialog(dialog)) {
    m_fittingPresenter->addDefaultParameters();
  }
  updateDataReferences();
  m_plotPresenter->appendLastDataToSelection(m_dataPresenter->createDisplayNames());
  updateParameterEstimationData();
}

void FitTab::handleDataRemoved() {
  m_fittingPresenter->removeDefaultParameters();
  updateDataReferences();
  m_plotPresenter->updateDataSelection(m_dataPresenter->createDisplayNames());
  updateParameterEstimationData();
  m_dataPresenter->updateFitFunctionList();
}

void FitTab::handlePlotSpectrumChanged() {
  auto const index = m_plotPresenter->getSelectedDomainIndex();
  m_fittingPresenter->setCurrentDataset(index);
}

void FitTab::handleFwhmChanged(double fwhm) {
  m_fittingPresenter->setFWHM(m_plotPresenter->getActiveWorkspaceID(), fwhm);
  m_fittingPresenter->updateFitBrowserParameterValues();
  m_plotPresenter->updateGuess();
}

void FitTab::handleBackgroundChanged(double value) {
  m_fittingPresenter->setBackground(m_plotPresenter->getActiveWorkspaceID(), value);
  updateFitFunction();
  m_plotPresenter->updateGuess();
}

void FitTab::handleFunctionChanged() {
  updateFitFunction();
  m_fittingPresenter->removeFittingData();
  m_plotPresenter->updatePlots();
  m_plotPresenter->updateFit();
  m_fittingPresenter->updateFitTypeString();
}

void FitTab::handleValidation(IUserInputValidator *validator) const {
  m_dataPresenter->validate(validator);
  m_fittingPresenter->validate(validator);
}

void FitTab::handleRun() {
  updateFitButtons(false);
  updateOutputOptions(false);
  m_fittingPresenter->runFit();
}

void FitTab::handleFitComplete(bool const error) {
  m_plotPresenter->setFitSingleSpectrumIsFitting(false);
  updateFitButtons(true);
  updateOutputOptions(!error);
  if (!error) {
    m_plotPresenter->setFitFunction(m_fittingPresenter->fitFunction());
  }
  m_plotPresenter->updatePlots();
}

void FitTab::updateParameterEstimationData() {
  m_fittingPresenter->updateParameterEstimationData(
      m_dataPresenter->getDataForParameterEstimation(m_fittingPresenter->getEstimationDataSelector()));
  m_fittingPresenter->estimateFunctionParameters(m_plotPresenter->getActiveWorkspaceID(),
                                                 m_plotPresenter->getActiveWorkspaceIndex());
}

void FitTab::updateDataReferences() {
  m_fittingPresenter->updateFunctionBrowserData(static_cast<int>(m_dataPresenter->getNumberOfDomains()),
                                                m_dataPresenter->getDatasets(), m_dataPresenter->getQValuesForData(),
                                                m_dataPresenter->getResolutionsForFit());
  updateFitFunction();
}

void FitTab::updateFitFunction() {
  auto const func = m_fittingPresenter->fitFunction();
  m_plotPresenter->setFitFunction(func);
  m_fittingPresenter->setFitFunction(func);
}

void FitTab::updateFitButtons(bool const enable) {
  m_runPresenter->setRunEnabled(enable);
  m_plotPresenter->setFitSingleSpectrumEnabled(enable);
  m_fittingPresenter->setFitEnabled(enable);
}

void FitTab::updateOutputOptions(bool const enable) {
  auto const enableOptions = enable && m_fittingPresenter->isPreviouslyFit(m_plotPresenter->getActiveWorkspaceID(),
                                                                           m_plotPresenter->getActiveWorkspaceIndex());
  m_outOptionsPresenter->enableOutputOptions(enableOptions, m_fittingPresenter->getResultWorkspace(),
                                             m_fittingPresenter->getOutputBasename(), m_fittingPresenter->minimizer());
}

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
