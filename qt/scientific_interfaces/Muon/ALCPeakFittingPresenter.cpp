// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCPeakFittingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidQtWidgets/Plotting/Qwt/QwtHelper.h"

using namespace Mantid::API;

namespace MantidQt {
namespace QwtHelper = API::QwtHelper;
namespace CustomInterfaces {

ALCPeakFittingPresenter::ALCPeakFittingPresenter(IALCPeakFittingView *view,
                                                 IALCPeakFittingModel *model)
    : m_view(view), m_model(model), m_guessPlotted(false) {}

void ALCPeakFittingPresenter::initialize() {
  m_view->initialize();

  connect(m_view, SIGNAL(fitRequested()), SLOT(fit()));
  connect(m_view, SIGNAL(currentFunctionChanged()),
          SLOT(onCurrentFunctionChanged()));
  connect(m_view, SIGNAL(peakPickerChanged()), SLOT(onPeakPickerChanged()));

  // We are updating the whole function anyway, so paramName if left out
  connect(m_view, SIGNAL(parameterChanged(QString, QString)),
          SLOT(onParameterChanged(QString)));

  connect(m_model, SIGNAL(fittedPeaksChanged()), SLOT(onFittedPeaksChanged()));
  connect(m_model, SIGNAL(dataChanged()), SLOT(onDataChanged()));
  connect(m_view, SIGNAL(plotGuessClicked()), SLOT(onPlotGuessClicked()));
  connect(m_model, SIGNAL(errorInModel(const QString &)), m_view,
          SLOT(displayError(const QString &)));
}

void ALCPeakFittingPresenter::fit() {
  IFunction_const_sptr func = m_view->function("");
  auto dataWS = m_model->data();
  if (func && dataWS) {
    removePlots();
    m_model->fitPeaks(func);
  } else {
    m_view->displayError("Couldn't fit with empty function/data");
  }
}

void ALCPeakFittingPresenter::onCurrentFunctionChanged() {
  if (auto index = m_view->currentFunctionIndex()) // If any function selected
  {
    IFunction_const_sptr currentFunc = m_view->function(*index);

    if (auto peakFunc =
            boost::dynamic_pointer_cast<const IPeakFunction>(currentFunc)) {
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
  assert(bool(index));

  auto peakFunc = m_view->peakPicker();

  // Update all the defined parameters of the peak function
  for (size_t i = 0; i < peakFunc->nParams(); ++i) {
    QString paramName = QString::fromStdString(peakFunc->parameterName(i));
    m_view->setParameter(*index, paramName,
                         peakFunc->getParameter(paramName.toStdString()));
  }
}

void ALCPeakFittingPresenter::onParameterChanged(const QString &funcIndex) {
  auto currentIndex = m_view->currentFunctionIndex();

  // We are interested in parameter changed of the currently selected function
  // only - that's what
  // PeakPicker is showing
  if (currentIndex && *currentIndex == funcIndex) {
    if (auto peak = boost::dynamic_pointer_cast<const IPeakFunction>(
            m_view->function(funcIndex))) {
      m_view->setPeakPicker(peak);
    }
  }
}

void ALCPeakFittingPresenter::onFittedPeaksChanged() {
  IFunction_const_sptr fittedPeaks = m_model->fittedPeaks();
  auto dataWS = m_model->data();
  if (fittedPeaks && dataWS) {
    const auto &x = dataWS->x(0);
    m_view->setFittedCurve(
        *(QwtHelper::curveDataFromFunction(fittedPeaks, x.rawData())));
    m_view->setFunction(fittedPeaks);
  } else {
    m_view->setFittedCurve(*(QwtHelper::emptyCurveData()));
    m_view->setFunction(IFunction_const_sptr());
  }
}

void ALCPeakFittingPresenter::onDataChanged() {
  auto dataWS = m_model->data();
  if (dataWS) {
    m_view->setDataCurve(*(QwtHelper::curveDataFromWs(m_model->data(), 0)),
                         QwtHelper::curveErrorsFromWs(m_model->data(), 0));
  } else {
    m_view->setDataCurve(*(QwtHelper::emptyCurveData()), Mantid::MantidVec{});
  }
}

/**
 * Called when user clicks "Plot/Remove guess" on the view.
 * Plots the current guess fit on the graph, or removes it.
 */
void ALCPeakFittingPresenter::onPlotGuessClicked() {
  if (m_guessPlotted) {
    removePlots();
  } else {
    if (plotGuessOnGraph()) {
      m_view->changePlotGuessState(true);
      m_guessPlotted = true;
    } else {
      m_view->displayError("Couldn't plot with empty function/data");
      removePlots();
    }
  }
}

/**
 * Plots current guess on the graph, if possible
 * Not possible if function or data are null
 * @returns :: success or failure
 */
bool ALCPeakFittingPresenter::plotGuessOnGraph() {
  bool plotted = false;
  auto func = m_view->function("");
  auto dataWS = m_model->data();
  if (func && dataWS) {
    const auto &xdata = dataWS->x(0);
    m_view->setFittedCurve(
        *(QwtHelper::curveDataFromFunction(func, xdata.rawData())));
    plotted = true;
  }
  return plotted;
}

/**
 * Removes any fit function from the graph.
 */
void ALCPeakFittingPresenter::removePlots() {
  m_view->setFittedCurve(*(QwtHelper::emptyCurveData()));
  m_view->changePlotGuessState(false);
  m_guessPlotted = false;
}

} // namespace CustomInterfaces
} // namespace MantidQt
