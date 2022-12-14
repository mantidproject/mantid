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

ALFAnalysisPresenter::ALFAnalysisPresenter(IALFAnalysisView *view, std::unique_ptr<IALFAnalysisModel> model)
    : m_view(view), m_model(std::move(model)) {
  m_view->subscribePresenter(this);
}

QWidget *ALFAnalysisPresenter::getView() { return m_view->getView(); }

void ALFAnalysisPresenter::setExtractedWorkspace(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                                 std::vector<double> const &twoThetas) {
  m_model->setExtractedWorkspace(workspace, twoThetas);
  calculateEstimate();
  updateViewFromModel();
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

  try {
    auto const fitWorkspace = m_model->doFit(m_view->getRange());
    m_view->addFitSpectrum(fitWorkspace);
  } catch (std::exception const &ex) {
    m_view->displayWarning(ex.what());
  }
  updatePeakCentreInViewFromModel();
  updateRotationAngleInViewFromModel();
}

void ALFAnalysisPresenter::notifyResetClicked() {
  calculateEstimate();
  updatePeakCentreInViewFromModel();
  updateRotationAngleInViewFromModel();
}

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
    m_model->calculateEstimate(m_view->getRange());
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
  m_view->setPeak(m_model->getPeakCopy());

  auto const fitStatus = m_model->fitStatus();
  m_view->setPeakCentreStatus(fitStatus);
  if (fitStatus.empty()) {
    m_view->removeFitSpectrum();
  }
  m_view->replot();
}

void ALFAnalysisPresenter::updateRotationAngleInViewFromModel() { m_view->setRotationAngle(m_model->rotationAngle()); }

} // namespace MantidQt::CustomInterfaces
