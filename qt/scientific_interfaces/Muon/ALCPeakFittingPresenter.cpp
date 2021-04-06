// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCPeakFittingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

ALCPeakFittingPresenter::ALCPeakFittingPresenter(IALCPeakFittingView *view, IALCPeakFittingModel *model)
    : m_view(view), m_model(model), m_guessPlotted(false) {}

void ALCPeakFittingPresenter::initialize() {
  m_view->initialize();

  connect(m_view, SIGNAL(fitRequested()), SLOT(fit()));
  connect(m_view, SIGNAL(currentFunctionChanged()), SLOT(onCurrentFunctionChanged()));
  connect(m_view, SIGNAL(peakPickerChanged()), SLOT(onPeakPickerChanged()));

  // We are updating the whole function anyway, so paramName if left out
  connect(m_view, SIGNAL(parameterChanged(QString, QString)), SLOT(onParameterChanged(QString)));

  connect(m_model, SIGNAL(fittedPeaksChanged()), SLOT(onFittedPeaksChanged()));
  connect(m_model, SIGNAL(dataChanged()), SLOT(onDataChanged()));
  connect(m_view, SIGNAL(plotGuessClicked()), SLOT(onPlotGuessClicked()));
  connect(m_model, SIGNAL(errorInModel(const QString &)), m_view, SLOT(displayError(const QString &)));
}

void ALCPeakFittingPresenter::fit() {
  IFunction_const_sptr func = m_view->function("");
  auto dataWS = m_model->data();
  if (func && dataWS) {
    removePlot("Fit");
    m_model->fitPeaks(func);
  } else {
    m_view->displayError("Couldn't fit with empty function/data");
  }
}

void ALCPeakFittingPresenter::onCurrentFunctionChanged() {
  if (auto index = m_view->currentFunctionIndex()) // If any function selected
  {
    IFunction_const_sptr currentFunc = m_view->function(*index);

    if (auto peakFunc = std::dynamic_pointer_cast<const IPeakFunction>(currentFunc)) {
      // If peak function selected - update and enable
      m_view->setPeakPicker(peakFunc);
      m_view->setPeakPickerEnabled(true);
      return;
    }
  }

  // Nothing or a non-peak function selected - disable Peak Picker
  m_view->setPeakPickerEnabled(false);
}

void ALCPeakFittingPresenter::onPeakPickerChanged() {
  auto index = m_view->currentFunctionIndex();

  // If PeakPicker is changed, it should be enabled, which means a peak function
  // should be selected
  // (See onCurrentFunctionChanged)
  if (!index)
    return;

  auto peakFunc = m_view->peakPicker();

  // Update all the defined parameters of the peak function
  for (size_t i = 0; i < peakFunc->nParams(); ++i) {
    QString paramName = QString::fromStdString(peakFunc->parameterName(i));
    m_view->setParameter(*index, paramName, peakFunc->getParameter(paramName.toStdString()));
  }
}

void ALCPeakFittingPresenter::onParameterChanged(const QString &funcIndex) {
  auto currentIndex = m_view->currentFunctionIndex();

  // We are interested in parameter changed of the currently selected function
  // only - that's what
  // PeakPicker is showing
  if (currentIndex && *currentIndex == funcIndex) {
    if (auto peak = std::dynamic_pointer_cast<const IPeakFunction>(m_view->function(funcIndex))) {
      m_view->setPeakPicker(peak);
    }
  }
}

void ALCPeakFittingPresenter::onFittedPeaksChanged() {
  IFunction_const_sptr fitted = m_model->fittedPeaks();
  auto dataWS = m_model->data();
  if (fitted && dataWS) {
    m_view->setFittedCurve(dataWS, 1);
    m_view->setFunction(fitted);
  } else {
    m_view->removePlot("Fit");
    m_view->setFunction(IFunction_const_sptr());
  }
}

void ALCPeakFittingPresenter::onDataChanged() {
  auto dataWS = m_model->data();
  if (dataWS)
    m_view->setDataCurve(dataWS);
}

/**
 * Called when user clicks "Plot/Remove guess" on the view.
 * Plots the current guess fit on the graph, or removes it.
 */
void ALCPeakFittingPresenter::onPlotGuessClicked() {
  if (m_guessPlotted) {
    removePlot("Guess");
  } else {
    if (plotGuessOnGraph()) {
      m_view->changePlotGuessState(true);
      m_guessPlotted = true;
    } else {
      m_view->displayError("Couldn't plot with empty function/data");
      removePlot("Guess");
    }
  }
}

/**
 * Plots current guess on the graph, if possible
 * Not possible if function or data are null
 * @returns :: success or failure
 */
bool ALCPeakFittingPresenter::plotGuessOnGraph() {
  if (const auto fitFunction = m_view->function("")) {
    if (const auto dataWorkspace = m_model->data()) {
      const auto &xValues = dataWorkspace->x(0);
      m_view->setGuessCurve(m_model->guessData(fitFunction, xValues.rawData()));
      return true;
    }
  }
  return false;
}

/**
 * Removes any fit function from the graph.
 */
void ALCPeakFittingPresenter::removePlot(std::string const &plotName) {
  m_view->removePlot(QString::fromStdString(plotName));
  m_view->changePlotGuessState(false);
  m_guessPlotted = false;
}

} // namespace CustomInterfaces
} // namespace MantidQt
