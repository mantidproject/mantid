// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SingleFunctionTemplatePresenter.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
#include "SingleFunctionTemplateBrowser.h"
#include <math.h>

namespace MantidQt::CustomInterfaces::IDA {

using namespace MantidWidgets;

/**
 * Constructor
 * @param parent :: The parent widget.
 */
SingleFunctionTemplatePresenter::SingleFunctionTemplatePresenter(
    SingleFunctionTemplateBrowser *view, std::unique_ptr<SingleFunctionTemplateModel> functionModel)
    : QObject(view), m_view(view), m_model(std::move(functionModel)) {
  m_view->subscribePresenter(this);
}

void SingleFunctionTemplatePresenter::init() {
  m_view->setDataType(m_model->getFunctionList());
  setFitType(m_model->getFitType());
}

void SingleFunctionTemplatePresenter::updateAvailableFunctions(
    const std::map<std::string, std::string> &functionInitialisationStrings) {
  m_model->updateAvailableFunctions(functionInitialisationStrings);
  m_view->setDataType(m_model->getFunctionList());
  setFitType(m_model->getFitType());
}

void SingleFunctionTemplatePresenter::setFitType(std::string const &name) {
  m_view->clear();
  m_model->setFitType(name);
  auto functionParameters = m_model->getParameterNames();
  for (auto &parameter : functionParameters) {
    m_view->addParameter(parameter, m_model->getParameterDescription(parameter));
  }
  setErrorsEnabled(false);
  updateView();
  m_view->emitFunctionStructureChanged();
}

void SingleFunctionTemplatePresenter::setNumberOfDatasets(int n) { m_model->setNumberDomains(n); }

int SingleFunctionTemplatePresenter::getNumberOfDatasets() const { return m_model->getNumberDomains(); }

int SingleFunctionTemplatePresenter::getCurrentDataset() { return m_model->currentDomainIndex(); }

void SingleFunctionTemplatePresenter::setFunction(std::string const &funStr) {
  m_view->clear();
  m_model->setFunctionString(funStr);

  if (m_model->getFitType() == "None")
    return;
  auto functionParameters = m_model->getParameterNames();
  for (auto &parameter : functionParameters) {
    m_view->addParameter(parameter, m_model->getParameterDescription(parameter));
  }
  m_view->setEnumValue(m_model->getEnumIndex());
  setErrorsEnabled(false);
  updateView();
  m_view->emitFunctionStructureChanged();
}

IFunction_sptr SingleFunctionTemplatePresenter::getGlobalFunction() const { return m_model->getFitFunction(); }

IFunction_sptr SingleFunctionTemplatePresenter::getFunction() const { return m_model->getCurrentFunction(); }

std::vector<std::string> SingleFunctionTemplatePresenter::getGlobalParameters() const {
  return m_model->getGlobalParameters();
}

std::vector<std::string> SingleFunctionTemplatePresenter::getLocalParameters() const {
  return m_model->getLocalParameters();
}

void SingleFunctionTemplatePresenter::setGlobalParameters(std::vector<std::string> const &globals) {
  m_model->setGlobalParameters(globals);
  m_view->setGlobalParametersQuiet(globals);
}

void SingleFunctionTemplatePresenter::setGlobal(std::string const &parameterName, bool on) {
  m_model->setGlobal(parameterName, on);
  m_view->setGlobalParametersQuiet(m_model->getGlobalParameters());
}

void SingleFunctionTemplatePresenter::updateMultiDatasetParameters(const IFunction &fun) {
  m_model->updateMultiDatasetParameters(fun);
  updateView();
}

void SingleFunctionTemplatePresenter::updateParameters(const IFunction &fun) {
  m_model->updateParameters(fun);
  updateView();
}

void SingleFunctionTemplatePresenter::setCurrentDataset(int i) {
  m_model->setCurrentDomainIndex(i);
  updateView();
}

void SingleFunctionTemplatePresenter::setDatasets(const QList<FunctionModelDataset> &datasets) {
  m_model->setDatasets(datasets);
}

void SingleFunctionTemplatePresenter::setErrorsEnabled(bool enabled) { m_view->setErrorsEnabled(enabled); }

EstimationDataSelector SingleFunctionTemplatePresenter::getEstimationDataSelector() const {
  return m_model->getEstimationDataSelector();
}

void SingleFunctionTemplatePresenter::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_model->updateParameterEstimationData(std::move(data));
  updateView();
}
void SingleFunctionTemplatePresenter::estimateFunctionParameters() {
  m_model->estimateFunctionParameters();
  updateView();
}

