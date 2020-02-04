// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "FQFunctionModel.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include <map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace MantidWidgets;
using namespace Mantid::API;

namespace {
std::map<FQFunctionModel::ParamID, QString> g_paramName{
    {FQFunctionModel::ParamID::GAUSSIAN_HEIGHT, "Height"},
    {FQFunctionModel::ParamID::GAUSSIAN_FQ, "Msd"},
    {FQFunctionModel::ParamID::PETERS_HEIGHT, "Height"},
    {FQFunctionModel::ParamID::PETERS_FQ, "Msd"},
    {FQFunctionModel::ParamID::PETERS_BETA, "Beta"},
    {FQFunctionModel::ParamID::YI_HEIGHT, "Height"},
    {FQFunctionModel::ParamID::YI_FQ, "Msd"},
    {FQFunctionModel::ParamID::YI_SIGMA, "Sigma"}};
}

FQFunctionModel::FQFunctionModel() {}

void FQFunctionModel::clearData() {
  m_fitType.clear();
  m_model.clear();
}

void FQFunctionModel::setFunction(IFunction_sptr fun) {
  clearData();
  if (!fun)
    return;
  if (fun->nFunctions() == 0) {
    const auto name = fun->name();
    if (name == "MSDGauss" || name == "MSDPeters" || name == "MSDYi") {
      m_fitType = QString::fromStdString(name);
    } else {
      throw std::runtime_error("Cannot set function " + name);
    }
    m_model.setFunction(fun);
    return;
  }
  bool isFitTypeSet = false;
  for (size_t i = 0; i < fun->nFunctions(); ++i) {
    const auto f = fun->getFunction(i);
    const auto name = f->name();
    if (name == "MSDGauss" || name == "MSDPeters" || name == "MSDYi") {
      if (isFitTypeSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      m_fitType = QString::fromStdString(name);
      isFitTypeSet = true;
    } else {
      clear();
      throw std::runtime_error("Function has wrong structure.");
    }
  }
  m_model.setFunction(fun);
}

IFunction_sptr FQFunctionModel::getFitFunction() const {
  return m_model.getFitFunction();
}

bool FQFunctionModel::hasFunction() const { return m_model.hasFunction(); }

void FQFunctionModel::addFunction(const QString &prefix,
                                  const QString &funStr) {
  if (!prefix.isEmpty())
    throw std::runtime_error(
        "Function doesn't have member function with prefix " +
        prefix.toStdString());
  const auto fun =
      FunctionFactory::Instance().createInitialized(funStr.toStdString());
  const auto name = fun->name();
  QString newPrefix;
  if (name == "MSDGauss" || name == "MSDPeters" || name == "MSDYi") {
    setFitType(QString::fromStdString(name));
    newPrefix = *getFitTypePrefix();
  } else {
    throw std::runtime_error("Cannot add function " + name);
  }
  auto newFun = getFunctionWithPrefix(newPrefix, getSingleFunction(0));
  copyParametersAndErrors(*fun, *newFun);
  if (getNumberLocalFunctions() > 1) {
    copyParametersAndErrorsToAllLocalFunctions(*getSingleFunction(0));
  }
}

void FQFunctionModel::removeFunction(const QString &prefix) {
  if (prefix.isEmpty()) {
    clear();
    return;
  }
  auto prefix1 = getFitTypePrefix();
  if (prefix1 && *prefix1 == prefix) {
    removeFitType();
    return;
  }
  throw std::runtime_error(
      "Function doesn't have member function with prefix " +
      prefix.toStdString());
}

void FQFunctionModel::setFitType(const QString &name) {
  const auto oldValues = getCurrentValues();
  m_fitType = name;
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

void FQFunctionModel::removeFitType() {
  const auto oldValues = getCurrentValues();
  m_fitType.clear();
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

bool FQFunctionModel::hasGaussianType() const {
  return m_fitType == "MSDGauss";
}

bool FQFunctionModel::hasPetersType() const { return m_fitType == "MSDPeters"; }

bool FQFunctionModel::hasYiType() const { return m_fitType == "MSDYi"; }

void FQFunctionModel::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_estimationData = std::move(data);
}

void FQFunctionModel::setNumberDomains(int n) { m_model.setNumberDomains(n); }

int FQFunctionModel::getNumberDomains() const {
  return m_model.getNumberDomains();
}

void FQFunctionModel::setParameter(const QString &paramName, double value) {
  m_model.setParameterError(paramName, value);
}

void FQFunctionModel::setParameterError(const QString &paramName,
                                        double value) {
  m_model.setParameterError(paramName, value);
}

double FQFunctionModel::getParameter(const QString &paramName) const {
  return m_model.getParameter(paramName);
}

double FQFunctionModel::getParameterError(const QString &paramName) const {
  return m_model.getParameterError(paramName);
}

QString
FQFunctionModel::getParameterDescription(const QString &paramName) const {
  return m_model.getParameterDescription(paramName);
}

QStringList FQFunctionModel::getParameterNames() const {
  return m_model.getParameterNames();
}

IFunction_sptr FQFunctionModel::getSingleFunction(int index) const {
  return m_model.getSingleFunction(index);
}

IFunction_sptr FQFunctionModel::getCurrentFunction() const {
  return m_model.getCurrentFunction();
}

QStringList FQFunctionModel::getGlobalParameters() const {
  return m_model.getGlobalParameters();
}

QStringList FQFunctionModel::getLocalParameters() const {
  return m_model.getLocalParameters();
}

void FQFunctionModel::setGlobalParameters(const QStringList &globals) {
  m_globals.clear();
  for (const auto &name : globals) {
    addGlobal(name);
  }
  auto newGlobals = makeGlobalList();
  m_model.setGlobalParameters(newGlobals);
}

bool FQFunctionModel::isGlobal(const QString &parName) const {
  return m_model.isGlobal(parName);
}

void FQFunctionModel::setGlobal(const QString &parName, bool on) {
  if (parName.isEmpty())
    return;
  if (on)
    addGlobal(parName);
  else
    removeGlobal(parName);
  auto globals = makeGlobalList();
  m_model.setGlobalParameters(globals);
}

void FQFunctionModel::addGlobal(const QString &parName) {
  const auto pid = getParameterId(parName);
  if (pid && !m_globals.contains(*pid)) {
    m_globals.push_back(*pid);
  }
}

void FQFunctionModel::removeGlobal(const QString &parName) {
  const auto pid = getParameterId(parName);
  if (pid && m_globals.contains(*pid)) {
    m_globals.removeOne(*pid);
  }
}

QStringList FQFunctionModel::makeGlobalList() const {
  QStringList globals;
  for (const auto &id : m_globals) {
    const auto name = getParameterName(id);
    if (name)
      globals << *name;
  }
  return globals;
}

void FQFunctionModel::updateMultiDatasetParameters(const IFunction &fun) {
  m_model.updateMultiDatasetParameters(fun);
}

void FQFunctionModel::updateMultiDatasetParameters(
    const ITableWorkspace &paramTable) {
  const auto nRows = paramTable.rowCount();
  if (nRows == 0)
    return;

  const auto globalParameterNames = getGlobalParameters();
  for (auto &&name : globalParameterNames) {
    const auto valueColumn = paramTable.getColumn(name.toStdString());
    const auto errorColumn =
        paramTable.getColumn((name + "_Err").toStdString());
    m_model.setParameter(name, valueColumn->toDouble(0));
    m_model.setParameterError(name, errorColumn->toDouble(0));
  }

  const auto localParameterNames = getLocalParameters();
  for (auto &&name : localParameterNames) {
    const auto valueColumn = paramTable.getColumn(name.toStdString());
    const auto errorColumn =
        paramTable.getColumn((name + "_Err").toStdString());
    if (nRows > 1) {
      for (size_t i = 0; i < nRows; ++i) {
        m_model.setLocalParameterValue(name, static_cast<int>(i),
                                       valueColumn->toDouble(i),
                                       errorColumn->toDouble(i));
      }
    } else {
      const auto i = m_model.currentDomainIndex();
      m_model.setLocalParameterValue(name, static_cast<int>(i),
                                     valueColumn->toDouble(0),
                                     errorColumn->toDouble(0));
    }
  }
}

void FQFunctionModel::updateParameters(const IFunction &fun) {
  m_model.updateParameters(fun);
}

void FQFunctionModel::setCurrentDomainIndex(int i) {
  m_model.setCurrentDomainIndex(i);
}

int FQFunctionModel::currentDomainIndex() const {
  return m_model.currentDomainIndex();
}

void FQFunctionModel::changeTie(const QString &paramName, const QString &tie) {
  m_model.changeTie(paramName, tie);
}

void FQFunctionModel::addConstraint(const QString &functionIndex,
                                    const QString &constraint) {
  m_model.addConstraint(functionIndex, constraint);
}

void FQFunctionModel::removeConstraint(const QString &paramName) {
  m_model.removeConstraint(paramName);
}

void FQFunctionModel::setDatasetNames(const QStringList &names) {
  m_model.setDatasetNames(names);
}

QStringList FQFunctionModel::getDatasetNames() const {
  return m_model.getDatasetNames();
}

double FQFunctionModel::getLocalParameterValue(const QString &parName,
                                               int i) const {
  return m_model.getLocalParameterValue(parName, i);
}

bool FQFunctionModel::isLocalParameterFixed(const QString &parName,
                                            int i) const {
  return m_model.isLocalParameterFixed(parName, i);
}

QString FQFunctionModel::getLocalParameterTie(const QString &parName,
                                              int i) const {
  return m_model.getLocalParameterTie(parName, i);
}

QString FQFunctionModel::getLocalParameterConstraint(const QString &parName,
                                                     int i) const {
  return m_model.getLocalParameterConstraint(parName, i);
}

void FQFunctionModel::setLocalParameterValue(const QString &parName, int i,
                                             double value) {
  m_model.setLocalParameterValue(parName, i, value);
}

void FQFunctionModel::setLocalParameterValue(const QString &parName, int i,
                                             double value, double error) {
  m_model.setLocalParameterValue(parName, i, value, error);
}

void FQFunctionModel::setLocalParameterTie(const QString &parName, int i,
                                           const QString &tie) {
  m_model.setLocalParameterTie(parName, i, tie);
}

void FQFunctionModel::setLocalParameterConstraint(const QString &parName, int i,
                                                  const QString &constraint) {
  m_model.setLocalParameterConstraint(parName, i, constraint);
}

void FQFunctionModel::setLocalParameterFixed(const QString &parName, int i,
                                             bool fixed) {
  m_model.setLocalParameterFixed(parName, i, fixed);
}

void FQFunctionModel::setParameter(ParamID name, double value) {
  const auto prefix = getPrefix(name);
  if (prefix) {
    m_model.setParameter(*prefix + g_paramName.at(name), value);
  }
}

boost::optional<double> FQFunctionModel::getParameter(ParamID name) const {
  const auto paramName = getParameterName(name);
  return paramName ? m_model.getParameter(*paramName)
                   : boost::optional<double>();
}

boost::optional<double> FQFunctionModel::getParameterError(ParamID name) const {
  const auto paramName = getParameterName(name);
  return paramName ? m_model.getParameterError(*paramName)
                   : boost::optional<double>();
}

boost::optional<QString> FQFunctionModel::getParameterName(ParamID name) const {
  const auto prefix = getPrefix(name);
  return prefix ? *prefix + g_paramName.at(name) : boost::optional<QString>();
}

boost::optional<QString>
FQFunctionModel::getParameterDescription(ParamID name) const {
  const auto paramName = getParameterName(name);
  return paramName ? m_model.getParameterDescription(*paramName)
                   : boost::optional<QString>();
}

boost::optional<QString> FQFunctionModel::getPrefix(ParamID) const {
  return getFitTypePrefix();
}

QMap<FQFunctionModel::ParamID, double>
FQFunctionModel::getCurrentValues() const {
  QMap<ParamID, double> values;
  auto store = [&values, this](ParamID name) {
    values[name] = *getParameter(name);
  };
  applyParameterFunction(store);
  return values;
}

QMap<FQFunctionModel::ParamID, double>
FQFunctionModel::getCurrentErrors() const {
  QMap<ParamID, double> errors;
  auto store = [&errors, this](ParamID name) {
    errors[name] = *getParameterError(name);
  };
  applyParameterFunction(store);
  return errors;
}

QMap<int, QString> FQFunctionModel::getParameterNameMap() const {
  QMap<int, QString> out;
  auto addToMap = [&out, this](ParamID name) {
    out[static_cast<int>(name)] = *getParameterName(name);
  };
  applyParameterFunction(addToMap);
  return out;
}

QMap<int, std::string> FQFunctionModel::getParameterDescriptionMap() const {
  QMap<int, std::string> out;
  auto gaussian = FunctionFactory::Instance().createInitialized(
      buildGaussianFunctionString());
  out[static_cast<int>(ParamID::GAUSSIAN_HEIGHT)] =
      gaussian->parameterDescription(0);
  out[static_cast<int>(ParamID::GAUSSIAN_FQ)] =
      gaussian->parameterDescription(1);
  auto peters = FunctionFactory::Instance().createInitialized(
      buildPetersFunctionString());
  out[static_cast<int>(ParamID::PETERS_HEIGHT)] =
      peters->parameterDescription(0);
  out[static_cast<int>(ParamID::PETERS_FQ)] = peters->parameterDescription(1);
  out[static_cast<int>(ParamID::PETERS_BETA)] = peters->parameterDescription(2);
  auto yi =
      FunctionFactory::Instance().createInitialized(buildYiFunctionString());
  out[static_cast<int>(ParamID::YI_HEIGHT)] = yi->parameterDescription(0);
  out[static_cast<int>(ParamID::YI_FQ)] = yi->parameterDescription(1);
  out[static_cast<int>(ParamID::YI_SIGMA)] = yi->parameterDescription(2);
  return out;
}

void FQFunctionModel::setCurrentValues(const QMap<ParamID, double> &values) {
  for (const auto &name : values.keys()) {
    setParameter(name, values[name]);
  }
}

void FQFunctionModel::applyParameterFunction(
    std::function<void(ParamID)> paramFun) const {
  if (m_fitType == "MSDGauss") {
    paramFun(ParamID::GAUSSIAN_HEIGHT);
    paramFun(ParamID::GAUSSIAN_FQ);
  }
  if (m_fitType == "MSDPeters") {
    paramFun(ParamID::PETERS_HEIGHT);
    paramFun(ParamID::PETERS_FQ);
    paramFun(ParamID::PETERS_BETA);
  }
  if (m_fitType == "MSDYi") {
    paramFun(ParamID::YI_HEIGHT);
    paramFun(ParamID::YI_FQ);
    paramFun(ParamID::YI_SIGMA);
  }
}

boost::optional<FQFunctionModel::ParamID>
FQFunctionModel::getParameterId(const QString &parName) {
  boost::optional<ParamID> result;
  auto getter = [&result, parName, this](ParamID pid) {
    if (parName == *getParameterName(pid))
      result = pid;
  };
  applyParameterFunction(getter);
  return result;
}

std::string FQFunctionModel::buildGaussianFunctionString() const {
  return "name=MSDGauss,Height=1,Msd=0.05,constraints=(Height>0)";
}

std::string FQFunctionModel::buildPetersFunctionString() const {
  return "name=MSDPeters,Height=1,Msd=0.05,Beta=1,constraints=(Height>0)";
}

std::string FQFunctionModel::buildYiFunctionString() const {
  return "name=MSDYi,Height=1,Msd=0.05,Sigma=1,constraints=(Height>0)";
}

QString FQFunctionModel::buildFunctionString() const {
  QStringList functions;
  if (m_fitType == "MSDGauss") {
    functions << QString::fromStdString(buildGaussianFunctionString());
  }
  if (m_fitType == "MSDPeters") {
    functions << QString::fromStdString(buildPetersFunctionString());
  }
  if (m_fitType == "MSDYi") {
    functions << QString::fromStdString(buildYiFunctionString());
  }
  return functions.join(";");
}

boost::optional<QString> FQFunctionModel::getFitTypePrefix() const {
  if (m_fitType.isEmpty())
    return boost::optional<QString>();
  return QString();
}

QString FQFunctionModel::setBackgroundA0(double) { return ""; }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt