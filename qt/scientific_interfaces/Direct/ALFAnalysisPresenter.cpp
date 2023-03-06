// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFAnalysisPresenter.h"

#include "ALFAnalysisView.h"
#include "ALFInstrumentPresenter.h"

#include <exception>

namespace {

double constexpr EPSILON = std::numeric_limits<double>::epsilon();

bool equalWithinTolerance(double const val1, double const val2, double const tolerance = 0.000001) {
  return std::abs(val1 - val2) <= (tolerance + 2.0 * EPSILON);
}

} // namespace

namespace MantidQt::CustomInterfaces {

ALFAnalysisPresenter::ALFAnalysisPresenter(IALFAnalysisView *view, std::unique_ptr<IALFAnalysisModel> model,
                                           std::unique_ptr<IALFAlgorithmManager> algorithmManager)
    : m_view(view), m_model(std::move(model)), m_algorithmManager(std::move(algorithmManager)) {
  m_view->subscribePresenter(this);
  m_algorithmManager->subscribe(this);
}

QWidget *ALFAnalysisPresenter::getView() { return m_view->getView(); }

void ALFAnalysisPresenter::setExtractedWorkspace(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                                 std::vector<double> const &twoThetas) {
  m_model->setExtractedWorkspace(workspace, twoThetas);
  calculateEstimate();
}

void ALFAnalysisPresenter::notifyPeakPickerChanged() {
  m_model->setPeakParameters(m_view->getPeak());

  auto const fitStatus = m_model->fitStatus();

  m_view->setPeakCentre(m_model->peakCentre());
  m_view->setPeakCentreStatus(fitStatus);
  if (fitStatus.empty()) {
    m_view->removeFitSpectrum();
  }
}

void ALFAnalysisPresenter::notifyPeakCentreEditingFinished() {
  auto const newPeakCentre = m_view->peakCentre();
  if (!equalWithinTolerance(m_model->peakCentre(), newPeakCentre)) {
    m_model->setPeakCentre(newPeakCentre);
    updatePeakCentreInViewFromModel();
    updateRotationAngleInViewFromModel();
  }
}

void ALFAnalysisPresenter::notifyFitClicked() {
  if (auto const validationMessage = validateFitValues()) {
    m_view->displayWarning(*validationMessage);
    return;
  }

  m_algorithmManager->fit(m_model->fitProperties(m_view->getRange()));
}

void ALFAnalysisPresenter::notifyAlgorithmError(std::string const &message) { m_view->displayWarning(message); }

void ALFAnalysisPresenter::notifyCropWorkspaceComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) {
  m_model->calculateEstimate(workspace);
  updateViewFromModel();
}

void ALFAnalysisPresenter::notifyFitComplete(Mantid::API::MatrixWorkspace_sptr workspace,
                                             Mantid::API::IFunction_sptr function, std::string fitStatus) {
  m_model->setFitResult(std::move(workspace), std::move(function), std::move(fitStatus));
  m_view->addFitSpectrum(m_model->fitWorkspace());

  updatePeakCentreInViewFromModel();
  updateRotationAngleInViewFromModel();
}

void ALFAnalysisPresenter::notifyExportWorkspaceToADSClicked() { m_model->exportWorkspaceCopyToADS(); }

void ALFAnalysisPresenter::notifyExternalPlotClicked() {
  if (auto const plotWorkspace = m_model->plottedWorkspace()) {
    m_view->openExternalPlot(plotWorkspace, m_model->plottedWorkspaceIndices());
  }
}

void ALFAnalysisPresenter::notifyResetClicked() { calculateEstimate(); }

std::optional<std::string> ALFAnalysisPresenter::validateFitValues() const {
  if (!m_model->isDataExtracted())
    return "Need to have extracted data to do a fit or estimate.";
  if (!checkPeakCentreIsWithinFitRange())
    return "The Peak Centre provided is outside the fit range.";
  return std::nullopt;
}

std::size_t ALFAnalysisPresenter::numberOfTubes() const { return m_model->numberOfTubes(); }

void ALFAnalysisPresenter::clear() {
  m_model->clear();
  updateViewFromModel();
}

bool ALFAnalysisPresenter::checkPeakCentreIsWithinFitRange() const {
  auto const peakCentre = m_view->peakCentre();
  auto const range = m_view->getRange();
  return range.first < peakCentre && peakCentre < range.second;
}

void ALFAnalysisPresenter::calculateEstimate() {
  if (m_model->isDataExtracted()) {
    m_algorithmManager->cropWorkspace(m_model->cropWorkspaceProperties(m_view->getRange()));
  } else {
    updatePlotInViewFromModel();
  }
}

void ALFAnalysisPresenter::updateViewFromModel() {
  updatePlotInViewFromModel();
  updateTwoThetaInViewFromModel();
  updatePeakCentreInViewFromModel();
  updateRotationAngleInViewFromModel();
}

void ALFAnalysisPresenter::updatePlotInViewFromModel() { m_view->addSpectrum(m_model->extractedWorkspace()); }

void ALFAnalysisPresenter::updateTwoThetaInViewFromModel() {
  m_view->setAverageTwoTheta(m_model->averageTwoTheta(), m_model->allTwoThetas());
}

void ALFAnalysisPresenter::updatePeakCentreInViewFromModel() {
  m_view->setPeak(m_model->getPeakCopy(), m_model->background());

  auto const fitStatus = m_model->fitStatus();
  m_view->setPeakCentreStatus(fitStatus);
  if (fitStatus.empty()) {
    m_view->removeFitSpectrum();
  }
  m_view->replot();
}

void ALFAnalysisPresenter::updateRotationAngleInViewFromModel() { m_view->setRotationAngle(m_model->rotationAngle()); }

} // namespace MantidQt::CustomInterfaces
