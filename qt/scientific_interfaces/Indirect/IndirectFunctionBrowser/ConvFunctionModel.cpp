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
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace MantidWidgets;
using namespace Mantid::API;

ConvFunctionModel::ConvFunctionModel() {}

void ConvFunctionModel::clearData() {
  m_fitType = FitType::None;
  m_hasDeltaFunction = false;
  m_hasTempCorrection = false;
  m_backgroundType = BackgroundType::None;
  m_model.clear();
}

void ConvFunctionModel::setModel() {
  m_model.setModel(buildBackgroundFunctionString(), m_fitResolutions,
                   buildPeaksFunctionString(), m_hasDeltaFunction, m_qValues,
                   m_isQDependentFunction, m_hasTempCorrection);
  m_model.setGlobalParameters(makeGlobalList());
}

void ConvFunctionModel::setFunction(IFunction_sptr fun) {
  clearData();
  if (!fun)
    return;
  bool isBackgroundSet = false;
  if (fun->name() == "Convolution") {
    checkConvolution(fun);
  } else if (fun->name() == "CompositeFunction") {
    for (size_t i = 0; i < fun->nFunctions(); ++i) {
      auto innerFunction = fun->getFunction(i);
      auto const name = innerFunction->name();
      if (name == "FlatBackground") {
        if (isBackgroundSet) {
          throw std::runtime_error("Function has wrong structure.");
        }
        m_backgroundType = BackgroundType::Flat;
        isBackgroundSet = true;
      } else if (name == "LinearBackground") {
        if (isBackgroundSet) {
          throw std::runtime_error("Function has wrong structure.");
        }
        m_backgroundType = BackgroundType::Linear;
        isBackgroundSet = true;
      } else if (name == "Convolution") {
        checkConvolution(innerFunction);
      }
    }
  }
  m_model.setFunction(fun);
}

