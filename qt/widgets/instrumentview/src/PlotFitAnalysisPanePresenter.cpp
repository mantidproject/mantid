// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"

#include <exception>
#include <functional>
#include <tuple>
#include <utility>

namespace MantidQt {
namespace MantidWidgets {

PlotFitAnalysisPanePresenter::PlotFitAnalysisPanePresenter(
    IPlotFitAnalysisPaneView *view, PlotFitAnalysisPaneModel *model)
    : m_fitObserver(nullptr), m_updateEstimateObserver(nullptr), m_view(view),
      m_model(model), m_currentName("") {

  m_fitObserver = new VoidObserver();
  m_updateEstimateObserver = new VoidObserver();

  m_view->observeFitButton(m_fitObserver);
  m_view->observeUpdateEstimateButton(m_updateEstimateObserver);

  std::function<void()> fitBinder =
      std::bind(&PlotFitAnalysisPanePresenter::doFit, this);
  std::function<void()> updateEstimateBinder =
      std::bind(&PlotFitAnalysisPanePresenter::updateEstimate, this);

  m_fitObserver->setSlot(fitBinder);
  m_updateEstimateObserver->setSlot(updateEstimateBinder);
}

void PlotFitAnalysisPanePresenter::doFit() {
  auto func = m_view->getFunction();
  if (m_currentName != "" && func->nParams() != 0) {
    try {
      func = m_model->doFit(m_currentName, m_view->getRange(), func);
      m_view->updateFunction(func);
    } catch (...) {
      m_view->displayWarning("Fit failed");
    }
    m_view->addFitSpectrum(m_currentName + "_fits_Workspace");
  } else {
    m_view->displayWarning(
        "Need to have extracted a data and selected a function to fit");
  }
}

void PlotFitAnalysisPanePresenter::updateEstimateAfterExtraction() {
  if (!m_model->hasEstimate())
    updateEstimate();
}

void PlotFitAnalysisPanePresenter::updateEstimate() {
  if (!m_currentName.empty()) {
    m_view->updateFunction(
        m_model->calculateEstimate(m_currentName, m_view->getRange()));
  } else {
    m_view->displayWarning(
        "Could not update estimate: data has not been extracted.");
  }
}

void PlotFitAnalysisPanePresenter::addFunction(
    Mantid::API::IFunction_sptr func) {
  m_view->addFunction(std::move(func));
}

void PlotFitAnalysisPanePresenter::addSpectrum(const std::string &wsName) {
  m_currentName = wsName;
  m_view->addSpectrum(wsName);
}

} // namespace MantidWidgets
} // namespace MantidQt
