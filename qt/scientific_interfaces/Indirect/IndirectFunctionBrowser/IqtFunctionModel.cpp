// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IqtFunctionModel.h"
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
std::map<IqtFunctionModel::ParamID, QString> g_paramName{
    {IqtFunctionModel::ParamID::EXP1_HEIGHT, "Height"},
    {IqtFunctionModel::ParamID::EXP1_LIFETIME, "Lifetime"},
    {IqtFunctionModel::ParamID::EXP2_HEIGHT, "Height"},
    {IqtFunctionModel::ParamID::EXP2_LIFETIME, "Lifetime"},
    {IqtFunctionModel::ParamID::STRETCH_HEIGHT, "Height"},
    {IqtFunctionModel::ParamID::STRETCH_LIFETIME, "Lifetime"},
    {IqtFunctionModel::ParamID::STRETCH_STRETCHING, "Stretching"},
    {IqtFunctionModel::ParamID::BG_A0, "A0"}};
}

IqtFunctionModel::IqtFunctionModel() {}

void IqtFunctionModel::clearData() {
  m_numberOfExponentials = 0;
  m_hasStretchExponential = false;
  m_background.clear();
  m_model.clear();
}

void IqtFunctionModel::setFunction(IFunction_sptr fun) {
  clearData();
  if (!fun)
    return;
  if (fun->nFunctions() == 0) {
    auto const name = fun->name();
    if (name == "ExpDecay") {
      m_numberOfExponentials = 1;
    } else if (name == "StretchExp") {
      m_hasStretchExponential = true;
    } else if (name == "FlatBackground") {
      m_background = QString::fromStdString(name);
    } else {
      throw std::runtime_error("Cannot set function " + name);
    }
    m_model.setFunction(fun);
    return;
  }
  bool areExponentialsSet = false;
  bool isStretchSet = false;
  bool isBackgroundSet = false;
  for (size_t i = 0; i < fun->nFunctions(); ++i) {
    auto f = fun->getFunction(i);
    auto const name = f->name();
    if (name == "ExpDecay") {
      if (areExponentialsSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      if (m_numberOfExponentials == 0) {
        m_numberOfExponentials = 1;
      } else {
        m_numberOfExponentials = 2;
        areExponentialsSet = true;
      }
    } else if (name == "StretchExp") {
      if (isStretchSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      m_hasStretchExponential = true;
      areExponentialsSet = true;
      isStretchSet = true;
    } else if (name == "FlatBackground") {
      if (isBackgroundSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      m_background = QString::fromStdString(name);
      areExponentialsSet = true;
      isStretchSet = true;
      isBackgroundSet = true;
    } else {
      clear();
      throw std::runtime_error("Function has wrong structure.");
    }
  }
  m_model.setFunction(fun);
}

IFunction_sptr IqtFunctionModel::getFitFunction() const {
  return m_model.getFitFunction();
}

bool IqtFunctionModel::hasFunction() const { return m_model.hasFunction(); }

void IqtFunctionModel::addFunction(const QString &prefix,
                                   const QString &funStr) {
  if (!prefix.isEmpty())
    throw std::runtime_error(
        "Function doesn't have member function with prefix " +
        prefix.toStdString());
  auto fun =
      FunctionFactory::Instance().createInitialized(funStr.toStdString());
  auto const name = fun->name();
  QString newPrefix;
  if (name == "ExpDecay") {
    auto const ne = getNumberOfExponentials();
    if (ne > 1)
      throw std::runtime_error("Cannot add more exponentials.");
    setNumberOfExponentials(ne + 1);
    if (auto const prefix = getExp2Prefix()) {
      newPrefix = *prefix;
    } else {
      newPrefix = *getExp1Prefix();
    }
  } else if (name == "StretchExp") {
    if (hasStretchExponential())
      throw std::runtime_error("Cannot add more stretched exponentials.");
    setStretchExponential(true);
    newPrefix = *getStretchPrefix();
  } else if (name == "FlatBackground") {
    if (hasBackground())
      throw std::runtime_error("Cannot add more backgrounds.");
    setBackground(QString::fromStdString(name));
    newPrefix = *getBackgroundPrefix();
  } else {
    throw std::runtime_error("Cannot add funtion " + name);
  }
  auto newFun = getFunctionWithPrefix(newPrefix, getSingleFunction(0));
  copyParametersAndErrors(*fun, *newFun);
  if (getNumberLocalFunctions() > 1) {
    copyParametersAndErrorsToAllLocalFunctions(*getSingleFunction(0));
  }
}

void IqtFunctionModel::removeFunction(const QString &prefix) {
  if (prefix.isEmpty()) {
    clear();
    return;
  }
  auto prefix1 = getExp1Prefix();
  if (prefix1 && *prefix1 == prefix) {
    setNumberOfExponentials(0);
    return;
  }
  prefix1 = getExp2Prefix();
  if (prefix1 && *prefix1 == prefix) {
    setNumberOfExponentials(1);
    return;
  }
  prefix1 = getStretchPrefix();
  if (prefix1 && *prefix1 == prefix) {
    setStretchExponential(false);
    return;
  }
  prefix1 = getBackgroundPrefix();
  if (prefix1 && *prefix1 == prefix) {
    removeBackground();
    return;
  }
  throw std::runtime_error(
      "Function doesn't have member function with prefix " +
      prefix.toStdString());
}

void IqtFunctionModel::setNumberOfExponentials(int n) {
  auto oldValues = getCurrentValues();
  m_numberOfExponentials = n;
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

int IqtFunctionModel::getNumberOfExponentials() const {
  return m_numberOfExponentials;
}

void IqtFunctionModel::setStretchExponential(bool on) {
  auto oldValues = getCurrentValues();
  m_hasStretchExponential = on;
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
  if (on) {
    estimateStretchExpParameters();
  }
}

bool IqtFunctionModel::hasStretchExponential() const {
  return m_hasStretchExponential;
}

void IqtFunctionModel::setBackground(const QString &name) {
  auto oldValues = getCurrentValues();
  m_background = name;
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

void IqtFunctionModel::removeBackground() {
  auto oldValues = getCurrentValues();
  m_background.clear();
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

bool IqtFunctionModel::hasBackground() const { return !m_background.isEmpty(); }

void IqtFunctionModel::tieIntensities(bool on) {
  auto heightName = getParameterName(ParamID::STRETCH_HEIGHT);
  if (!heightName)
    heightName = getParameterName(ParamID::EXP1_HEIGHT);
  auto const a0Name = getParameterName(ParamID::BG_A0);
  if (!heightName || !a0Name)
    return;
  auto const tie = on? QString("1-%1").arg(*a0Name) : QString();
  for (auto i = 0; i < getNumberDomains(); ++i) {
    setLocalParameterTie(*heightName, i, tie);
  }
}

void IqtFunctionModel::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_estimationData = std::move(data);
}

QString IqtFunctionModel::setBackgroundA0(double value) {
  if (hasBackground()) {
    setParameter(ParamID::BG_A0, value);
    return *getParameterName(ParamID::BG_A0);
  }
  return "";
}

void IqtFunctionModel::setNumberDomains(int n) { m_model.setNumberDomains(n); }

int IqtFunctionModel::getNumberDomains() const {
  return m_model.getNumberDomains();
}

void IqtFunctionModel::setParameter(const QString &paramName, double value) {
  m_model.setParameter(paramName, value);
}

void IqtFunctionModel::setParameterError(const QString &paramName,
                                         double value) {
  m_model.setParameterError(paramName, value);
}

double IqtFunctionModel::getParameter(const QString &paramName) const {
  return m_model.getParameter(paramName);
}

double IqtFunctionModel::getParameterError(const QString &paramName) const {
  return m_model.getParameterError(paramName);
}

QString
IqtFunctionModel::getParameterDescription(const QString &paramName) const {
  return m_model.getParameterDescription(paramName);
}

QStringList IqtFunctionModel::getParameterNames() const {
  return m_model.getParameterNames();
}

IFunction_sptr IqtFunctionModel::getSingleFunction(int index) const {
  return m_model.getSingleFunction(index);
}

IFunction_sptr IqtFunctionModel::getCurrentFunction() const {
  return m_model.getCurrentFunction();
}

QStringList IqtFunctionModel::getGlobalParameters() const {
  return m_model.getGlobalParameters();
}

QStringList IqtFunctionModel::getLocalParameters() const {
  return m_model.getLocalParameters();
}

void IqtFunctionModel::setGlobalParameters(const QStringList &globals) {
  m_globals.clear();
  for (auto const &name : globals) {
    addGlobal(name);
  }
  auto newGlobals = makeGlobalList();
  m_model.setGlobalParameters(newGlobals);
}

bool IqtFunctionModel::isGlobal(const QString &parName) const {
  return m_model.isGlobal(parName);
}

void IqtFunctionModel::setGlobal(const QString &parName, bool on) {
  if (parName.isEmpty())
    return;
  if (on)
    addGlobal(parName);
  else
    removeGlobal(parName);
  auto globals = makeGlobalList();
  m_model.setGlobalParameters(globals);
}

void IqtFunctionModel::addGlobal(const QString &parName) {
  auto const pid = getParameterId(parName);
  if (pid && !m_globals.contains(*pid)) {
    m_globals.push_back(*pid);
  }
}

void IqtFunctionModel::removeGlobal(const QString &parName) {
  auto const pid = getParameterId(parName);
  if (pid && m_globals.contains(*pid)) {
    m_globals.removeOne(*pid);
  }
}

QStringList IqtFunctionModel::makeGlobalList() const {
  QStringList globals;
  for (auto const id : m_globals) {
    auto const name = getParameterName(id);
    if (name)
      globals << *name;
  }
  return globals;
}

void IqtFunctionModel::updateMultiDatasetParameters(const IFunction &fun) {
  m_model.updateMultiDatasetParameters(fun);
}

void IqtFunctionModel::updateMultiDatasetParameters(
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

void IqtFunctionModel::updateParameters(const IFunction &fun) {
  m_model.updateParameters(fun);
}

void IqtFunctionModel::setCurrentDomainIndex(int i) {
  m_model.setCurrentDomainIndex(i);
}

int IqtFunctionModel::currentDomainIndex() const {
  return m_model.currentDomainIndex();
}

void IqtFunctionModel::changeTie(const QString &paramName, const QString &tie) {
  m_model.changeTie(paramName, tie);
}

void IqtFunctionModel::addConstraint(const QString &functionIndex,
                                     const QString &constraint) {
  m_model.addConstraint(functionIndex, constraint);
}

void IqtFunctionModel::removeConstraint(const QString &paramName) {
  m_model.removeConstraint(paramName);
}

void IqtFunctionModel::setDatasetNames(const QStringList &names) {
  m_model.setDatasetNames(names);
}

QStringList IqtFunctionModel::getDatasetNames() const {
  return m_model.getDatasetNames();
}

double IqtFunctionModel::getLocalParameterValue(const QString &parName,
                                                int i) const {
  return m_model.getLocalParameterValue(parName, i);
}

bool IqtFunctionModel::isLocalParameterFixed(const QString &parName,
                                             int i) const {
  return m_model.isLocalParameterFixed(parName, i);
}

QString IqtFunctionModel::getLocalParameterTie(const QString &parName,
                                               int i) const {
  return m_model.getLocalParameterTie(parName, i);
}

QString IqtFunctionModel::getLocalParameterConstraint(const QString &parName,
                                                      int i) const {
  return m_model.getLocalParameterConstraint(parName, i);
}

void IqtFunctionModel::setLocalParameterValue(const QString &parName, int i,
                                              double value) {
  m_model.setLocalParameterValue(parName, i, value);
}

void IqtFunctionModel::setLocalParameterValue(const QString &parName, int i,
                                              double value, double error) {
  m_model.setLocalParameterValue(parName, i, value, error);
}

void IqtFunctionModel::setLocalParameterTie(const QString &parName, int i,
                                            const QString &tie) {
  m_model.setLocalParameterTie(parName, i, tie);
}

void IqtFunctionModel::setLocalParameterConstraint(const QString &parName,
                                                   int i,
                                                   const QString &constraint) {
  m_model.setLocalParameterConstraint(parName, i, constraint);
}

void IqtFunctionModel::setLocalParameterFixed(const QString &parName, int i,
                                              bool fixed) {
  m_model.setLocalParameterFixed(parName, i, fixed);
}

void IqtFunctionModel::setParameter(ParamID name, double value) {
  auto const prefix = getPrefix(name);
  if (prefix) {
    m_model.setParameter(*prefix + g_paramName.at(name), value);
  }
}

boost::optional<double> IqtFunctionModel::getParameter(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model.getParameter(*paramName)
                   : boost::optional<double>();
}

boost::optional<double>
IqtFunctionModel::getParameterError(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model.getParameterError(*paramName)
                   : boost::optional<double>();
}

boost::optional<QString>
IqtFunctionModel::getParameterName(ParamID name) const {
  auto const prefix = getPrefix(name);
  return prefix ? *prefix + g_paramName.at(name) : boost::optional<QString>();
}

boost::optional<QString>
IqtFunctionModel::getParameterDescription(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model.getParameterDescription(*paramName)
                   : boost::optional<QString>();
}

boost::optional<QString> IqtFunctionModel::getPrefix(ParamID name) const {
  if (name <= ParamID::EXP1_LIFETIME) {
    return getExp1Prefix();
  } else if (name <= ParamID::EXP2_LIFETIME) {
    return getExp2Prefix();
  } else if (name <= ParamID::STRETCH_STRETCHING) {
    return getStretchPrefix();
  } else {
    return getBackgroundPrefix();
  }
}

QMap<IqtFunctionModel::ParamID, double>
IqtFunctionModel::getCurrentValues() const {
  QMap<ParamID, double> values;
  auto store = [&values, this](ParamID name) {
    values[name] = *getParameter(name);
  };
  applyParameterFunction(store);
  return values;
}

QMap<IqtFunctionModel::ParamID, double>
IqtFunctionModel::getCurrentErrors() const {
  QMap<ParamID, double> errors;
  auto store = [&errors, this](ParamID name) {
    errors[name] = *getParameterError(name);
  };
  applyParameterFunction(store);
  return errors;
}

QMap<int, QString> IqtFunctionModel::getParameterNameMap() const {
  QMap<int, QString> out;
  auto addToMap = [&out, this](ParamID name) {
    out[static_cast<int>(name)] = *getParameterName(name);
  };
  applyParameterFunction(addToMap);
  return out;
}

QMap<int, std::string> IqtFunctionModel::getParameterDescriptionMap() const {
  QMap<int, std::string> out;
  auto expDecay = FunctionFactory::Instance().createInitialized(
      buildExpDecayFunctionString());
  out[static_cast<int>(ParamID::EXP1_HEIGHT)] =
      expDecay->parameterDescription(0);
  out[static_cast<int>(ParamID::EXP1_LIFETIME)] =
      expDecay->parameterDescription(1);
  out[static_cast<int>(ParamID::EXP2_HEIGHT)] =
      expDecay->parameterDescription(0);
  out[static_cast<int>(ParamID::EXP2_LIFETIME)] =
      expDecay->parameterDescription(1);
  auto stretchExp = FunctionFactory::Instance().createInitialized(
      buildStretchExpFunctionString());
  out[static_cast<int>(ParamID::STRETCH_HEIGHT)] =
      stretchExp->parameterDescription(0);
  out[static_cast<int>(ParamID::STRETCH_LIFETIME)] =
      stretchExp->parameterDescription(1);
  out[static_cast<int>(ParamID::STRETCH_STRETCHING)] =
      stretchExp->parameterDescription(2);
  auto background = FunctionFactory::Instance().createInitialized(
      buildBackgroundFunctionString());
  out[static_cast<int>(ParamID::BG_A0)] =
      background->parameterDescription(0);
  return out;
}

void IqtFunctionModel::setCurrentValues(
    const QMap<ParamID, double> &values) {
  for (auto const name : values.keys()) {
    setParameter(name, values[name]);
  }
}

void IqtFunctionModel::applyParameterFunction(
    std::function<void(ParamID)> paramFun) const {
  if (m_numberOfExponentials > 0) {
    paramFun(ParamID::EXP1_HEIGHT);
    paramFun(ParamID::EXP1_LIFETIME);
  }
  if (m_numberOfExponentials > 1) {
    paramFun(ParamID::EXP2_HEIGHT);
    paramFun(ParamID::EXP2_LIFETIME);
  }
  if (m_hasStretchExponential) {
    paramFun(ParamID::STRETCH_HEIGHT);
    paramFun(ParamID::STRETCH_LIFETIME);
    paramFun(ParamID::STRETCH_STRETCHING);
  }
  if (!m_background.isEmpty()) {
    paramFun(ParamID::BG_A0);
  }
}

boost::optional<IqtFunctionModel::ParamID>
IqtFunctionModel::getParameterId(const QString &parName) {
  boost::optional<ParamID> result;
  auto getter = [&result, parName, this](ParamID pid) {
    if (parName == *getParameterName(pid))
      result = pid;
  };
  applyParameterFunction(getter);
  return result;
}

std::string IqtFunctionModel::buildExpDecayFunctionString() const {
  return "name=ExpDecay,Height=1,Lifetime=1,constraints=(Height>0,Lifetime>0)";
}

std::string IqtFunctionModel::buildStretchExpFunctionString() const {
  return "name=StretchExp,Height=1,Lifetime=1,Stretching=1,constraints=(Height>"
         "0,Lifetime>0,0<Stretching<1.001)";
}

std::string IqtFunctionModel::buildBackgroundFunctionString() const {
  return "name=FlatBackground,A0=0,constraints=(A0>0)";
}

void IqtFunctionModel::estimateStretchExpParameters() {
  auto const heightName = getParameterName(ParamID::STRETCH_HEIGHT);
  auto const lifeTimeName = getParameterName(ParamID::STRETCH_LIFETIME);
  auto const stretchingName = getParameterName(ParamID::STRETCH_STRETCHING);
  if (!heightName || !lifeTimeName || !stretchingName)
    return;
  assert(getNumberDomains() == m_estimationData.size());
  for (auto i = 0; i < getNumberDomains(); ++i) {
    auto const &x = m_estimationData[i].x;
    auto const &y = m_estimationData[i].y;
    auto lifeTime = (x[1] - x[0]) / (log(y[0]) - log(y[1]));
    if (lifeTime <= 0)
      lifeTime = 1.0;
    auto const height = y[0] * exp(x[0] / lifeTime);
    setLocalParameterValue(*heightName, i, height);
    setLocalParameterValue(*lifeTimeName, i, lifeTime);
    setLocalParameterValue(*stretchingName, i, 1.0);
  }
}

QString IqtFunctionModel::buildFunctionString() const {
  QStringList functions;
  if (m_numberOfExponentials > 0) {
    functions << QString::fromStdString(buildExpDecayFunctionString());
  }
  if (m_numberOfExponentials > 1) {
    functions << QString::fromStdString(buildExpDecayFunctionString());
  }
  if (m_hasStretchExponential) {
    functions << QString::fromStdString(buildStretchExpFunctionString());
  }
  if (!m_background.isEmpty()) {
    functions << QString::fromStdString(buildBackgroundFunctionString());
  }
  return functions.join(";");
}

boost::optional<QString> IqtFunctionModel::getExp1Prefix() const {
  if (m_numberOfExponentials == 0)
    return boost::optional<QString>();
  if (m_numberOfExponentials == 1 && !m_hasStretchExponential &&
      m_background.isEmpty())
    return "";
  return "f0.";
}

boost::optional<QString> IqtFunctionModel::getExp2Prefix() const {
  if (m_numberOfExponentials < 2)
    return boost::optional<QString>();
  return "f1.";
}

boost::optional<QString> IqtFunctionModel::getStretchPrefix() const {
  if (!m_hasStretchExponential)
    return boost::optional<QString>();
  if (m_numberOfExponentials == 0 && m_background.isEmpty())
    return "";
  return QString("f%1.").arg(m_numberOfExponentials);
}

boost::optional<QString> IqtFunctionModel::getBackgroundPrefix() const {
  if (m_background.isEmpty())
    return boost::optional<QString>();
  if (m_numberOfExponentials == 0 && !m_hasStretchExponential)
    return "";
  return QString("f%1.").arg(m_numberOfExponentials +
                             (m_hasStretchExponential ? 1 : 0));
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
