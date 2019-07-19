// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvFunctionModel.h"
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
}

ConvFunctionModel::ConvFunctionModel() {}

void ConvFunctionModel::clearData() {
  m_fitType = FitType::None;
  m_hasDeltaFunction = false;
  m_background.clear();
  m_model.clear();
}

void ConvFunctionModel::setFunction(IFunction_sptr fun) {
  clearData();
  if (!fun)
    return;
  if (fun->nFunctions() == 0) {
    auto const name = fun->name();
    if (name == "Lorentzian") {
      m_fitType = FitType::OneLorentzian;
    } else if (name == "DeltaFunction") {
      m_hasDeltaFunction = true;
    } else if (name == "FlatBackground" || name == "LinearBackground") {
      m_background = QString::fromStdString(name);
    } else {
      throw std::runtime_error("Cannot set function " + name);
    }
    m_model.setFunction(fun);
    return;
  }
  bool isFitTypeSet = false;
  int numberLorentzians = 0;
  bool isBackgroundSet = false;
  for (size_t i = 0; i < fun->nFunctions(); ++i) {
    auto f = fun->getFunction(i);
    auto const name = f->name();
    if (name == "Lorentzian") {
      if (isFitTypeSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      if (numberLorentzians == 0) {
        numberLorentzians = 1;
      } else {
        numberLorentzians = 2;
        isFitTypeSet = true;
      }
    } else if (name == "DeltaFunction") {
      if (m_hasDeltaFunction || isBackgroundSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      m_hasDeltaFunction = true;
      isFitTypeSet = true;
    } else if (name == "FlatBackground" || name == "LinearBackground") {
      if (isBackgroundSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      m_background = QString::fromStdString(name);
      isFitTypeSet = true;
      isBackgroundSet = true;
    } else {
      clear();
      throw std::runtime_error("Function has wrong structure.");
    }
  }
  m_model.setFunction(fun);
}

IFunction_sptr ConvFunctionModel::getFitFunction() const {
  // Add the resolution here
  return m_model.getFitFunction();
}

bool ConvFunctionModel::hasFunction() const { return m_model.hasFunction(); }

void ConvFunctionModel::addFunction(const QString &prefix,
                                    const QString &funStr) {
  if (!prefix.isEmpty())
    throw std::runtime_error(
        "Function doesn't have member function with prefix " +
        prefix.toStdString());
  auto fun =
      FunctionFactory::Instance().createInitialized(funStr.toStdString());
  auto const name = fun->name();
  QString newPrefix;
  if (name == "Lorentzian") {
    if (m_fitType == FitType::TwoLorentzians) {
      throw std::runtime_error("Cannot add more Lorentzians.");
    } else if (m_fitType == FitType::OneLorentzian) {
      m_fitType = FitType::TwoLorentzians;
      newPrefix = *getLor2Prefix();
    } else if (m_fitType == FitType::None) {
      m_fitType = FitType::OneLorentzian;
      newPrefix = *getLor1Prefix();
    } else {
      throw std::runtime_error("Cannot add more Lorentzians.");
    }
  } else if (name == "DeltaFunction") {
    if (m_hasDeltaFunction)
      throw std::runtime_error("Cannot add a DeltaFunction.");
    setDeltaFunction(true);
    newPrefix = *getDeltaPrefix();
  } else if (name == "FlatBackground" || "LinearBackground") {
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

void ConvFunctionModel::removeFunction(const QString &prefix) {
  if (prefix.isEmpty()) {
    clear();
    return;
  }
  auto prefix1 = getLor1Prefix();
  if (prefix1 && *prefix1 == prefix) {
    setFitType(FitType::None);
    return;
  }
  prefix1 = getLor2Prefix();
  if (prefix1 && *prefix1 == prefix) {
    setFitType(FitType::OneLorentzian);
    return;
  }
  prefix1 = getDeltaPrefix();
  if (prefix1 && *prefix1 == prefix) {
    setDeltaFunction(false);
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

void ConvFunctionModel::setFitType(const QString &fitType) {
  setFitType(fitTypeId(fitType));
}

void ConvFunctionModel::setDeltaFunction(bool on) {
  auto oldValues = getCurrentValues();
  m_hasDeltaFunction = on;
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

bool ConvFunctionModel::hasDeltaFunction() const {
  return m_hasDeltaFunction;
}

void ConvFunctionModel::setBackground(const QString &name) {
  auto oldValues = getCurrentValues();
  m_background = name;
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

void ConvFunctionModel::removeBackground() {
  auto oldValues = getCurrentValues();
  m_background.clear();
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

bool ConvFunctionModel::hasBackground() const {
  return !m_background.isEmpty();
}

void ConvFunctionModel::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_estimationData = std::move(data);
}

QString ConvFunctionModel::setBackgroundA0(double value) {
  if (hasBackground()) {
    setParameter(ParamID::BG_A0, value);
    return *getParameterName(ParamID::BG_A0);
  }
  return "";
}

void ConvFunctionModel::setNumberDomains(int n) { m_model.setNumberDomains(n); }

int ConvFunctionModel::getNumberDomains() const {
  return m_model.getNumberDomains();
}

void ConvFunctionModel::setParameter(const QString &paramName, double value) {
  m_model.setParameter(paramName, value);
}

void ConvFunctionModel::setParameterError(const QString &paramName,
                                          double value) {
  m_model.setParameterError(paramName, value);
}

double ConvFunctionModel::getParameter(const QString &paramName) const {
  return m_model.getParameter(paramName);
}

double ConvFunctionModel::getParameterError(const QString &paramName) const {
  return m_model.getParameterError(paramName);
}

QString
ConvFunctionModel::getParameterDescription(const QString &paramName) const {
  return m_model.getParameterDescription(paramName);
}

QStringList ConvFunctionModel::getParameterNames() const {
  return m_model.getParameterNames();
}

IFunction_sptr ConvFunctionModel::getSingleFunction(int index) const {
  return m_model.getSingleFunction(index);
}

IFunction_sptr ConvFunctionModel::getCurrentFunction() const {
  return m_model.getCurrentFunction();
}

QStringList ConvFunctionModel::getGlobalParameters() const {
  return m_model.getGlobalParameters();
}

QStringList ConvFunctionModel::getLocalParameters() const {
  return m_model.getLocalParameters();
}

void ConvFunctionModel::setGlobalParameters(const QStringList &globals) {
  m_globals.clear();
  for (auto const &name : globals) {
    addGlobal(name);
  }
  auto newGlobals = makeGlobalList();
  m_model.setGlobalParameters(newGlobals);
}

bool ConvFunctionModel::isGlobal(const QString &parName) const {
  return m_model.isGlobal(parName);
}

void ConvFunctionModel::setGlobal(const QString &parName, bool on) {
  if (parName.isEmpty())
    return;
  if (on)
    addGlobal(parName);
  else
    removeGlobal(parName);
  auto globals = makeGlobalList();
  m_model.setGlobalParameters(globals);
}

void ConvFunctionModel::addGlobal(const QString &parName) {
  auto const pid = getParameterId(parName);
  if (pid && !m_globals.contains(*pid)) {
    m_globals.push_back(*pid);
  }
}

void ConvFunctionModel::removeGlobal(const QString &parName) {
  auto const pid = getParameterId(parName);
  if (pid && m_globals.contains(*pid)) {
    m_globals.removeOne(*pid);
  }
}

QStringList ConvFunctionModel::makeGlobalList() const {
  QStringList globals;
  for (auto const id : m_globals) {
    auto const name = getParameterName(id);
    if (name)
      globals << *name;
  }
  return globals;
}

void ConvFunctionModel::setFitType(FitType fitType) {
  auto oldValues = getCurrentValues();
  m_fitType = fitType;
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

int ConvFunctionModel::getNumberOfPeaks() const { 
  if (m_fitType == FitType::None)
    return 0;
  if (m_fitType == FitType::TwoLorentzians)
    return 2;
  return 1;
}

void ConvFunctionModel::updateMultiDatasetParameters(const IFunction &fun) {
  m_model.updateMultiDatasetParameters(fun);
}

void ConvFunctionModel::updateMultiDatasetParameters(
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

void ConvFunctionModel::updateParameters(const IFunction &fun) {
  m_model.updateParameters(fun);
}

void ConvFunctionModel::setCurrentDomainIndex(int i) {
  m_model.setCurrentDomainIndex(i);
}

int ConvFunctionModel::currentDomainIndex() const {
  return m_model.currentDomainIndex();
}

void ConvFunctionModel::changeTie(const QString &paramName,
                                  const QString &tie) {
  m_model.changeTie(paramName, tie);
}

void ConvFunctionModel::addConstraint(const QString &functionIndex,
                                      const QString &constraint) {
  m_model.addConstraint(functionIndex, constraint);
}

void ConvFunctionModel::removeConstraint(const QString &paramName) {
  m_model.removeConstraint(paramName);
}

void ConvFunctionModel::setDatasetNames(const QStringList &names) {
  m_model.setDatasetNames(names);
}

QStringList ConvFunctionModel::getDatasetNames() const {
  return m_model.getDatasetNames();
}

double ConvFunctionModel::getLocalParameterValue(const QString &parName,
                                                 int i) const {
  return m_model.getLocalParameterValue(parName, i);
}

bool ConvFunctionModel::isLocalParameterFixed(const QString &parName,
                                              int i) const {
  return m_model.isLocalParameterFixed(parName, i);
}

QString ConvFunctionModel::getLocalParameterTie(const QString &parName,
                                                int i) const {
  return m_model.getLocalParameterTie(parName, i);
}

QString ConvFunctionModel::getLocalParameterConstraint(const QString &parName,
                                                       int i) const {
  return m_model.getLocalParameterConstraint(parName, i);
}

void ConvFunctionModel::setLocalParameterValue(const QString &parName, int i,
                                               double value) {
  m_model.setLocalParameterValue(parName, i, value);
}

void ConvFunctionModel::setLocalParameterValue(const QString &parName, int i,
                                               double value, double error) {
  m_model.setLocalParameterValue(parName, i, value, error);
}

void ConvFunctionModel::setLocalParameterTie(const QString &parName, int i,
                                             const QString &tie) {
  m_model.setLocalParameterTie(parName, i, tie);
}

void ConvFunctionModel::setLocalParameterConstraint(const QString &parName,
                                                    int i,
                                                    const QString &constraint) {
  m_model.setLocalParameterConstraint(parName, i, constraint);
}

void ConvFunctionModel::setLocalParameterFixed(const QString &parName, int i,
                                               bool fixed) {
  m_model.setLocalParameterFixed(parName, i, fixed);
}

void ConvFunctionModel::setParameter(ParamID name, double value) {
  auto const prefix = getPrefix(name);
  if (prefix) {
    m_model.setParameter(*prefix + paramName(name), value);
  }
}

boost::optional<double> ConvFunctionModel::getParameter(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model.getParameter(*paramName)
                   : boost::optional<double>();
}

boost::optional<double>
ConvFunctionModel::getParameterError(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model.getParameterError(*paramName)
                   : boost::optional<double>();
}

boost::optional<QString>
ConvFunctionModel::getParameterName(ParamID name) const {
  auto const prefix = getPrefix(name);
  return prefix ? *prefix + paramName(name) : boost::optional<QString>();
}

boost::optional<QString>
ConvFunctionModel::getParameterDescription(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model.getParameterDescription(*paramName)
                   : boost::optional<QString>();
}

boost::optional<QString> ConvFunctionModel::getPrefix(ParamID name) const {
  if (name <= ParamID::LOR1_FWHM) {
    return getLor1Prefix();
  } else if (name <= ParamID::LOR2_FWHM) {
    return getLor2Prefix();
  } else {
    return getBackgroundPrefix();
  }
}

QMap<ParamID, double>
ConvFunctionModel::getCurrentValues() const {
  QMap<ParamID, double> values;
  auto store = [&values, this](ParamID name) {
    values[name] = *getParameter(name);
  };
  applyParameterFunction(store);
  return values;
}

QMap<ParamID, double>
ConvFunctionModel::getCurrentErrors() const {
  QMap<ParamID, double> errors;
  auto store = [&errors, this](ParamID name) {
    errors[name] = *getParameterError(name);
  };
  applyParameterFunction(store);
  return errors;
}

QMap<int, QString> ConvFunctionModel::getParameterNameMap() const {
  QMap<int, QString> out;
  auto addToMap = [&out, this](ParamID name) {
    out[static_cast<int>(name)] = *getParameterName(name);
  };
  applyParameterFunction(addToMap);
  return out;
}

QMap<int, std::string> ConvFunctionModel::getParameterDescriptionMap() const {
  QMap<int, std::string> out;
  auto lorentzian = FunctionFactory::Instance().createInitialized(
      buildLorentzianFunctionString());
  out[static_cast<int>(ParamID::LOR1_AMPLITUDE)] =
      lorentzian->parameterDescription(0);
  out[static_cast<int>(ParamID::LOR1_PEAKCENTRE)] =
      lorentzian->parameterDescription(1);
  out[static_cast<int>(ParamID::LOR1_FWHM)] =
      lorentzian->parameterDescription(2);
  out[static_cast<int>(ParamID::LOR2_AMPLITUDE)] =
      lorentzian->parameterDescription(0);
  out[static_cast<int>(ParamID::LOR2_PEAKCENTRE)] =
      lorentzian->parameterDescription(1);
  out[static_cast<int>(ParamID::LOR2_FWHM)] =
      lorentzian->parameterDescription(2);
  auto background = FunctionFactory::Instance().createInitialized(
      buildBackgroundFunctionString());
  out[static_cast<int>(ParamID::BG_A0)] = background->parameterDescription(0);
  if (background->nParams() > 1)
    out[static_cast<int>(ParamID::BG_A1)] = background->parameterDescription(1);
  return out;
}

void ConvFunctionModel::setCurrentValues(const QMap<ParamID, double> &values) {
  for (auto const name : values.keys()) {
    setParameter(name, values[name]);
  }
}

void ConvFunctionModel::applyParameterFunction(
    std::function<void(ParamID)> paramFun) const {
  if (m_fitType == FitType::OneLorentzian ||
      m_fitType == FitType::TwoLorentzians) {
    paramFun(ParamID::LOR1_AMPLITUDE);
    paramFun(ParamID::LOR1_PEAKCENTRE);
    paramFun(ParamID::LOR1_FWHM);
  }
  if (m_fitType == FitType::TwoLorentzians) {
    paramFun(ParamID::LOR2_AMPLITUDE);
    paramFun(ParamID::LOR2_PEAKCENTRE);
    paramFun(ParamID::LOR2_FWHM);
  }
  if (!m_background.isEmpty()) {
    paramFun(ParamID::BG_A0);
    if (m_background == "LinearBackgrund") {
      paramFun(ParamID::BG_A1);
    }
  }
}

boost::optional<ParamID>
ConvFunctionModel::getParameterId(const QString &parName) {
  boost::optional<ParamID> result;
  auto getter = [&result, parName, this](ParamID pid) {
    if (parName == *getParameterName(pid))
      result = pid;
  };
  applyParameterFunction(getter);
  return result;
}

std::string ConvFunctionModel::buildLorentzianFunctionString() const {
  return "name=Lorentzian,Amplitude=1,FWHM=1,constraints=(Amplitude>0,FWHM>0)";
}

std::string ConvFunctionModel::buildDeltaFunctionString() const {
  return "name=DeltaFunction";
}

std::string ConvFunctionModel::buildBackgroundFunctionString() const {
  return "name=" + m_background.toStdString() + ",A0=0,constraints=(A0>0)";
}

//void ConvFunctionModel::estimateStretchExpParameters() {
//  auto const heightName = getParameterName(ParamID::STRETCH_HEIGHT);
//  auto const lifeTimeName = getParameterName(ParamID::STRETCH_LIFETIME);
//  auto const stretchingName = getParameterName(ParamID::STRETCH_STRETCHING);
//  if (!heightName || !lifeTimeName || !stretchingName)
//    return;
//  assert(getNumberDomains() == m_estimationData.size());
//  for (auto i = 0; i < getNumberDomains(); ++i) {
//    auto const &x = m_estimationData[i].x;
//    auto const &y = m_estimationData[i].y;
//    auto lifeTime = (x[1] - x[0]) / (log(y[0]) - log(y[1]));
//    if (lifeTime <= 0)
//      lifeTime = 1.0;
//    auto const height = y[0] * exp(x[0] / lifeTime);
//    setLocalParameterValue(*heightName, i, height);
//    setLocalParameterValue(*lifeTimeName, i, lifeTime);
//    setLocalParameterValue(*stretchingName, i, 1.0);
//  }
//}

QString ConvFunctionModel::buildFunctionString() const {
  QStringList functions;
  if (m_fitType == FitType::OneLorentzian) {
    functions << QString::fromStdString(buildLorentzianFunctionString());
  }
  if (m_fitType == FitType::TwoLorentzians) {
    functions << QString::fromStdString(buildLorentzianFunctionString());
    functions << QString::fromStdString(buildLorentzianFunctionString());
  }
  if (m_hasDeltaFunction) {
    functions << QString::fromStdString(buildDeltaFunctionString());
  }
  if (!m_background.isEmpty()) {
    functions << QString::fromStdString(buildBackgroundFunctionString());
  }
  return functions.join(";");
}

boost::optional<QString> ConvFunctionModel::getLor1Prefix() const {
  if (m_fitType != FitType::OneLorentzian &&
      m_fitType != FitType::TwoLorentzians)
    return boost::optional<QString>();
  if (m_fitType != FitType::OneLorentzian && !m_hasDeltaFunction &&
      m_background.isEmpty())
    return "";
  return "f0.";
}

boost::optional<QString> ConvFunctionModel::getLor2Prefix() const {
  if (m_fitType != FitType::TwoLorentzians)
    return boost::optional<QString>();
  return "f1.";
}

boost::optional<QString> ConvFunctionModel::getDeltaPrefix() const {
  if (!m_hasDeltaFunction)
    return boost::optional<QString>();
  if (m_fitType == FitType::None && m_background.isEmpty())
    return "";
  return QString("f%1.").arg(getNumberOfPeaks());
}

boost::optional<QString> ConvFunctionModel::getBackgroundPrefix() const {
  if (m_background.isEmpty())
    return boost::optional<QString>();
  auto const numberOfPeaks = getNumberOfPeaks();
  if (numberOfPeaks == 0 && !m_hasDeltaFunction)
    return "";
  return QString("f%1.").arg(numberOfPeaks + (m_hasDeltaFunction ? 1 : 0));
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
