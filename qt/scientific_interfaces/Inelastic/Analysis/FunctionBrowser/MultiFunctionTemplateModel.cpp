// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MultiFunctionTemplateModel.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"

namespace MantidQt::CustomInterfaces::IDA {

using namespace Mantid::API;

MultiFunctionTemplateModel::MultiFunctionTemplateModel(std::unique_ptr<FunctionModel> model,
                                                       std::unique_ptr<IDAFunctionParameterEstimation> estimators)
    : m_model(std::move(model)), m_parameterEstimation(std::move(estimators)) {}

bool MultiFunctionTemplateModel::hasFunction() const { return m_model->hasFunction(); }

IFunction_sptr MultiFunctionTemplateModel::getFullFunction() const { return m_model->getFullFunction(); }

IFunction_sptr MultiFunctionTemplateModel::getFitFunction() const { return m_model->getFitFunction(); }

IFunction_sptr MultiFunctionTemplateModel::getSingleFunction(int index) const {
  return m_model->getSingleFunction(index);
}

IFunction_sptr MultiFunctionTemplateModel::getCurrentFunction() const { return m_model->getCurrentFunction(); }

void MultiFunctionTemplateModel::setParameter(std::string const &parameterName, double value) {
  m_model->setParameter(parameterName, value);
}

void MultiFunctionTemplateModel::setParameterError(std::string const &parameterName, double value) {
  m_model->setParameterError(parameterName, value);
}

double MultiFunctionTemplateModel::getParameter(std::string const &parameterName) const {
  return m_model->getParameter(parameterName);
}

double MultiFunctionTemplateModel::getParameterError(std::string const &parameterName) const {
  return m_model->getParameterError(parameterName);
}

std::string MultiFunctionTemplateModel::getParameterDescription(std::string const &parameterName) const {
  return m_model->getParameterDescription(parameterName);
}

std::vector<std::string> MultiFunctionTemplateModel::getParameterNames() const { return m_model->getParameterNames(); }

void MultiFunctionTemplateModel::setNumberDomains(int n) { m_model->setNumberDomains(n); }

int MultiFunctionTemplateModel::getNumberDomains() const { return m_model->getNumberDomains(); }

void MultiFunctionTemplateModel::setDatasets(const QList<FunctionModelDataset> &datasets) {
  m_model->setDatasets(datasets);
}

QStringList MultiFunctionTemplateModel::getDatasetNames() const { return m_model->getDatasetNames(); }

QStringList MultiFunctionTemplateModel::getDatasetDomainNames() const { return m_model->getDatasetDomainNames(); }

void MultiFunctionTemplateModel::setCurrentDomainIndex(int i) { m_model->setCurrentDomainIndex(i); }

int MultiFunctionTemplateModel::currentDomainIndex() const { return m_model->currentDomainIndex(); }

void MultiFunctionTemplateModel::setGlobalParameters(std::vector<std::string> const &globals) {
  m_globals.clear();
  for (auto const &name : globals) {
    addGlobal(name);
  }
  auto newGlobals = makeGlobalList();
  m_model->setGlobalParameters(newGlobals);
}

std::vector<std::string> MultiFunctionTemplateModel::getGlobalParameters() const {
  return m_model->getGlobalParameters();
}

bool MultiFunctionTemplateModel::isGlobal(std::string const &parameterName) const {
  return m_model->isGlobal(parameterName);
}

void MultiFunctionTemplateModel::setGlobal(std::string const &parameterName, bool on) {
  if (parameterName.empty())
    return;
  if (on)
    addGlobal(parameterName);
  else
    removeGlobal(parameterName);
  auto globals = makeGlobalList();
  m_model->setGlobalParameters(globals);
}

std::vector<std::string> MultiFunctionTemplateModel::getLocalParameters() const {
  return m_model->getLocalParameters();
}

void MultiFunctionTemplateModel::updateMultiDatasetParameters(const IFunction &fun) {
  m_model->updateMultiDatasetParameters(fun);
}

void MultiFunctionTemplateModel::updateMultiDatasetParameters(const ITableWorkspace &paramTable) {
  auto const nRows = paramTable.rowCount();
  if (nRows == 0)
    return;

  auto const globalParameterNames = getGlobalParameters();
  for (auto &&name : globalParameterNames) {
    auto valueColumn = paramTable.getColumn(name);
    auto errorColumn = paramTable.getColumn(name + "_Err");
    m_model->setParameter(name, valueColumn->toDouble(0));
    m_model->setParameterError(name, errorColumn->toDouble(0));
  }

  auto const localParameterNames = getLocalParameters();
  for (auto &&name : localParameterNames) {
    auto valueColumn = paramTable.getColumn(name);
    auto errorColumn = paramTable.getColumn(name + "_Err");
    if (nRows > 1) {
      for (size_t i = 0; i < nRows; ++i) {
        m_model->setLocalParameterValue(name, static_cast<int>(i), valueColumn->toDouble(i), errorColumn->toDouble(i));
      }
    } else {
      auto const i = m_model->currentDomainIndex();
      m_model->setLocalParameterValue(name, static_cast<int>(i), valueColumn->toDouble(0), errorColumn->toDouble(0));
    }
  }
}

void MultiFunctionTemplateModel::updateParameters(const IFunction &fun) { m_model->updateParameters(fun); }

double MultiFunctionTemplateModel::getLocalParameterValue(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterValue(parameterName, i);
}

bool MultiFunctionTemplateModel::isLocalParameterFixed(std::string const &parameterName, int i) const {
  return m_model->isLocalParameterFixed(parameterName, i);
}

std::string MultiFunctionTemplateModel::getLocalParameterTie(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterTie(parameterName, i);
}

std::string MultiFunctionTemplateModel::getLocalParameterConstraint(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterConstraint(parameterName, i);
}

void MultiFunctionTemplateModel::setLocalParameterValue(std::string const &parameterName, int i, double value) {
  m_model->setLocalParameterValue(parameterName, i, value);
}

void MultiFunctionTemplateModel::setLocalParameterValue(std::string const &parameterName, int i, double value,
                                                        double error) {
  m_model->setLocalParameterValue(parameterName, i, value, error);
}

void MultiFunctionTemplateModel::setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie) {
  m_model->setLocalParameterTie(parameterName, i, tie);
}