void ConvFunctionModel::checkConvolution(IFunction_sptr fun) {
  bool isFitTypeSet = false;
  bool isResolutionSet = false;
  for (size_t i = 0; i < fun->nFunctions(); ++i) {
    auto innerFunction = fun->getFunction(i);
    auto const name = innerFunction->name();
    if (name == "Resolution") {
      if (isResolutionSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      isResolutionSet = true;
    } else if (name == "ProductFunction") {
      if (innerFunction->getFunction(0)->name() != "UserFunction") {
        throw std::runtime_error("Function has wrong structure.");
      }
      m_hasTempCorrection = true;
      checkConvolution(innerFunction->getFunction(1));
    }

    else if (name == "CompositeFunction") {
      checkConvolution(innerFunction);
    } else if (name == "Lorentzian") {
      if (isFitTypeSet && m_fitType != FitType::OneLorentzian) {
        throw std::runtime_error("Function has wrong structure.");
      }
      if (isFitTypeSet)
        m_fitType = FitType::TwoLorentzians;
      else
        m_fitType = FitType::OneLorentzian;
      m_isQDependentFunction = false;
      isFitTypeSet = true;

    } else if (FitTypeStringToEnum.count(name) == 1) {
      if (isFitTypeSet) {
        throw std::runtime_error(
            "Function has wrong structure. More than one fit type set");
      }
      m_fitType = FitTypeStringToEnum[name];
      m_isQDependentFunction = FitTypeQDepends[m_fitType];
      isFitTypeSet = true;
    } else if (name == "DeltaFunction") {
      m_hasDeltaFunction = true;
    } else {
      clear();
      throw std::runtime_error(
          "Function has wrong structure. Function not recognized");
    }
  }
}

// void ConvFunctionModel::checkProductFunction(IFunction_sptr fun) {}

IFunction_sptr ConvFunctionModel::getFitFunction() const {
  return m_model.getFitFunction();
}

void ConvFunctionModel::setQValues(const std::vector<double> &qValues) {
  m_qValues = qValues;
}

FitType ConvFunctionModel::getFitType() const { return m_fitType; }
BackgroundType ConvFunctionModel::getBackgroundType() const {
  return m_backgroundType;
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
  } else if (name == "FlatBackground" || name == "LinearBackground") {
    if (hasBackground())
      throw std::runtime_error("Cannot add more backgrounds.");
    if (name == "FlatBackground") {
      setBackground(BackgroundType::Flat);
    } else if (name == "LinearBackground") {
      setBackground(BackgroundType::Linear);
    }
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

void ConvFunctionModel::setDeltaFunction(bool on) {
  auto oldValues = getCurrentValues();
  m_hasDeltaFunction = on;
  setModel();
  setCurrentValues(oldValues);
}

void ConvFunctionModel::setTempCorrection(bool on) {
  auto oldValues = getCurrentValues();
  m_hasTempCorrection = on;
  setModel();
  setCurrentValues(oldValues);
}

bool ConvFunctionModel::hasTempCorrection() const {
  return m_hasTempCorrection;
}

bool ConvFunctionModel::hasDeltaFunction() const { return m_hasDeltaFunction; }

void ConvFunctionModel::setBackground(BackgroundType bgType) {
  auto oldValues = getCurrentValues();
  m_backgroundType = bgType;
  setModel();
  setCurrentValues(oldValues);
}

void ConvFunctionModel::removeBackground() {
  auto oldValues = getCurrentValues();
  m_backgroundType = BackgroundType::None;
  setModel();
  setCurrentValues(oldValues);
}

bool ConvFunctionModel::hasBackground() const {
  return m_backgroundType != BackgroundType::None;
}

void ConvFunctionModel::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_estimationData = std::move(data);
}

void ConvFunctionModel::setResolution(std::string const &name,
                                      TableDatasetIndex const &index) {
  m_resolutionName = name;
  m_resolutionIndex = index;
  setModel();
}

void ConvFunctionModel::setResolution(
    const std::vector<std::pair<std::string, int>> &fitResolutions) {
  m_fitResolutions = fitResolutions;
  setModel();
}

QString ConvFunctionModel::setBackgroundA0(double value) {
  if (hasBackground()) {
    auto const paramID = (m_backgroundType == BackgroundType::Flat)
                             ? ParamID::FLAT_BG_A0
                             : ParamID::LINEAR_BG_A0;
    setParameter(paramID, value);
    return *getParameterName(paramID);
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
  m_fitType = fitType;
  if (FitTypeQDepends[m_fitType])
    m_isQDependentFunction = true;
  else
    m_isQDependentFunction = false;
  setModel();
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
    auto valueColumn = paramTable.getColumn((name).toStdString());
    auto errorColumn = paramTable.getColumn((name + "_Err").toStdString());
    m_model.setParameter(name, valueColumn->toDouble(0));
    m_model.setParameterError(name, errorColumn->toDouble(0));
  }

  auto const localParameterNames = getLocalParameters();
  for (auto &&name : localParameterNames) {
    auto valueColumn = paramTable.getColumn((name).toStdString());
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
  if (name >= ParamID::FLAT_BG_A0) {
    return m_model.backgroundPrefix();
  } else if (name == ParamID::DELTA_HEIGHT) {
    return m_model.deltaFunctionPrefix();
  } else if (name == ParamID::TEMPERATURE) {
    return m_model.tempFunctionPrefix();
  } else {
    auto const prefixes = m_model.peakPrefixes();
    if (!prefixes)
      return boost::optional<QString>();
    auto const index =
        name > ParamID::LOR2_FWHM_1 && name <= ParamID::LOR2_FWHM_2 ? 1 : 0;
    return m_model.peakPrefixes()->at(index);
  }
}

QMap<ParamID, double> ConvFunctionModel::getCurrentValues() const {
  QMap<ParamID, double> values;
  auto store = [&values, this](ParamID name) {
    values[name] = *getParameter(name);
  };
  applyParameterFunction(store);
  return values;
}

QMap<ParamID, double> ConvFunctionModel::getCurrentErrors() const {
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

void ConvFunctionModel::setCurrentValues(const QMap<ParamID, double> &values) {
  for (auto const name : values.keys()) {
    setParameter(name, values[name]);
  }
}

void ConvFunctionModel::applyParameterFunction(
    std::function<void(ParamID)> paramFun) const {
  applyToFitType(m_fitType, paramFun);
  applyToBackground(m_backgroundType, paramFun);
  applyToDelta(m_hasDeltaFunction, paramFun);
  auto tempType = m_hasTempCorrection ? TempCorrectionType::Exponential
                                      : TempCorrectionType::None;
  applyToTemp(tempType, paramFun);
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
  return "name=Lorentzian,Amplitude=1,FWHM=1,constraints=(Amplitude>0,FWHM>"
         "0)";
}

std::string ConvFunctionModel::buildTeixeiraFunctionString() const {
  return "name=TeixeiraWaterSQE";
}

std::string ConvFunctionModel::buildStretchExpFTFunctionString() const {
  return "name=StretchedExpFT";
}

std::string ConvFunctionModel::buildElasticDiffSphereFunctionString() const {
  return "name=ElasticDiffSphere";
}

std::string ConvFunctionModel::buildInelasticDiffSphereFunctionString() const {
  return "name=InelasticDiffSphere";
}

std::string
ConvFunctionModel::buildInelasticDiffRotDiscreteCircleFunctionString() const {
  return "name=InelasticDiffRotDiscreteCircle";
}

std::string
ConvFunctionModel::buildElasticDiffRotDiscreteCircleFunctionString() const {
  return "name=ElasticDiffRotDiscreteCircle";
}

std::string ConvFunctionModel::buildPeaksFunctionString() const {
  std::string functions;
  if (m_fitType == FitType::OneLorentzian) {
    functions.append(buildLorentzianFunctionString());
  } else if (m_fitType == FitType::TwoLorentzians) {
    auto const lorentzian = buildLorentzianFunctionString();
    functions.append(lorentzian);
    functions.append(";");
    functions.append(lorentzian);
  } else if (m_fitType == FitType::TeixeiraWater) {
    functions.append(buildTeixeiraFunctionString());
  } else if (m_fitType == FitType::StretchedExpFT) {
    functions.append(buildStretchExpFTFunctionString());
  } else if (m_fitType == FitType::ElasticDiffSphere) {
    functions.append(buildElasticDiffSphereFunctionString());
  } else if (m_fitType == FitType::InelasticDiffSphere) {
    functions.append(buildInelasticDiffSphereFunctionString());
  } else if (m_fitType == FitType::InelasticDiffRotDiscreteCircle) {
    functions.append(buildInelasticDiffRotDiscreteCircleFunctionString());
  } else if (m_fitType == FitType::ElasticDiffRotDiscreteCircle) {
    functions.append(buildElasticDiffRotDiscreteCircleFunctionString());
  }
  return functions;
} // namespace IDA

std::string ConvFunctionModel::buildBackgroundFunctionString() const {
  if (m_backgroundType == BackgroundType::None)
    return "";
  return "name=" + m_backgroundSubtype.getFunctionName(m_backgroundType) +
         ",A0=0,constraints=(A0>0)";
}

boost::optional<QString> ConvFunctionModel::getLor1Prefix() const {
  return m_model.peakPrefixes()->at(0);
}

boost::optional<QString> ConvFunctionModel::getLor2Prefix() const {
  return m_model.peakPrefixes()->at(1);
}

boost::optional<QString> ConvFunctionModel::getDeltaPrefix() const {
  return m_model.deltaFunctionPrefix();
}

boost::optional<QString> ConvFunctionModel::getBackgroundPrefix() const {
  return m_model.backgroundPrefix();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
