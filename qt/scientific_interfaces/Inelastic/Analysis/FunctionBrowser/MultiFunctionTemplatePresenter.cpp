// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MultiFunctionTemplatePresenter.h"
#include "FitTypes.h"
#include "MultiFunctionTemplateView.h"

namespace MantidQt::CustomInterfaces::IDA {

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

  MultiFunctionTemplateView *convView = view();
  MultiFunctionTemplateModel const *convModel = model();

  convView->setSubType(ConvTypes::SubTypeIndex::Lorentzian, static_cast<int>(convModel->getLorentzianType()));
  convView->setSubType(ConvTypes::SubTypeIndex::Fit, static_cast<int>(convModel->getFitType()));
  convView->setSubType(ConvTypes::SubTypeIndex::Delta, static_cast<int>(convModel->getDeltaType()));
  convView->setSubType(ConvTypes::SubTypeIndex::TempCorrection, static_cast<int>(convModel->getTempCorrectionType()));
  convView->setSubType(ConvTypes::SubTypeIndex::Background, static_cast<int>(convModel->getBackgroundType()));

  convView->setProperty(ConvTypes::SubTypeIndex::Lorentzian, static_cast<int>(convModel->getLorentzianType()));
  convView->setProperty(ConvTypes::SubTypeIndex::Fit, static_cast<int>(convModel->getFitType()));
  convView->setProperty(ConvTypes::SubTypeIndex::Delta, static_cast<bool>(convModel->getDeltaType()));
  convView->setProperty(ConvTypes::SubTypeIndex::TempCorrection, static_cast<bool>(convModel->getTempCorrectionType()));
  convView->setProperty(ConvTypes::SubTypeIndex::Background, static_cast<int>(convModel->getBackgroundType()));

  setErrorsEnabled(false);
  updateView();
  m_view->emitFunctionStructureChanged();
}

void MultiFunctionTemplatePresenter::setBackgroundA0(double value) {
  m_model->setBackgroundA0(value);
  updateViewParameters();
}

void MultiFunctionTemplatePresenter::setQValues(const std::vector<double> &qValues) { model()->setQValues(qValues); }

void MultiFunctionTemplatePresenter::setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  model()->setResolution(fitResolutions);
}

void MultiFunctionTemplatePresenter::updateView() {
  updateViewParameterNames();
  updateViewParameters();
  m_view->setGlobalParametersQuiet(m_model->getGlobalParameters());
}

void MultiFunctionTemplatePresenter::updateViewParameters() {
  auto templateView = view();
  auto templateModel = model();

  auto values = templateModel->getCurrentValues();
  auto errors = templateModel->getCurrentErrors();
  for (auto const &id : values.keys()) {
    templateView->setParameterValueQuiet(id, values[id], errors[id]);
  }
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

} // namespace MantidQt::CustomInterfaces::IDA
