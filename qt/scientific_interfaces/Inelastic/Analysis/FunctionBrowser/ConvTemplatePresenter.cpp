// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvTemplatePresenter.h"
#include "ConvTemplateBrowser.h"
#include "FitTypes.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
#include <QInputDialog>
#include <QtConcurrentRun>
#include <cmath>
#include <float.h>

namespace MantidQt::CustomInterfaces::IDA {

using namespace MantidWidgets;

ConvTemplatePresenter::ConvTemplatePresenter(ConvTemplateBrowser *view, std::unique_ptr<ConvFunctionModel> model)
    : FunctionTemplatePresenter(view, std::move(model)) {}

ConvTemplateBrowser *ConvTemplatePresenter::view() const { return dynamic_cast<ConvTemplateBrowser *>(m_view); }

ConvFunctionModel *ConvTemplatePresenter::model() const { return dynamic_cast<ConvFunctionModel *>(m_model.get()); }

// This function creates a Qt thread to run the model updates
// This was found to be necessary to allow the processing of the GUI thread to
// continue which is necessary to stop the int manager from self-incrementing
// itself due to an internal timer occurring within the class
void ConvTemplatePresenter::setSubType(size_t subTypeIndex, int typeIndex) {
  if (subTypeIndex == ConvTypes::SubTypeIndex::Fit) {
    model()->setFitType(static_cast<ConvTypes::FitType>(typeIndex));
  } else if (subTypeIndex == ConvTypes::SubTypeIndex::Lorentzian) {
    model()->setLorentzianType(static_cast<ConvTypes::LorentzianType>(typeIndex));
  } else {
    model()->setBackground(static_cast<ConvTypes::BackgroundType>(typeIndex));
  }
  view()->setSubType(subTypeIndex, typeIndex);
  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  m_view->emitFunctionStructureChanged();
}

void ConvTemplatePresenter::setDeltaFunction(bool on) {
  if (on == model()->hasDeltaFunction())
    return;
  model()->setDeltaFunction(on);
  if (on)
    view()->addDeltaFunction();
  else
    view()->removeDeltaFunction();

  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  m_view->emitFunctionStructureChanged();
}

void ConvTemplatePresenter::setTempCorrection(bool on) {
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

void ConvTemplatePresenter::setFunction(std::string const &funStr) {
  m_model->setFunctionString(funStr);

  auto convView = view();
  auto convModel = model();
  convView->updateTemperatureCorrectionAndDelta(convModel->hasTempCorrection(), convModel->hasDeltaFunction());

  convView->setSubType(ConvTypes::SubTypeIndex::Lorentzian, static_cast<int>(convModel->getLorentzianType()));
  convView->setSubType(ConvTypes::SubTypeIndex::Fit, static_cast<int>(convModel->getFitType()));
  convView->setSubType(ConvTypes::SubTypeIndex::Background, static_cast<int>(convModel->getBackgroundType()));

  convView->setInt(ConvTypes::SubTypeIndex::Lorentzian, static_cast<int>(convModel->getLorentzianType()));
  convView->setEnum(ConvTypes::SubTypeIndex::Fit, static_cast<int>(convModel->getFitType()));
  convView->setEnum(ConvTypes::SubTypeIndex::Background, static_cast<int>(convModel->getBackgroundType()));

  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  m_view->emitFunctionStructureChanged();
}

void ConvTemplatePresenter::setGlobalParameters(std::vector<std::string> const &globals) {
  m_model->setGlobalParameters(globals);
}

void ConvTemplatePresenter::setGlobal(std::string const &parameterName, bool on) {
  auto globals = m_model->getGlobalParameters();
  auto const findIter = std::find(globals.cbegin(), globals.cend(), parameterName);
  if (on) {
    if (findIter == globals.cend()) {
      globals.emplace_back(parameterName);
    }
  } else if (findIter != globals.cend()) {
    globals.erase(findIter);
  }
  setGlobalParameters(globals);
}

void ConvTemplatePresenter::updateMultiDatasetParameters(const IFunction &fun) {
  m_model->updateMultiDatasetParameters(fun);
  updateViewParameters();
}

void ConvTemplatePresenter::updateMultiDatasetParameters(const ITableWorkspace &table) {
  model()->updateMultiDatasetParameters(table);
  updateViewParameters();
}

void ConvTemplatePresenter::updateParameters(const IFunction &fun) {
  m_model->updateParameters(fun);
  updateViewParameters();
}

void ConvTemplatePresenter::setCurrentDataset(int i) {
  m_model->setCurrentDomainIndex(i);
  updateViewParameters();
}

void ConvTemplatePresenter::setBackgroundA0(double value) {
  m_model->setBackgroundA0(value);
  updateViewParameters();
}

void ConvTemplatePresenter::setQValues(const std::vector<double> &qValues) { model()->setQValues(qValues); }

void ConvTemplatePresenter::setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  model()->setResolution(fitResolutions);
}

void ConvTemplatePresenter::updateViewParameters() {
  auto values = model()->getCurrentValues();
  auto errors = model()->getCurrentErrors();
  for (auto const id : values.keys()) {
    view()->setParameterValueQuiet(id, values[id], errors[id]);
  }
}

void ConvTemplatePresenter::updateViewParameterNames() { m_view->updateParameterNames(model()->getParameterNameMap()); }

void ConvTemplatePresenter::handleEditLocalParameterFinished(std::string const &parameterName,
                                                             QList<double> const &values, QList<bool> const &fixes,
                                                             QStringList const &ties, QStringList const &constraints) {
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
    m_model->setLocalParameterConstraint(parameterName, i, constraints[i].toStdString());
  }
  updateViewParameters();
}

EstimationDataSelector ConvTemplatePresenter::getEstimationDataSelector() const {
  return model()->getEstimationDataSelector();
}

void ConvTemplatePresenter::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  model()->updateParameterEstimationData(std::move(data));
}

void ConvTemplatePresenter::estimateFunctionParameters() {
  model()->estimateFunctionParameters();
  updateViewParameters();
}

} // namespace MantidQt::CustomInterfaces::IDA