void MultiFunctionTemplateModel::setLocalParameterConstraint(std::string const &parameterName, int i,
                                                             std::string const &constraint) {
  m_model->setLocalParameterConstraint(parameterName, i, constraint);
}

void MultiFunctionTemplateModel::setLocalParameterFixed(std::string const &parameterName, int i, bool fixed) {
  m_model->setLocalParameterFixed(parameterName, i, fixed);
}

void MultiFunctionTemplateModel::setGlobalParameterValue(std::string const &parameterName, double value) {
  m_model->setGlobalParameterValue(parameterName, value);
}

void MultiFunctionTemplateModel::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_estimationData = std::move(data);
}

void MultiFunctionTemplateModel::estimateFunctionParameters() {
  m_parameterEstimation->estimateFunctionParameters(getFullFunction(), m_estimationData);
}

QMap<ParamID, double> MultiFunctionTemplateModel::getCurrentValues() const {
  QMap<ParamID, double> values;
  auto store = [&values, this](ParamID name) { values[name] = *getParameter(name); };
  applyParameterFunction(store);
  return values;
}

QMap<ParamID, double> MultiFunctionTemplateModel::getCurrentErrors() const {
  QMap<ParamID, double> errors;
  auto store = [&errors, this](ParamID name) { errors[name] = *getParameterError(name); };
  applyParameterFunction(store);
  return errors;
}

QMap<int, std::string> MultiFunctionTemplateModel::getParameterNameMap() const {
  QMap<int, std::string> out;
  auto addToMap = [&out, this](ParamID name) { out[static_cast<int>(name)] = *getParameterName(name); };
  applyParameterFunction(addToMap);
  return out;
}

std::vector<std::string> MultiFunctionTemplateModel::makeGlobalList() const {
  std::vector<std::string> globals;
  for (auto const id : m_globals) {
    auto const name = getParameterName(id);
    if (name)
      globals.emplace_back(*name);
  }
  return globals;
}

void MultiFunctionTemplateModel::setParameter(ParamID name, double value) {
  auto const prefix = getPrefix(name);
  if (prefix) {
    m_model->setParameter(*prefix + g_paramName.at(name), value);
  }
}

void MultiFunctionTemplateModel::setCurrentValues(const QMap<ParamID, double> &values) {
  for (auto const name : values.keys()) {
    setParameter(name, values[name]);
  }
}

std::optional<double> MultiFunctionTemplateModel::getParameter(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model->getParameter(*paramName) : std::optional<double>();
}

std::optional<double> MultiFunctionTemplateModel::getParameterError(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model->getParameterError(*paramName) : std::optional<double>();
}

std::optional<std::string> MultiFunctionTemplateModel::getParameterName(ParamID name) const {
  auto const prefix = getPrefix(name);
  return prefix ? *prefix + g_paramName.at(name) : std::optional<std::string>();
}

std::optional<std::string> MultiFunctionTemplateModel::getParameterDescription(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model->getParameterDescription(*paramName) : std::optional<std::string>();
}

std::optional<ParamID> MultiFunctionTemplateModel::getParameterId(std::string const &parameterName) {
  std::optional<ParamID> result;
  auto getter = [&result, parameterName, this](ParamID pid) {
    if (parameterName == *getParameterName(pid))
      result = pid;
  };
  applyParameterFunction(getter);
  return result;
}

void MultiFunctionTemplateModel::changeTie(std::string const &parameterName, std::string const &tie) {
  m_model->changeTie(parameterName, tie);
}

void MultiFunctionTemplateModel::addConstraint(std::string const &functionIndex, std::string const &constraint) {
  m_model->addConstraint(functionIndex, constraint);
}

void MultiFunctionTemplateModel::removeConstraint(std::string const &parameterName) {
  m_model->removeConstraint(parameterName);
}

void MultiFunctionTemplateModel::removeGlobal(std::string const &parameterName) {
  auto const pid = getParameterId(parameterName);
  if (pid && m_globals.contains(*pid)) {
    m_globals.removeOne(*pid);
  }
}

void MultiFunctionTemplateModel::addGlobal(std::string const &parameterName) {
  auto const pid = getParameterId(parameterName);
  if (pid && !m_globals.contains(*pid)) {
    m_globals.push_back(*pid);
  }
}

} // namespace MantidQt::CustomInterfaces::IDA
