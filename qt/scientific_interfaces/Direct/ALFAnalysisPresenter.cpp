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

namespace MantidQt::CustomInterfaces {

ALFAnalysisPresenter::ALFAnalysisPresenter(IALFAnalysisView *view, std::unique_ptr<IALFAnalysisModel> model)
    : m_view(view), m_model(std::move(model)) {
  m_view->subscribePresenter(this);
}

QWidget *ALFAnalysisPresenter::getView() { return m_view->getView(); }

void ALFAnalysisPresenter::subscribeInstrumentPresenter(IALFInstrumentPresenter *presenter) {
  m_instrumentPresenter = presenter;
}

void ALFAnalysisPresenter::notifyPeakCentreEditingFinished() {
  m_model->setPeakCentre(m_view->peakCentre());
  m_view->setPeakCentreStatus(m_model->fitStatus());
}

void ALFAnalysisPresenter::notifyFitClicked() {
  if (auto const validationMessage = validateFitValues()) {
    m_view->displayWarning(*validationMessage);
    return;
  }

  auto const extractedWsName = m_instrumentPresenter->extractedWsName();
  try {
    m_model->doFit(extractedWsName, m_view->getRange());
  } catch (...) {
    m_view->displayWarning("Fit failed");
  }
  updatePeakCentreInViewFromModel();
  m_view->addFitSpectrum(extractedWsName + "_fits_Workspace");
}

void ALFAnalysisPresenter::notifyUpdateEstimateClicked() {
  auto const validationMessage = validateFitValues();
  if (!validationMessage) {
    m_model->calculateEstimate(m_instrumentPresenter->extractedWsName(), m_view->getRange());
    updatePeakCentreInViewFromModel();
  } else {
    m_view->displayWarning(*validationMessage);
  }
}

std::optional<std::string> ALFAnalysisPresenter::validateFitValues() const {
  if (!m_instrumentPresenter->checkDataIsExtracted())
    return "Need to have extracted data to do a fit or estimate.";
  if (!checkPeakCentreIsWithinFitRange())
    return "The Peak Centre provided is outside the fit range.";
  return std::nullopt;
}

void ALFAnalysisPresenter::notifyTubeExtracted(double const twoTheta) {
  m_model->clearTwoThetas();
  m_model->addTwoTheta(twoTheta);
  m_view->addSpectrum(m_instrumentPresenter->extractedWsName());
  m_view->setAverageTwoTheta(m_model->averageTwoTheta(), m_model->allTwoThetas());
}

void ALFAnalysisPresenter::notifyTubeAveraged(double const twoTheta) {
  m_model->addTwoTheta(twoTheta);
  m_view->addSpectrum(m_instrumentPresenter->extractedWsName());
  m_view->setAverageTwoTheta(m_model->averageTwoTheta(), m_model->allTwoThetas());
}

std::size_t ALFAnalysisPresenter::numberOfTubes() const { return m_model->numberOfTubes(); }

void ALFAnalysisPresenter::clearTwoThetas() {
  m_model->clearTwoThetas();
  m_view->setAverageTwoTheta(m_model->averageTwoTheta(), m_model->allTwoThetas());
}

bool ALFAnalysisPresenter::checkPeakCentreIsWithinFitRange() const {
  auto const peakCentre = m_view->peakCentre();
  auto const range = m_view->getRange();
  return range.first < peakCentre && peakCentre < range.second;
}

void ALFAnalysisPresenter::updatePeakCentreInViewFromModel() {
  m_view->setPeakCentre(m_model->peakCentre());
  m_view->setPeakCentreStatus(m_model->fitStatus());
}

} // namespace MantidQt::CustomInterfaces
