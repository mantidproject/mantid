// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlotFitAnalysisPanePresenter.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FunctionFactory.h"

#include <exception>
#include <functional>
#include <tuple>

namespace MantidQt {
namespace CustomInterfaces {

PlotFitAnalysisPanePresenter::PlotFitAnalysisPanePresenter(
    PlotFitAnalysisPaneView *view, PlotFitAnalysisPaneModel *model)
    : m_fitObserver(nullptr), m_view(view), m_model(model), m_currentName("") {

  m_fitObserver = new VoidObserver();
  m_view->observeFitButton(m_fitObserver);
  std::function<void()> fitBinder =
      std::bind(&PlotFitAnalysisPanePresenter::doFit, this);
  m_fitObserver->setSlot(fitBinder);
}

void PlotFitAnalysisPanePresenter::doFit() {
  auto func = m_view->getFunction();
  if (m_currentName != "" && func->nParams() != 0) {
    try {
      func = m_model->doFit(m_currentName, m_view->getRange(), func);
      m_view->updateFunction(func);
    } catch (...) {
      m_view->fitWarning("Fit failed");
    }
    m_view->addFitSpectrum(m_currentName + "_fits_Workspace");
  } else {
    m_view->fitWarning(
        "Need to have extracted a data and selected a function to fit");
  }
}

void PlotFitAnalysisPanePresenter::addFunction(
    Mantid::API::IFunction_sptr func) {
  m_view->addFunction(func);
}

void PlotFitAnalysisPanePresenter::addSpectrum(const std::string &wsName) {
  m_currentName = wsName;
  m_view->addSpectrum(wsName);
}

} // namespace CustomInterfaces
} // namespace MantidQt
