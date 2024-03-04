// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MultiFunctionTemplatePresenter.h"
#include "FitTypes.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
#include "MultiFunctionTemplateView.h"
#include <QtConcurrentRun>
#include <cmath>
#include <float.h>

namespace MantidQt::CustomInterfaces::IDA {

using namespace MantidWidgets;

MultiFunctionTemplatePresenter::MultiFunctionTemplatePresenter(MultiFunctionTemplateView *view,
                                                               std::unique_ptr<MultiFunctionTemplateModel> model)
    : FunctionTemplatePresenter(view, std::move(model)) {}

MultiFunctionTemplateView *MultiFunctionTemplatePresenter::view() const {
  return dynamic_cast<MultiFunctionTemplateView *>(m_view);
}

MultiFunctionTemplateModel *MultiFunctionTemplatePresenter::model() const {
  return dynamic_cast<MultiFunctionTemplateModel *>(m_model.get());
}

// This function creates a Qt thread to run the model updates
// This was found to be necessary to allow the processing of the GUI thread to
// continue which is necessary to stop the int manager from self-incrementing
// itself due to an internal timer occurring within the class
void MultiFunctionTemplatePresenter::setSubType(size_t subTypeIndex, int typeIndex) {
  if (subTypeIndex == ConvTypes::SubTypeIndex::Fit) {
    model()->setFitType(static_cast<ConvTypes::FitType>(typeIndex));
  } else if (subTypeIndex == ConvTypes::SubTypeIndex::Lorentzian) {
    model()->setLorentzianType(static_cast<ConvTypes::LorentzianType>(typeIndex));
  } else if (subTypeIndex == ConvTypes::SubTypeIndex::Delta) {
    model()->setDeltaType(static_cast<ConvTypes::DeltaType>(typeIndex));
  } else if (subTypeIndex == ConvTypes::SubTypeIndex::TempCorrection) {
    model()->setTempCorrectionType(static_cast<ConvTypes::TempCorrectionType>(typeIndex));
  } else {
    model()->setBackground(static_cast<ConvTypes::BackgroundType>(typeIndex));
  }
  view()->setSubType(subTypeIndex, typeIndex);
  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  view()->setGlobalParametersQuiet(model()->getGlobalParameters());
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
  updateViewParameterNames();
  updateViewParameters();
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
}

void MultiFunctionTemplatePresenter::updateViewParameters() {
  auto values = model()->getCurrentValues();
  auto errors = model()->getCurrentErrors();
  for (auto const id : values.keys()) {
    view()->setParameterValueQuiet(id, values[id], errors[id]);
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
