// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SingleFunctionTemplatePresenter.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
#include "SingleFunctionTemplateView.h"
#include <math.h>

namespace MantidQt::CustomInterfaces::Inelastic {

using namespace MantidWidgets;

SingleFunctionTemplatePresenter::SingleFunctionTemplatePresenter(SingleFunctionTemplateView *view,
                                                                 std::unique_ptr<SingleFunctionTemplateModel> model)
    : FunctionTemplatePresenter(view, std::move(model)) {}

SingleFunctionTemplateView *SingleFunctionTemplatePresenter::view() const {
  return dynamic_cast<SingleFunctionTemplateView *>(m_view);
}
SingleFunctionTemplateModel *SingleFunctionTemplatePresenter::model() const {
  return dynamic_cast<SingleFunctionTemplateModel *>(m_model.get());
}

void SingleFunctionTemplatePresenter::init() {
  view()->setDataType(model()->getFunctionList());
  setFitType(model()->getFitType());
}

void SingleFunctionTemplatePresenter::updateAvailableFunctions(
    const std::map<std::string, std::string> &functionInitialisationStrings) {
  model()->updateAvailableFunctions(functionInitialisationStrings);
  view()->setDataType(model()->getFunctionList());
  setFitType(model()->getFitType());
}

void SingleFunctionTemplatePresenter::setFitType(std::string const &name) {
  m_view->clear();
  model()->setFitType(name);
  auto functionParameters = m_model->getParameterNames();
  for (auto const &parameter : functionParameters) {
    view()->addParameter(parameter, m_model->getParameterDescription(parameter));
  }
  setErrorsEnabled(false);
  updateView();
  m_view->emitFunctionStructureChanged();
}

void SingleFunctionTemplatePresenter::setFunction(std::string const &funStr) {
  m_view->clear();
  m_model->setFunctionString(funStr);

  if (model()->getFitType() == "None")
    return;
  auto functionParameters = m_model->getParameterNames();
  for (auto const &parameter : functionParameters) {
    view()->addParameter(parameter, m_model->getParameterDescription(parameter));
  }
  view()->setEnumValue(model()->getEnumIndex());
  setErrorsEnabled(false);
  updateView();
  m_view->emitFunctionStructureChanged();
}

EstimationDataSelector SingleFunctionTemplatePresenter::getEstimationDataSelector() const {
  return model()->getEstimationDataSelector();
}

void SingleFunctionTemplatePresenter::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  model()->updateParameterEstimationData(std::move(data));
  updateView();
}
void SingleFunctionTemplatePresenter::estimateFunctionParameters() {
  model()->estimateFunctionParameters();
  updateView();
}

void SingleFunctionTemplatePresenter::updateView() {
  if (model()->getFitType() == "None")
    return;
  for (auto const &parameterName : m_model->getParameterNames()) {
    view()->setParameterValueQuietly(parameterName, m_model->getParameter(parameterName),
                                     m_model->getParameterError(parameterName));
  }
}

} // namespace MantidQt::CustomInterfaces::Inelastic
