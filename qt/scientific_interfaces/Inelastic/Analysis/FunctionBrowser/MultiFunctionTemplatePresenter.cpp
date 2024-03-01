// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MultiFunctionTemplatePresenter.h"
#include "FitTypes.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
#include "MultiFunctionTemplateBrowser.h"
#include <QInputDialog>
#include <QtConcurrentRun>
#include <cmath>
#include <float.h>

namespace MantidQt::CustomInterfaces::IDA {

using namespace MantidWidgets;

MultiFunctionTemplatePresenter::MultiFunctionTemplatePresenter(MultiFunctionTemplateBrowser *view,
                                                               std::unique_ptr<MultiFunctionTemplateModel> model)
    : FunctionTemplatePresenter(view, std::move(model)) {}

MultiFunctionTemplateBrowser *MultiFunctionTemplatePresenter::view() const {
  return dynamic_cast<MultiFunctionTemplateBrowser *>(m_view);
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
  } else {
    model()->setBackground(static_cast<ConvTypes::BackgroundType>(typeIndex));
  }
  view()->setSubType(subTypeIndex, typeIndex);
  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  m_view->emitFunctionStructureChanged();
}

void MultiFunctionTemplatePresenter::setTempCorrection(bool on) {
  if (on == model()->hasTempCorrection())
    return;
  double temp = model()->getTempValue();
  if (on) {
    bool ok;
    temp = QInputDialog::getDouble(m_view, "Temperature", "Set Temperature", temp, 0.0,
                                   std::numeric_limits<double>::max(), 3, &ok);
    if (!ok)
      return;
  }
  model()->setTempCorrection(on, temp);
  if (on)
    view()->addTempCorrection(temp);
  else
    view()->removeTempCorrection();

  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  m_view->emitFunctionStructureChanged();
}

void MultiFunctionTemplatePresenter::setFunction(std::string const &funStr) {
  m_model->setFunctionString(funStr);

  MultiFunctionTemplateBrowser *convView = view();
  MultiFunctionTemplateModel const *convModel = model();
  convView->updateTemperatureCorrectionAndDelta(convModel->hasTempCorrection());

  convView->setSubType(ConvTypes::SubTypeIndex::Lorentzian, static_cast<int>(convModel->getLorentzianType()));
  convView->setSubType(ConvTypes::SubTypeIndex::Fit, static_cast<int>(convModel->getFitType()));
  convView->setSubType(ConvTypes::SubTypeIndex::Delta, static_cast<int>(convModel->getDeltaType()));
  convView->setSubType(ConvTypes::SubTypeIndex::Background, static_cast<int>(convModel->getBackgroundType()));

  convView->setInt(ConvTypes::SubTypeIndex::Lorentzian, static_cast<int>(convModel->getLorentzianType()));
  convView->setEnum(ConvTypes::SubTypeIndex::Fit, static_cast<int>(convModel->getFitType()));
  convView->setBool(ConvTypes::SubTypeIndex::Delta, static_cast<bool>(convModel->getDeltaType()));
  convView->setEnum(ConvTypes::SubTypeIndex::Background, static_cast<int>(convModel->getBackgroundType()));

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
