// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IqtTemplatePresenter.h"
#include "IqtTemplateBrowser.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"

#include <cmath>

namespace MantidQt::CustomInterfaces::IDA {

using namespace MantidWidgets;

IqtTemplatePresenter::IqtTemplatePresenter(IqtTemplateBrowser *browser, std::unique_ptr<IqtFunctionModel> model)
    : FunctionTemplatePresenter(browser, std::move(model)) {
  view()->updateState();
}

IqtTemplateBrowser *IqtTemplatePresenter::view() const { return dynamic_cast<IqtTemplateBrowser *>(m_view); }

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

void IqtTemplatePresenter::setGlobalParameters(std::vector<std::string> const &globals) {
  m_model->setGlobalParameters(globals);
  view()->setGlobalParametersQuiet(globals);
}

void IqtTemplatePresenter::setGlobal(std::string const &parameterName, bool on) {
  model()->setGlobal(parameterName, on);
  view()->setGlobalParametersQuiet(m_model->getGlobalParameters());
}

void IqtTemplatePresenter::updateMultiDatasetParameters(const IFunction &fun) {
  m_model->updateMultiDatasetParameters(fun);
  updateViewParameters();
}

void IqtTemplatePresenter::updateMultiDatasetParameters(const ITableWorkspace &table) {
  model()->updateMultiDatasetParameters(table);
  updateViewParameters();
}

void IqtTemplatePresenter::updateParameters(const IFunction &fun) {
  m_model->updateParameters(fun);
  updateViewParameters();
}

void IqtTemplatePresenter::setCurrentDataset(int i) {
  m_model->setCurrentDomainIndex(i);
  updateViewParameters();
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

void IqtTemplatePresenter::setBackgroundA0(double value) {
  m_model->setBackgroundA0(value);
  view()->setA0(value, 0.0);
}

void IqtTemplatePresenter::updateViewParameters() {
  static std::map<IqtFunctionModel::ParamID, void (IqtTemplateBrowser::*)(double, double)> setters{
      {IqtFunctionModel::ParamID::EXP1_HEIGHT, &IqtTemplateBrowser::setExp1Height},
      {IqtFunctionModel::ParamID::EXP1_LIFETIME, &IqtTemplateBrowser::setExp1Lifetime},
      {IqtFunctionModel::ParamID::EXP2_HEIGHT, &IqtTemplateBrowser::setExp2Height},
      {IqtFunctionModel::ParamID::EXP2_LIFETIME, &IqtTemplateBrowser::setExp2Lifetime},
      {IqtFunctionModel::ParamID::STRETCH_HEIGHT, &IqtTemplateBrowser::setStretchHeight},
      {IqtFunctionModel::ParamID::STRETCH_LIFETIME, &IqtTemplateBrowser::setStretchLifetime},
      {IqtFunctionModel::ParamID::STRETCH_STRETCHING, &IqtTemplateBrowser::setStretchStretching},
      {IqtFunctionModel::ParamID::BG_A0, &IqtTemplateBrowser::setA0}};
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

void IqtTemplatePresenter::handleEditLocalParameterFinished(std::string const &parameterName,
                                                            QList<double> const &values, QList<bool> const &fixes,
                                                            QStringList const &ties, QStringList const &constraints) {
  (void)constraints;
  assert(values.size() == getNumberOfDatasets());
  for (int i = 0; i < values.size(); ++i) {
    setLocalParameterValue(parameterName, i, values[i]);
    if (!ties[i].isEmpty()) {
      setLocalParameterTie(parameterName, i, ties[i].toStdString());
    } else if (fixes[i]) {
      setLocalParameterFixed(parameterName, i, fixes[i]);
    } else {
      setLocalParameterTie(parameterName, i, "");
    }
  }
  updateViewParameters();
}

} // namespace MantidQt::CustomInterfaces::IDA