QStringList SingleFunctionTemplatePresenter::getDatasetNames() const { return m_model->getDatasetNames(); }

QStringList SingleFunctionTemplatePresenter::getDatasetDomainNames() const { return m_model->getDatasetDomainNames(); }

double SingleFunctionTemplatePresenter::getLocalParameterValue(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterValue(parameterName, i);
}

bool SingleFunctionTemplatePresenter::isLocalParameterFixed(std::string const &parameterName, int i) const {
  return m_model->isLocalParameterFixed(parameterName, i);
}

std::string SingleFunctionTemplatePresenter::getLocalParameterTie(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterTie(parameterName, i);
}

std::string SingleFunctionTemplatePresenter::getLocalParameterConstraint(std::string const &parameterName,
                                                                         int i) const {
  return m_model->getLocalParameterConstraint(parameterName, i);
}

void SingleFunctionTemplatePresenter::setLocalParameterValue(std::string const &parameterName, int i, double value) {
  m_model->setLocalParameterValue(parameterName, i, value);
}

void SingleFunctionTemplatePresenter::setLocalParameterTie(std::string const &parameterName, int i,
                                                           std::string const &tie) {
  m_model->setLocalParameterTie(parameterName, i, tie);
}

void SingleFunctionTemplatePresenter::updateView() {
  if (m_model->getFitType() == "None")
    return;
  for (auto const &parameterName : m_model->getParameterNames()) {
    m_view->setParameterValueQuietly(parameterName, m_model->getParameter(parameterName),
                                     m_model->getParameterError(parameterName));
  }
}

void SingleFunctionTemplatePresenter::setLocalParameterFixed(std::string const &parameterName, int i, bool fixed) {
  m_model->setLocalParameterFixed(parameterName, i, fixed);
}

void SingleFunctionTemplatePresenter::handleEditLocalParameter(std::string const &parameterName) {
  auto const datasetNames = getDatasetNames();
  auto const domainNames = getDatasetDomainNames();
  QList<double> values;
  QList<bool> fixes;
  QStringList ties;
  QStringList constraints;
  const int n = domainNames.size();
  for (auto i = 0; i < n; ++i) {
    const double value = getLocalParameterValue(parameterName, i);
    values.push_back(value);
    const bool fixed = isLocalParameterFixed(parameterName, i);
    fixes.push_back(fixed);
    const auto tie = getLocalParameterTie(parameterName, i);
    ties.push_back(QString::fromStdString(tie));
    const auto constraint = getLocalParameterConstraint(parameterName, i);
    constraints.push_back(QString::fromStdString(constraint));
  }

  m_editLocalParameterDialog =
      new EditLocalParameterDialog(m_view, parameterName, datasetNames, domainNames, values, fixes, ties, constraints);
  connect(m_editLocalParameterDialog, SIGNAL(finished(int)), this, SLOT(editLocalParameterFinish(int)));
  m_editLocalParameterDialog->open();
}

void SingleFunctionTemplatePresenter::editLocalParameterFinish(int result) {
  if (result == QDialog::Accepted) {
    const auto parName = m_editLocalParameterDialog->getParameterName();
    const auto values = m_editLocalParameterDialog->getValues();
    const auto fixes = m_editLocalParameterDialog->getFixes();
    const auto ties = m_editLocalParameterDialog->getTies();
    assert(values.size() == getNumberOfDatasets());
    for (int i = 0; i < values.size(); ++i) {
      setLocalParameterValue(parName, i, values[i]);
      if (!ties[i].isEmpty()) {
        setLocalParameterTie(parName, i, ties[i].toStdString());
      } else if (fixes[i]) {
        setLocalParameterFixed(parName, i, fixes[i]);
      } else {
        setLocalParameterTie(parName, i, "");
      }
    }
  }
  m_editLocalParameterDialog = nullptr;
  updateView();
  m_view->emitFunctionStructureChanged();
}

void SingleFunctionTemplatePresenter::handleParameterValueChanged(std::string const &parameterName, double value) {
  if (parameterName.empty())
    return;
  if (m_model->isGlobal(parameterName)) {
    const auto n = getNumberOfDatasets();
    for (int i = 0; i < n; ++i) {
      setLocalParameterValue(parameterName, i, value);
    }
  } else {
    const auto i = m_model->currentDomainIndex();
    const auto oldValue = m_model->getLocalParameterValue(parameterName, i);
    if (fabs(value - oldValue) > 1e-6) {
      setErrorsEnabled(false);
    }
    setLocalParameterValue(parameterName, i, value);
  }
  m_view->emitFunctionStructureChanged();
}

} // namespace MantidQt::CustomInterfaces::IDA
