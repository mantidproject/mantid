// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IqtTemplatePresenter.h"
#include "IqtFunctionTemplateView.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"

#include <cmath>

namespace MantidQt::CustomInterfaces::IDA {

using namespace MantidWidgets;

IqtTemplatePresenter::IqtTemplatePresenter(IqtFunctionTemplateView *browser, std::unique_ptr<IqtFunctionModel> model)
    : FunctionTemplatePresenter(browser, std::move(model)) {
  view()->updateState();
}

IqtFunctionTemplateView *IqtTemplatePresenter::view() const { return dynamic_cast<IqtFunctionTemplateView *>(m_view); }

IqtFunctionModel *IqtTemplatePresenter::model() const { return dynamic_cast<IqtFunctionModel *>(m_model.get()); }

void IqtTemplatePresenter::setNumberOfExponentials(int n) {
  if (n < 0) {
    throw std::logic_error("The number of exponents cannot be a negative number.");
  }
  if (n > 2) {
    throw std::logic_error("The number of exponents is limited to 2.");
  }
  auto nCurrent = model()->getNumberOfExponentials();
  if (n == 0) {
    if (nCurrent == 2) {
      view()->removeExponentialTwo();
      --nCurrent;
    }
    if (nCurrent == 1) {
      view()->removeExponentialOne();
      --nCurrent;
    }
  } else if (n == 1) {
    if (nCurrent == 0) {
      view()->addExponentialOne();
      ++nCurrent;
    } else {
      view()->removeExponentialTwo();
      --nCurrent;
    }
  } else /*n == 2*/ {
    if (nCurrent == 0) {
      view()->addExponentialOne();
      ++nCurrent;
    }
    if (nCurrent == 1) {
      view()->addExponentialTwo();
      ++nCurrent;
    }
  }
  assert(nCurrent == n);
  model()->setNumberOfExponentials(n);
  setErrorsEnabled(false);
  updateView();
  m_view->emitFunctionStructureChanged();
}

void IqtTemplatePresenter::setStretchExponential(bool on) {
  if (on == model()->hasStretchExponential())
    return;
  if (on) {
    view()->addStretchExponential();
  } else {
    view()->removeStretchExponential();
  }
  model()->setStretchExponential(on);
  setErrorsEnabled(false);
  updateView();
  m_view->emitFunctionStructureChanged();
}

void IqtTemplatePresenter::setBackground(std::string const &name) {
  if (name == "None") {
    view()->removeBackground();
    model()->removeBackground();
  } else if (name == "FlatBackground") {
    view()->addFlatBackground();
    model()->setBackground(name);
  } else {
    throw std::logic_error("Browser doesn't support background " + name);
  }
  setErrorsEnabled(false);
  updateView();
  m_view->emitFunctionStructureChanged();
}

void IqtTemplatePresenter::setFunction(std::string const &funStr) {
  m_model->setFunctionString(funStr);
  m_view->clear();
  setErrorsEnabled(false);
  if (model()->hasBackground()) {
    view()->addFlatBackground();
  }
  if (model()->hasStretchExponential()) {
    view()->addStretchExponential();
  }
  auto const nExp = model()->getNumberOfExponentials();
  if (nExp > 0) {
    view()->addExponentialOne();
  }
  if (nExp > 1) {
    view()->addExponentialTwo();
  }
  updateView();
  m_view->emitFunctionStructureChanged();
}

void IqtTemplatePresenter::tieIntensities(bool on) {
  if (on && !canTieIntensities())
    return;
  model()->tieIntensities(on);
  m_view->emitFunctionStructureChanged();
}

bool IqtTemplatePresenter::canTieIntensities() const {
  return (model()->hasStretchExponential() || model()->getNumberOfExponentials() > 0) && model()->hasBackground();
}

EstimationDataSelector IqtTemplatePresenter::getEstimationDataSelector() const {
  return model()->getEstimationDataSelector();
}

void IqtTemplatePresenter::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  model()->updateParameterEstimationData(std::move(data));
}

void IqtTemplatePresenter::estimateFunctionParameters() {
  model()->estimateFunctionParameters();
  updateView();
}

void IqtTemplatePresenter::updateViewParameters() {
  static std::map<IqtFunctionModel::ParamID, void (IqtFunctionTemplateView::*)(double, double)> setters{
      {IqtFunctionModel::ParamID::EXP1_HEIGHT, &IqtFunctionTemplateView::setExp1Height},
      {IqtFunctionModel::ParamID::EXP1_LIFETIME, &IqtFunctionTemplateView::setExp1Lifetime},
      {IqtFunctionModel::ParamID::EXP2_HEIGHT, &IqtFunctionTemplateView::setExp2Height},
      {IqtFunctionModel::ParamID::EXP2_LIFETIME, &IqtFunctionTemplateView::setExp2Lifetime},
      {IqtFunctionModel::ParamID::STRETCH_HEIGHT, &IqtFunctionTemplateView::setStretchHeight},
      {IqtFunctionModel::ParamID::STRETCH_LIFETIME, &IqtFunctionTemplateView::setStretchLifetime},
      {IqtFunctionModel::ParamID::STRETCH_STRETCHING, &IqtFunctionTemplateView::setStretchStretching},
      {IqtFunctionModel::ParamID::BG_A0, &IqtFunctionTemplateView::setA0}};
  auto values = model()->getCurrentValues();
  auto errors = model()->getCurrentErrors();
  for (auto const name : values.keys()) {
    (view()->*setters.at(name))(values[name], errors[name]);
  }
}

void IqtTemplatePresenter::updateViewParameterNames() { m_view->updateParameterNames(model()->getParameterNameMap()); }

void IqtTemplatePresenter::updateView() {
  updateViewParameterNames();
  updateViewParameters();
  view()->updateState();
}

} // namespace MantidQt::CustomInterfaces::IDA
