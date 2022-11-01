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
  if (m_currentName != "") {
    try {
      m_model->doFit(m_currentName, m_view->getRange());
      updatePeakCentreInViewFromModel();
    } catch (...) {
      m_view->displayWarning("Fit failed");
    }
    m_view->addFitSpectrum(m_currentName + "_fits_Workspace");
  } else {
    m_view->displayWarning("Need to have extracted data to do a fit");
  }
}

void PlotFitAnalysisPanePresenter::updateEstimateClicked() {
  if (!m_currentName.empty()) {
    m_model->calculateEstimate(m_currentName, m_view->getRange());
    updatePeakCentreInViewFromModel();
  } else {
    m_view->displayWarning("Could not update estimate: data has not been extracted.");
  }
}

void PlotFitAnalysisPanePresenter::addSpectrum(const std::string &wsName) {
  m_currentName = wsName;
  m_view->addSpectrum(wsName);
}

void PlotFitAnalysisPanePresenter::updatePeakCentreInViewFromModel() {
  m_view->setPeakCentre(m_model->peakCentre());
  m_view->setPeakCentreStatus(m_model->fitStatus());
}

} // namespace MantidQt::MantidWidgets
