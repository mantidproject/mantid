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

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace MantidWidgets;

/**
 * Constructor
 * @param parent :: The parent widget.
 */
SingleFunctionTemplatePresenter::SingleFunctionTemplatePresenter(
    SingleFunctionTemplateBrowser *view, const std::map<std::string, std::string> &functionInitialisationStrings,
    std::unique_ptr<IDAFunctionParameterEstimation> parameterEstimation)
    : QObject(view), m_view(view), m_model(std::move(parameterEstimation)) {
  connect(m_view, SIGNAL(localParameterButtonClicked(const QString &)), this,
          SLOT(editLocalParameter(const QString &)));
  connect(m_view, SIGNAL(parameterValueChanged(const QString &, double)), this,
          SLOT(viewChangedParameterValue(const QString &, double)));

  m_model.updateAvailableFunctions(functionInitialisationStrings);
}

void SingleFunctionTemplatePresenter::init() {
  m_view->setDataType(m_model.getFunctionList());
  setFitType(m_model.getFitType());
}

void SingleFunctionTemplatePresenter::updateAvailableFunctions(
    const std::map<std::string, std::string> &functionInitialisationStrings) {
  m_model.updateAvailableFunctions(functionInitialisationStrings);
  m_view->setDataType(m_model.getFunctionList());
  setFitType(m_model.getFitType());
}

void SingleFunctionTemplatePresenter::setFitType(const QString &name) {
  m_view->clear();
  m_model.setFitType(name);
  auto functionParameters = m_model.getParameterNames();
  for (auto &parameter : functionParameters) {
    m_view->addParameter(parameter, m_model.getParameterDescription(parameter));
  }
  setErrorsEnabled(false);
  updateView();
  emit functionStructureChanged();
}

void SingleFunctionTemplatePresenter::setNumberOfDatasets(int n) { m_model.setNumberDomains(n); }

int SingleFunctionTemplatePresenter::getNumberOfDatasets() const { return m_model.getNumberDomains(); }

int SingleFunctionTemplatePresenter::getCurrentDataset() { return m_model.currentDomainIndex(); }

void SingleFunctionTemplatePresenter::setFunction(const QString &funStr) {
  m_view->clear();
  m_model.setFunctionString(funStr);

  if (m_model.getFitType() == "None")
    return;
  auto functionParameters = m_model.getParameterNames();
  for (auto &parameter : functionParameters) {
    m_view->addParameter(parameter, m_model.getParameterDescription(parameter));
  }
  m_view->setEnumValue(m_model.getEnumIndex());
  setErrorsEnabled(false);
  updateView();
  emit functionStructureChanged();
}

IFunction_sptr SingleFunctionTemplatePresenter::getGlobalFunction() const { return m_model.getFitFunction(); }

IFunction_sptr SingleFunctionTemplatePresenter::getFunction() const { return m_model.getCurrentFunction(); }

QStringList SingleFunctionTemplatePresenter::getGlobalParameters() const { return m_model.getGlobalParameters(); }

QStringList SingleFunctionTemplatePresenter::getLocalParameters() const { return m_model.getLocalParameters(); }

void SingleFunctionTemplatePresenter::setGlobalParameters(const QStringList &globals) {
  m_model.setGlobalParameters(globals);
  m_view->setGlobalParametersQuiet(globals);
}

void SingleFunctionTemplatePresenter::setGlobal(const QString &parName, bool on) {
  m_model.setGlobal(parName, on);
  m_view->setGlobalParametersQuiet(m_model.getGlobalParameters());
}

void SingleFunctionTemplatePresenter::updateMultiDatasetParameters(const IFunction &fun) {
  m_model.updateMultiDatasetParameters(fun);
  updateView();
}

void SingleFunctionTemplatePresenter::updateParameters(const IFunction &fun) {
  m_model.updateParameters(fun);
  updateView();
}

void SingleFunctionTemplatePresenter::setCurrentDataset(int i) {
  m_model.setCurrentDomainIndex(i);
  updateView();
}

void SingleFunctionTemplatePresenter::setDatasets(const QList<FunctionModelDataset> &datasets) {
  m_model.setDatasets(datasets);
}

