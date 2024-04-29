// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FunctionTemplatePresenter.h"
#include "FunctionTemplateView.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

FunctionTemplatePresenter::FunctionTemplatePresenter(FunctionTemplateView *view,
                                                     std::unique_ptr<MantidWidgets::IFunctionModel> model)
    : m_view(view), m_model(std::move(model)) {
  m_view->subscribePresenter(this);
}

void FunctionTemplatePresenter::init() {}

void FunctionTemplatePresenter::updateAvailableFunctions(
    const std::map<std::string, std::string> &functionInitialisationStrings) {
  (void)functionInitialisationStrings;
}

void FunctionTemplatePresenter::setNumberOfDatasets(int n) { m_model->setNumberDomains(n); }

int FunctionTemplatePresenter::getNumberOfDatasets() const { return m_model->getNumberDomains(); }

int FunctionTemplatePresenter::getCurrentDataset() { return m_model->currentDomainIndex(); }

Mantid::API::IFunction_sptr FunctionTemplatePresenter::getGlobalFunction() const { return m_model->getFitFunction(); }

Mantid::API::IFunction_sptr FunctionTemplatePresenter::getFunction() const { return m_model->getCurrentFunction(); }

std::vector<std::string> FunctionTemplatePresenter::getGlobalParameters() const {
  return m_model->getGlobalParameters();
}

std::vector<std::string> FunctionTemplatePresenter::getLocalParameters() const { return m_model->getLocalParameters(); }

void FunctionTemplatePresenter::setLocalParameterValue(std::string const &parameterName, int i, double value) {
  m_model->setLocalParameterValue(parameterName, i, value);
}

void FunctionTemplatePresenter::setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie) {
  m_model->setLocalParameterTie(parameterName, i, tie);
}

void FunctionTemplatePresenter::setLocalParameterFixed(std::string const &parameterName, int i, bool fixed) {
  m_model->setLocalParameterFixed(parameterName, i, fixed);
}

double FunctionTemplatePresenter::getLocalParameterValue(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterValue(parameterName, i);
}

bool FunctionTemplatePresenter::isLocalParameterFixed(std::string const &parameterName, int i) const {
  return m_model->isLocalParameterFixed(parameterName, i);
}

std::string FunctionTemplatePresenter::getLocalParameterTie(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterTie(parameterName, i);
}

std::string FunctionTemplatePresenter::getLocalParameterConstraint(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterConstraint(parameterName, i);
}

void FunctionTemplatePresenter::setDatasets(const QList<MantidWidgets::FunctionModelDataset> &datasets) {
  m_model->setDatasets(datasets);
}

std::vector<std::string> FunctionTemplatePresenter::getDatasetNames() const { return m_model->getDatasetNames(); }

std::vector<std::string> FunctionTemplatePresenter::getDatasetDomainNames() const {
  return m_model->getDatasetDomainNames();
}

void FunctionTemplatePresenter::setErrorsEnabled(bool enabled) { m_view->setErrorsEnabled(enabled); }

void FunctionTemplatePresenter::handleParameterValueChanged(std::string const &parameterName, double value) {
  if (parameterName.empty())
    return;
  if (m_model->isGlobal(parameterName)) {
    auto const n = getNumberOfDatasets();
    for (int i = 0; i < n; ++i) {
      setLocalParameterValue(parameterName, i, value);
    }
  } else {
    auto const i = m_model->currentDomainIndex();
    auto const oldValue = m_model->getLocalParameterValue(parameterName, i);
    if (fabs(value - oldValue) > 1e-6) {
      setErrorsEnabled(false);
    }
    setLocalParameterValue(parameterName, i, value);
  }
  m_view->emitFunctionStructureChanged();
}

void FunctionTemplatePresenter::handleEditLocalParameterFinished(std::string const &parameterName,
                                                                 QList<double> const &values, QList<bool> const &fixes,
                                                                 QStringList const &ties,
                                                                 QStringList const &constraints) {
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
  updateView();
}

void FunctionTemplatePresenter::handleEditLocalParameter(std::string const &parameterName) {
  auto const datasetNames = getDatasetNames();
  auto const domainNames = getDatasetDomainNames();
  QList<double> values;
  QList<bool> fixes;
  QStringList ties;
  QStringList constraints;
  for (auto i = 0; i < static_cast<int>(domainNames.size()); ++i) {
    const double value = getLocalParameterValue(parameterName, i);
    values.push_back(value);
    const bool fixed = isLocalParameterFixed(parameterName, i);
    fixes.push_back(fixed);
    const auto tie = getLocalParameterTie(parameterName, i);
    ties.push_back(QString::fromStdString(tie));
    const auto constraint = getLocalParameterConstraint(parameterName, i);
    constraints.push_back(QString::fromStdString(constraint));
  }
  m_view->openEditLocalParameterDialog(parameterName, datasetNames, domainNames, values, fixes, ties, constraints);
}

void FunctionTemplatePresenter::setFitType(std::string const &name) { (void)name; }

void FunctionTemplatePresenter::updateMultiDatasetParameters(const IFunction &fun) {
  m_model->updateMultiDatasetParameters(fun);
  updateView();
}

void FunctionTemplatePresenter::updateMultiDatasetParameters(const ITableWorkspace &table) {
  m_model->updateMultiDatasetParameters(table);
  updateView();
}

void FunctionTemplatePresenter::updateParameters(const IFunction &fun) {
  m_model->updateParameters(fun);
  updateView();
}

void FunctionTemplatePresenter::setCurrentDataset(int i) {
  m_model->setCurrentDomainIndex(i);
  updateView();
}

void FunctionTemplatePresenter::setGlobalParameters(std::vector<std::string> const &globals) {
  m_model->setGlobalParameters(globals);
  m_view->setGlobalParametersQuiet(globals);
}

void FunctionTemplatePresenter::setGlobal(std::string const &parameterName, bool on) {
  m_model->setGlobal(parameterName, on);
  m_view->setGlobalParametersQuiet(m_model->getGlobalParameters());
}

void FunctionTemplatePresenter::setNumberOfExponentials(int nExponentials) { (void)nExponentials; }

void FunctionTemplatePresenter::setStretchExponential(bool on) { (void)on; }

void FunctionTemplatePresenter::setBackground(std::string const &name) { (void)name; }

void FunctionTemplatePresenter::tieIntensities(bool on) { (void)on; }

bool FunctionTemplatePresenter::canTieIntensities() const { return true; }

void FunctionTemplatePresenter::setSubType(std::size_t subTypeIndex, int typeIndex) {
  (void)subTypeIndex;
  (void)typeIndex;
}

void FunctionTemplatePresenter::setDeltaFunction(bool on) { (void)on; }

void FunctionTemplatePresenter::setTempCorrection(bool on) { (void)on; }

void FunctionTemplatePresenter::setBackgroundA0(double value) {
  m_model->setBackgroundA0(value);
  updateView();
}

void FunctionTemplatePresenter::setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  m_model->setResolution(fitResolutions);
}

void FunctionTemplatePresenter::setQValues(const std::vector<double> &qValues) { m_model->setQValues(qValues); }

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt