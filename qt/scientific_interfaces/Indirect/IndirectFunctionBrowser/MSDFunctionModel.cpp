// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MSDFunctionModel.h"
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
std::map<MSDFunctionModel::ParamID, QString> g_paramName{
    {MSDFunctionModel::ParamID::GAUSSIAN_HEIGHT, "Height"},
    {MSDFunctionModel::ParamID::GAUSSIAN_MSD, "Msd"},
    {MSDFunctionModel::ParamID::PETERS_HEIGHT, "Height"},
    {MSDFunctionModel::ParamID::PETERS_MSD, "Msd"},
    {MSDFunctionModel::ParamID::PETERS_BETA, "Beta"},
    {MSDFunctionModel::ParamID::YI_HEIGHT, "Height"},
    {MSDFunctionModel::ParamID::YI_MSD, "Msd"},
    {MSDFunctionModel::ParamID::YI_SIGMA, "Sigma"}};
}

MSDFunctionModel::MSDFunctionModel() {}

void MSDFunctionModel::clearData() {
  m_fitType.clear();
  m_model.clear();
}

void MSDFunctionModel::setFunction(IFunction_sptr fun) {
  clearData();
  if (!fun)
    return;
  if (fun->nFunctions() == 0) {
    auto const name = fun->name();
    if (name == "MsdGauss" || name == "MsdPeters" || name == "MsdYi") {
      m_fitType = QString::fromStdString(name);
    } else {
      throw std::runtime_error("Cannot set function " + name);
    }
    m_model.setFunction(fun);
    return;
  }
  bool isFitTypeSet = false;
  for (size_t i = 0; i < fun->nFunctions(); ++i) {
    auto f = fun->getFunction(i);
    auto const name = f->name();
    if (name == "MsdGauss" || name == "MsdPeters" || name == "MsdYi") {
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

IFunction_sptr MSDFunctionModel::getFitFunction() const {
  return m_model.getFitFunction();
}

bool MSDFunctionModel::hasFunction() const { return m_model.hasFunction(); }

void MSDFunctionModel::addFunction(const QString &prefix,
                                   const QString &funStr) {
  if (!prefix.isEmpty())
    throw std::runtime_error(
        "Function doesn't have member function with prefix " +
        prefix.toStdString());
  auto fun =
      FunctionFactory::Instance().createInitialized(funStr.toStdString());
  auto const name = fun->name();
  QString newPrefix;
  if (name == "MsdGauss" || name == "MsdPeters" || name == "MsdYi") {
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

void MSDFunctionModel::removeFunction(const QString &prefix) {
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

void MSDFunctionModel::setFitType(const QString &name) {
  auto oldValues = getCurrentValues();
  m_fitType = name;
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

void MSDFunctionModel::removeFitType() {
  auto oldValues = getCurrentValues();
  m_fitType.clear();
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

bool MSDFunctionModel::hasGaussianType() const {
  return m_fitType == "MsdGauss";
}

bool MSDFunctionModel::hasPetersType() const {
  return m_fitType == "MsdPeters";
}

bool MSDFunctionModel::hasYiType() const { return m_fitType == "MsdYi"; }

void MSDFunctionModel::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_estimationData = std::move(data);
}

void MSDFunctionModel::setNumberDomains(int n) { m_model.setNumberDomains(n); }

int MSDFunctionModel::getNumberDomains() const {
  return m_model.getNumberDomains();
}

void MSDFunctionModel::setParameter(const QString &paramName, double value) {
  m_model.setParameterError(paramName, value);
}

void MSDFunctionModel::setParameterError(const QString &paramName,
                                         double value) {
  m_model.setParameterError(paramName, value);
}

double MSDFunctionModel::getParameter(const QString &paramName) const {
  return m_model.getParameter(paramName);
}

double MSDFunctionModel::getParameterError(const QString &paramName) const {
  return m_model.getParameterError(paramName);
}

QString
MSDFunctionModel::getParameterDescription(const QString &paramName) const {
  return m_model.getParameterDescription(paramName);
}

QStringList MSDFunctionModel::getParameterNames() const {
  return m_model.getParameterNames();
}

IFunction_sptr MSDFunctionModel::getSingleFunction(int index) const {
  return m_model.getSingleFunction(index);
}

IFunction_sptr MSDFunctionModel::getCurrentFunction() const {
  return m_model.getCurrentFunction();
}

QStringList MSDFunctionModel::getGlobalParameters() const {
  return m_model.getGlobalParameters();
}

QStringList MSDFunctionModel::getLocalParameters() const {
  return m_model.getLocalParameters();
}

void MSDFunctionModel::setGlobalParameters(const QStringList &globals) {
  m_globals.clear();
  for (auto const &name : globals) {
    addGlobal(name);
  }
  auto newGlobals = makeGlobalList();
  m_model.setGlobalParameters(newGlobals);
}

bool MSDFunctionModel::isGlobal(const QString &parName) const {
  return m_model.isGlobal(parName);
}

void MSDFunctionModel::setGlobal(const QString &parName, bool on) {
  if (parName.isEmpty())
    return;
  if (on)
    addGlobal(parName);
  else
    removeGlobal(parName);
  auto globals = makeGlobalList();
  m_model.setGlobalParameters(globals);
}

void MSDFunctionModel::addGlobal(const QString &parName) {
  auto const pid = getParameterId(parName);
  if (pid && !m_globals.contains(*pid)) {
    m_globals.push_back(*pid);
  }
}

void MSDFunctionModel::removeGlobal(const QString &parName) {
  auto const pid = getParameterId(parName);
  if (pid && m_globals.contains(*pid)) {
    m_globals.removeOne(*pid);
  }
}

QStringList MSDFunctionModel::makeGlobalList() const {
  QStringList globals;
  for (auto const id : m_globals) {
    auto const name = getParameterName(id);
    if (name)
      globals << *name;
  }
  return globals;
}

void MSDFunctionModel::updateMultiDatasetParameters(const IFunction &fun) {
  m_model.updateMultiDatasetParameters(fun);
}

void MSDFunctionModel::updateMultiDatasetParameters(
    const ITableWorkspace &paramTable) {
  auto const nRows = paramTable.rowCount();
  if (nRows == 0)
    return;

  auto const globalParameterNames = getGlobalParameters();
  for (auto &&name : globalParameterNames) {
    auto valueColumn = paramTable.getColumn(name.toStdString());
    auto errorColumn = paramTable.getColumn((name + "_Err").toStdString());
    m_model.setParameter(name, valueColumn->toDouble(0));
    m_model.setParameterError(name, errorColumn->toDouble(0));
  }

  auto const localParameterNames = getLocalParameters();
  for (auto &&name : localParameterNames) {
    auto valueColumn = paramTable.getColumn(name.toStdString());
    auto errorColumn = paramTable.getColumn((name + "_Err").toStdString());
    if (nRows > 1) {
      for (size_t i = 0; i < nRows; ++i) {
        m_model.setLocalParameterValue(name, static_cast<int>(i),
                                       valueColumn->toDouble(i),
                                       errorColumn->toDouble(i));
      }
    } else {
      auto const i = m_model.currentDomainIndex();
      m_model.setLocalParameterValue(name, static_cast<int>(i),
                                     valueColumn->toDouble(0),
                                     errorColumn->toDouble(0));
    }
  }
}

void MSDFunctionModel::updateParameters(const IFunction &fun) {
  m_model.updateParameters(fun);
}

void MSDFunctionModel::setCurrentDomainIndex(int i) {
  m_model.setCurrentDomainIndex(i);
}

int MSDFunctionModel::currentDomainIndex() const {
  return m_model.currentDomainIndex();
}

void MSDFunctionModel::changeTie(const QString &paramName, const QString &tie) {
  m_model.changeTie(paramName, tie);
}

void MSDFunctionModel::addConstraint(const QString &functionIndex,
                                     const QString &constraint) {
  m_model.addConstraint(functionIndex, constraint);
}

void MSDFunctionModel::removeConstraint(const QString &paramName) {
  m_model.removeConstraint(paramName);
}

void MSDFunctionModel::setDatasetNames(const QStringList &names) {
  m_model.setDatasetNames(names);
}

QStringList MSDFunctionModel::getDatasetNames() const {
  return m_model.getDatasetNames();
}

double MSDFunctionModel::getLocalParameterValue(const QString &parName,
                                                int i) const {
  return m_model.getLocalParameterValue(parName, i);
}

bool MSDFunctionModel::isLocalParameterFixed(const QString &parName,
                                             int i) const {
  return m_model.isLocalParameterFixed(parName, i);
}

QString MSDFunctionModel::getLocalParameterTie(const QString &parName,
                                               int i) const {
  return m_model.getLocalParameterTie(parName, i);
}

QString MSDFunctionModel::getLocalParameterConstraint(const QString &parName,
                                                      int i) const {
  return m_model.getLocalParameterConstraint(parName, i);
}

void MSDFunctionModel::setLocalParameterValue(const QString &parName, int i,
                                              double value) {
  m_model.setLocalParameterValue(parName, i, value);
}

void MSDFunctionModel::setLocalParameterValue(const QString &parName, int i,
                                              double value, double error) {
  m_model.setLocalParameterValue(parName, i, value, error);
}

void MSDFunctionModel::setLocalParameterTie(const QString &parName, int i,
                                            const QString &tie) {
  m_model.setLocalParameterTie(parName, i, tie);
}

void MSDFunctionModel::setLocalParameterConstraint(const QString &parName,
                                                   int i,
                                                   const QString &constraint) {
  m_model.setLocalParameterConstraint(parName, i, constraint);
}

void MSDFunctionModel::setLocalParameterFixed(const QString &parName, int i,
                                              bool fixed) {
  m_model.setLocalParameterFixed(parName, i, fixed);
}

void MSDFunctionModel::setParameter(ParamID name, double value) {
  auto const prefix = getPrefix(name);
  if (prefix) {
    m_model.setParameter(*prefix + g_paramName.at(name), value);
  }
}

boost::optional<double> MSDFunctionModel::getParameter(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model.getParameter(*paramName)
                   : boost::optional<double>();
}

boost::optional<double>
MSDFunctionModel::getParameterError(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model.getParameterError(*paramName)
                   : boost::optional<double>();
}

boost::optional<QString>
MSDFunctionModel::getParameterName(ParamID name) const {
  auto const prefix = getPrefix(name);
  return prefix ? *prefix + g_paramName.at(name) : boost::optional<QString>();
}

boost::optional<QString>
MSDFunctionModel::getParameterDescription(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model.getParameterDescription(*paramName)
                   : boost::optional<QString>();
}

boost::optional<QString> MSDFunctionModel::getPrefix(ParamID name) const {
  return getFitTypePrefix();
}

QMap<MSDFunctionModel::ParamID, double>
MSDFunctionModel::getCurrentValues() const {
  QMap<ParamID, double> values;
  auto store = [&values, this](ParamID name) {
    values[name] = *getParameter(name);
  };
  applyParameterFunction(store);
  return values;
}

QMap<MSDFunctionModel::ParamID, double>
MSDFunctionModel::getCurrentErrors() const {
  QMap<ParamID, double> errors;
  auto store = [&errors, this](ParamID name) {
    errors[name] = *getParameterError(name);
  };
  applyParameterFunction(store);
  return errors;
}

QMap<int, QString> MSDFunctionModel::getParameterNameMap() const {
  QMap<int, QString> out;
  auto addToMap = [&out, this](ParamID name) {
    out[static_cast<int>(name)] = *getParameterName(name);
  };
  applyParameterFunction(addToMap);
  return out;
}

QMap<int, std::string> MSDFunctionModel::getParameterDescriptionMap() const {
  QMap<int, std::string> out;
  auto gaussian = FunctionFactory::Instance().createInitialized(
      buildGaussianFunctionString());
  out[static_cast<int>(ParamID::GAUSSIAN_HEIGHT)] =
      gaussian->parameterDescription(0);
  out[static_cast<int>(ParamID::GAUSSIAN_MSD)] =
      gaussian->parameterDescription(1);
  auto peters = FunctionFactory::Instance().createInitialized(
      buildPetersFunctionString());
  out[static_cast<int>(ParamID::PETERS_HEIGHT)] =
      peters->parameterDescription(0);
  out[static_cast<int>(ParamID::PETERS_MSD)] = peters->parameterDescription(1);
  out[static_cast<int>(ParamID::PETERS_BETA)] = peters->parameterDescription(2);
  auto yi =
      FunctionFactory::Instance().createInitialized(buildYiFunctionString());
  out[static_cast<int>(ParamID::YI_HEIGHT)] = yi->parameterDescription(0);
  out[static_cast<int>(ParamID::YI_MSD)] = yi->parameterDescription(1);
  out[static_cast<int>(ParamID::YI_SIGMA)] = yi->parameterDescription(2);
  return out;
}

void MSDFunctionModel::setCurrentValues(const QMap<ParamID, double> &values) {
  for (auto const name : values.keys()) {
    setParameter(name, values[name]);
  }
}

void MSDFunctionModel::applyParameterFunction(
    std::function<void(ParamID)> paramFun) const {
  if (m_fitType == "MsdGauss") {
    paramFun(ParamID::GAUSSIAN_HEIGHT);
    paramFun(ParamID::GAUSSIAN_MSD);
  }
  if (m_fitType == "MsdPeters") {
    paramFun(ParamID::PETERS_HEIGHT);
    paramFun(ParamID::PETERS_MSD);
    paramFun(ParamID::PETERS_BETA);
  }
  if (m_fitType == "MsdYi") {
    paramFun(ParamID::YI_HEIGHT);
    paramFun(ParamID::YI_MSD);
    paramFun(ParamID::YI_SIGMA);
  }
}

boost::optional<MSDFunctionModel::ParamID>
MSDFunctionModel::getParameterId(const QString &parName) {
  boost::optional<ParamID> result;
  auto getter = [&result, parName, this](ParamID pid) {
    if (parName == *getParameterName(pid))
      result = pid;
  };
  applyParameterFunction(getter);
  return result;
}

std::string MSDFunctionModel::buildGaussianFunctionString() const {
  return "name=MsdGauss,Height=1,Msd=0.05,constraints=(Height>0)";
}

std::string MSDFunctionModel::buildPetersFunctionString() const {
  return "name=MsdPeters,Height=1,Msd=0.05,Beta=1,constraints=(Height>0)";
}

std::string MSDFunctionModel::buildYiFunctionString() const {
  return "name=MsdYi,Height=1,Msd=0.05,Sigma=1,constraints=(Height>0)";
}

QString MSDFunctionModel::buildFunctionString() const {
  QStringList functions;
  if (m_fitType == "MsdGauss") {
    functions << QString::fromStdString(buildGaussianFunctionString());
  }
  if (m_fitType == "MsdPeters") {
    functions << QString::fromStdString(buildPetersFunctionString());
  }
  if (m_fitType == "MsdYi") {
    functions << QString::fromStdString(buildYiFunctionString());
  }
  return functions.join(";");
}

boost::optional<QString> MSDFunctionModel::getFitTypePrefix() const {
  if (m_fitType.isEmpty())
    return boost::optional<QString>();
  return "";
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