void SingleFunctionTemplatePresenter::setErrorsEnabled(bool enabled) { m_view->setErrorsEnabled(enabled); }

void SingleFunctionTemplatePresenter::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_model.updateParameterEstimationData(std::move(data));
  updateView();
}
void SingleFunctionTemplatePresenter::estimateFunctionParameters() {
  m_model.estimateFunctionParameters();
  updateView();
}

QStringList SingleFunctionTemplatePresenter::getDatasetNames() const { return m_model.getDatasetNames(); }

QStringList SingleFunctionTemplatePresenter::getDatasetDomainNames() const { return m_model.getDatasetDomainNames(); }

double SingleFunctionTemplatePresenter::getLocalParameterValue(const QString &parName, int i) const {
  return m_model.getLocalParameterValue(parName, i);
}

bool SingleFunctionTemplatePresenter::isLocalParameterFixed(const QString &parName, int i) const {
  return m_model.isLocalParameterFixed(parName, i);
}

QString SingleFunctionTemplatePresenter::getLocalParameterTie(const QString &parName, int i) const {
  return m_model.getLocalParameterTie(parName, i);
}

QString SingleFunctionTemplatePresenter::getLocalParameterConstraint(const QString &parName, int i) const {
  return m_model.getLocalParameterConstraint(parName, i);
}

void SingleFunctionTemplatePresenter::setLocalParameterValue(const QString &parName, int i, double value) {
  m_model.setLocalParameterValue(parName, i, value);
}

void SingleFunctionTemplatePresenter::setLocalParameterTie(const QString &parName, int i, const QString &tie) {
  m_model.setLocalParameterTie(parName, i, tie);
}

void SingleFunctionTemplatePresenter::updateView() {
  if (m_model.getFitType() == "None")
    return;
  for (auto &parameterName : m_model.getParameterNames()) {
    m_view->setParameterValueQuietly(parameterName, m_model.getParameter(parameterName),
                                     m_model.getParameterError(parameterName));
  }
}

void SingleFunctionTemplatePresenter::setLocalParameterFixed(const QString &parName, int i, bool fixed) {
  m_model.setLocalParameterFixed(parName, i, fixed);
}

void SingleFunctionTemplatePresenter::editLocalParameter(const QString &parName) {
  auto const datasetNames = getDatasetNames();
  auto const domainNames = getDatasetDomainNames();
  QList<double> values;
  QList<bool> fixes;
  QStringList ties;
  QStringList constraints;
  const int n = domainNames.size();
  for (auto i = 0; i < n; ++i) {
    const double value = getLocalParameterValue(parName, i);
    values.push_back(value);
    const bool fixed = isLocalParameterFixed(parName, i);
    fixes.push_back(fixed);
    const auto tie = getLocalParameterTie(parName, i);
    ties.push_back(tie);
    const auto constraint = getLocalParameterConstraint(parName, i);
    constraints.push_back(constraint);
  }

  m_editLocalParameterDialog =
      new EditLocalParameterDialog(m_view, parName, datasetNames, domainNames, values, fixes, ties, constraints);
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
        setLocalParameterTie(parName, i, ties[i]);
      } else if (fixes[i]) {
        setLocalParameterFixed(parName, i, fixes[i]);
      } else {
        setLocalParameterTie(parName, i, "");
      }
    }
  }
  m_editLocalParameterDialog = nullptr;
  updateView();
  emit functionStructureChanged();
}

void SingleFunctionTemplatePresenter::viewChangedParameterValue(const QString &parName, double value) {
  if (parName.isEmpty())
    return;
  if (m_model.isGlobal(parName)) {
    const auto n = getNumberOfDatasets();
    for (int i = 0; i < n; ++i) {
      setLocalParameterValue(parName, i, value);
    }
  } else {
    const auto i = m_model.currentDomainIndex();
    const auto oldValue = m_model.getLocalParameterValue(parName, i);
    if (fabs(value - oldValue) > 1e-6) {
      setErrorsEnabled(false);
    }
    setLocalParameterValue(parName, i, value);
  }
  emit functionStructureChanged();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
