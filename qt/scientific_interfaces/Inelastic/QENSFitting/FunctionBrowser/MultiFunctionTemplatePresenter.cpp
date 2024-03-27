// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MultiFunctionTemplatePresenter.h"
#include "ConvFunctionTemplateModel.h"
#include "MultiFunctionTemplateView.h"

namespace MantidQt::CustomInterfaces::Inelastic {

MultiFunctionTemplatePresenter::MultiFunctionTemplatePresenter(MultiFunctionTemplateView *view,
                                                               std::unique_ptr<MultiFunctionTemplateModel> model)
    : FunctionTemplatePresenter(view, std::move(model)) {}

MultiFunctionTemplateView *MultiFunctionTemplatePresenter::view() const {
  return dynamic_cast<MultiFunctionTemplateView *>(m_view);
}

MultiFunctionTemplateModel *MultiFunctionTemplatePresenter::model() const {
  return dynamic_cast<MultiFunctionTemplateModel *>(m_model.get());
}

void MultiFunctionTemplatePresenter::setSubType(size_t subTypeIndex, int typeIndex) {
  model()->setSubType(subTypeIndex, typeIndex);
  view()->setSubType(subTypeIndex, typeIndex);

  setErrorsEnabled(false);
  updateView();
  m_view->emitFunctionStructureChanged();
}

void MultiFunctionTemplatePresenter::setFunction(std::string const &funStr) {
  m_model->setFunctionString(funStr);

  view()->setSubTypes(model()->getSubTypes());

  setErrorsEnabled(false);
  updateView();
  m_view->emitFunctionStructureChanged();
}

void MultiFunctionTemplatePresenter::updateView() {
  updateViewParameterNames();
  updateViewParameters();
}

void MultiFunctionTemplatePresenter::updateViewParameters() {
  auto templateView = view();
  MultiFunctionTemplateModel const *templateModel = model();

  auto values = templateModel->getCurrentValues();
  auto errors = templateModel->getCurrentErrors();
  for (auto const &value : values) {
    templateView->setParameterValueQuiet(value.first, value.second, errors[value.first]);
  }
  m_view->setGlobalParametersQuiet(m_model->getGlobalParameters());
}

void MultiFunctionTemplatePresenter::updateViewParameterNames() {
  m_view->updateParameterNames(model()->getParameterNameMap());
}

EstimationDataSelector MultiFunctionTemplatePresenter::getEstimationDataSelector() const {
  return model()->getEstimationDataSelector();
}

void MultiFunctionTemplatePresenter::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  model()->updateParameterEstimationData(std::move(data));
}

void MultiFunctionTemplatePresenter::estimateFunctionParameters() {
  model()->estimateFunctionParameters();
  updateViewParameters();
}

} // namespace MantidQt::CustomInterfaces::Inelastic
