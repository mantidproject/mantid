// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"

#include <exception>
#include <functional>

namespace MantidQt::MantidWidgets {

PlotFitAnalysisPanePresenter::PlotFitAnalysisPanePresenter(IPlotFitAnalysisPaneView *view,
                                                           IPlotFitAnalysisPaneModel *model)
    : m_fitObserver(nullptr), m_updateEstimateObserver(nullptr), m_view(view), m_model(model), m_currentName("") {

  m_peakCentreObserver = new VoidObserver();
  m_fitObserver = new VoidObserver();
  m_updateEstimateObserver = new VoidObserver();

  m_view->observePeakCentreLineEdit(m_peakCentreObserver);
  m_view->observeFitButton(m_fitObserver);
  m_view->observeUpdateEstimateButton(m_updateEstimateObserver);

  std::function<void()> peakCentreBinder = std::bind(&PlotFitAnalysisPanePresenter::peakCentreEditingFinished, this);
  std::function<void()> fitBinder = std::bind(&PlotFitAnalysisPanePresenter::fitClicked, this);
  std::function<void()> updateEstimateBinder = std::bind(&PlotFitAnalysisPanePresenter::updateEstimateClicked, this);

  m_peakCentreObserver->setSlot(peakCentreBinder);
  m_fitObserver->setSlot(fitBinder);
  m_updateEstimateObserver->setSlot(updateEstimateBinder);
}

void PlotFitAnalysisPanePresenter::peakCentreEditingFinished() {
  m_model->setPeakCentre(m_view->peakCentre());
  m_view->setPeakCentreStatus(m_model->fitStatus());
}

void PlotFitAnalysisPanePresenter::fitClicked() {
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

void PlotFitAnalysisPanePresenter::updateEstimateClicked() {
  const auto validationMessage = validateFitValues();
  if (!validationMessage) {
    m_model->calculateEstimate(m_currentName, m_view->getRange());
    updatePeakCentreInViewFromModel();
  } else {
    m_view->displayWarning(*validationMessage);
  }
}

std::optional<std::string> PlotFitAnalysisPanePresenter::validateFitValues() const {
  if (!checkDataIsExtracted())
    return "Need to have extracted data to do a fit or estimate.";
  if (!checkPeakCentreIsWithinFitRange())
    return "The Peak Centre provided is outside the fit range.";
  return std::nullopt;
}

void PlotFitAnalysisPanePresenter::addSpectrum(const std::string &wsName) {
  m_currentName = wsName;
  m_view->addSpectrum(wsName);
}

bool PlotFitAnalysisPanePresenter::checkDataIsExtracted() const { return !m_currentName.empty(); }

bool PlotFitAnalysisPanePresenter::checkPeakCentreIsWithinFitRange() const {
  const auto peakCentre = m_view->peakCentre();
  const auto range = m_view->getRange();
  return range.first < peakCentre && peakCentre < range.second;
}

void PlotFitAnalysisPanePresenter::updatePeakCentreInViewFromModel() {
  m_view->setPeakCentre(m_model->peakCentre());
  m_view->setPeakCentreStatus(m_model->fitStatus());
}

} // namespace MantidQt::MantidWidgets
