// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFAnalysisPresenter.h"
#include "ALFAnalysisView.h"

#include <exception>

namespace MantidQt::CustomInterfaces {

ALFAnalysisPresenter::ALFAnalysisPresenter(IALFAnalysisView *view, std::unique_ptr<IALFAnalysisModel> model)
    : m_currentName(""), m_view(view), m_model(std::move(model)) {
  m_view->subscribePresenter(this);
}

QWidget *ALFAnalysisPresenter::getView() { return m_view->getView(); };

void ALFAnalysisPresenter::notifyPeakCentreEditingFinished() {
  m_model->setPeakCentre(m_view->peakCentre());
  m_view->setPeakCentreStatus(m_model->fitStatus());
}

void ALFAnalysisPresenter::notifyFitClicked() {
  if (const auto validationMessage = validateFitValues()) {
    m_view->displayWarning(*validationMessage);
    return;
  }

  try {
    m_model->doFit(m_currentName, m_view->getRange());
  } catch (...) {
    m_view->displayWarning("Fit failed");
  }
  updatePeakCentreInViewFromModel();
  m_view->addFitSpectrum(m_currentName + "_fits_Workspace");
}

void ALFAnalysisPresenter::notifyUpdateEstimateClicked() {
  const auto validationMessage = validateFitValues();
  if (!validationMessage) {
    m_model->calculateEstimate(m_currentName, m_view->getRange());
    updatePeakCentreInViewFromModel();
  } else {
    m_view->displayWarning(*validationMessage);
  }
}

std::optional<std::string> ALFAnalysisPresenter::validateFitValues() const {
  if (!checkDataIsExtracted())
    return "Need to have extracted data to do a fit or estimate.";
  if (!checkPeakCentreIsWithinFitRange())
    return "The Peak Centre provided is outside the fit range.";
  return std::nullopt;
}

void ALFAnalysisPresenter::addSpectrum(const std::string &wsName) {
  m_currentName = wsName;
  m_view->addSpectrum(wsName);
}

bool ALFAnalysisPresenter::checkDataIsExtracted() const { return !m_currentName.empty(); }

bool ALFAnalysisPresenter::checkPeakCentreIsWithinFitRange() const {
  const auto peakCentre = m_view->peakCentre();
  const auto range = m_view->getRange();
  return range.first < peakCentre && peakCentre < range.second;
}

void ALFAnalysisPresenter::updatePeakCentreInViewFromModel() {
  m_view->setPeakCentre(m_model->peakCentre());
  m_view->setPeakCentreStatus(m_model->fitStatus());
}

} // namespace MantidQt::CustomInterfaces
