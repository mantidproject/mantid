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
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include <map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace MantidWidgets;
using namespace Mantid::API;

FQFunctionModel::FQFunctionModel() {
  auto functionList = std::vector<std::string>(
      {"ChudleyElliot", "HallRoss", "FickDiffusion", "TeixeiraWater",
       "EISFDiffCylinder", "EISFDiffSphere", "EISFDiffSphereAlkyl"});

  for (auto functionName : functionList) {
    auto function = FunctionFactory::Instance().createFunction(functionName);
    m_functionStore.insert(QString::fromStdString(functionName), function);
    m_globalParameterStore.insert(QString::fromStdString(functionName),
                                  QStringList());
  }
  m_fitType = "None";
}

void FQFunctionModel::setFunction(IFunction_sptr fun) {
  // clearData();
  // if (!fun)
  //   return;
  // if (fun->nFunctions() == 0) {
  //   const auto name = fun->name();
  //   if (name == "MSDGauss" || name == "MSDPeters" || name == "MSDYi") {
  //     m_fitType = QString::fromStdString(name);
  //   } else {
  //     throw std::runtime_error("Cannot set function " + name);
  //   }
  //   setFunction(fun);
  //   return;
  // }
  // bool isFitTypeSet = false;
  // for (size_t i = 0; i < fun->nFunctions(); ++i) {
  //   const auto f = fun->getFunction(i);
  //   const auto name = f->name();
  //   if (name == "MSDGauss" || name == "MSDPeters" || name == "MSDYi") {
  //     if (isFitTypeSet) {
  //       throw std::runtime_error("Function has wrong structure.");
  //     }
  //     m_fitType = QString::fromStdString(name);
  //     isFitTypeSet = true;
  //   } else {
  //     clear();
  //     throw std::runtime_error("Function has wrong structure.");
  //   }
  // }
  // setFunction(fun);
}

void FQFunctionModel::addFunction(const QString &prefix,
                                  const QString &funStr) {
  // if (!prefix.isEmpty())
  //   throw std::runtime_error(
  //       "Function doesn't have member function with prefix " +
  //       prefix.toStdString());
  // const auto fun =
  //     FunctionFactory::Instance().createInitialized(funStr.toStdString());
  // const auto name = fun->name();
  // QString newPrefix;
  // if (name == "MSDGauss" || name == "MSDPeters" || name == "MSDYi") {
  //   setFitType(QString::fromStdString(name));
  //   newPrefix = *getFitTypePrefix();
  // } else {
  //   throw std::runtime_error("Cannot add function " + name);
  // }
  // auto newFun = getFunctionWithPrefix(newPrefix, getSingleFunction(0));
  // copyParametersAndErrors(*fun, *newFun);
  // if (getNumberLocalFunctions() > 1) {
  //   copyParametersAndErrorsToAllLocalFunctions(*getSingleFunction(0));
  // }
}

void FQFunctionModel::removeFunction(const QString &prefix) {
  // if (prefix.isEmpty()) {
  //   clear();
  //   return;
  // }
  // auto prefix1 = getFitTypePrefix();
  // if (prefix1 && *prefix1 == prefix) {
  //   removeFitType();
  //   return;
  // }
  // throw std::runtime_error(
  //     "Function doesn't have member function with prefix " +
  //     prefix.toStdString());
}

void FQFunctionModel::setFitType(const QString &name) {
  if (m_function) {
    m_globalParameterStore[m_fitType] = getGlobalParameters();
    m_functionStore[m_fitType] = m_function->clone();
  }
  m_fitType = name;
  setGlobalParameters(m_globalParameterStore[name]);
  FunctionModel::setFunction(m_functionStore[name]);
}

QString FQFunctionModel::getFitType() { return m_fitType; }

void FQFunctionModel::removeFitType() {
  // const auto oldValues = getCurrentValues();
  // m_fitType.clear();
  // setFunctionString(buildFunctionString());
  // setGlobalParameters(makeGlobalList());
  // setCurrentValues(oldValues);
}

void FQFunctionModel::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  // m_estimationData = std::move(data);
} // namespace IDA

void FQFunctionModel::setGlobalParameters(const QStringList &globals) {
  // m_globals.clear();
  // for (const auto &name : globals) {
  //   addGlobal(name);
  // }
  // auto newGlobals = makeGlobalList();
  // FunctionModel::setGlobalParameters(newGlobals);
}

void FQFunctionModel::setGlobal(const QString &parName, bool on) {
  // if (parName.isEmpty())
  //   return;
  // if (on)
  //   addGlobal(parName);
  // else
  //   removeGlobal(parName);
  // auto globals = makeGlobalList();
  // FunctionModel::setGlobalParameters(globals);
}

void FQFunctionModel::updateMultiDatasetParameters(
    const ITableWorkspace &paramTable) {
  // const auto nRows = paramTable.rowCount();
  // if (nRows == 0)
  //   return;

  // const auto globalParameterNames = getGlobalParameters();
  // for (auto &&name : globalParameterNames) {
  //   const auto valueColumn = paramTable.getColumn(name.toStdString());
  //   const auto errorColumn =
  //       paramTable.getColumn((name + "_Err").toStdString());
  //   setParameter(name, valueColumn->toDouble(0));
  //   setParameterError(name, errorColumn->toDouble(0));
  // }

  // const auto localParameterNames = getLocalParameters();
  // for (auto &&name : localParameterNames) {
  //   const auto valueColumn = paramTable.getColumn(name.toStdString());
  //   const auto errorColumn =
  //       paramTable.getColumn((name + "_Err").toStdString());
  //   if (nRows > 1) {
  //     for (size_t i = 0; i < nRows; ++i) {
  //       setLocalParameterValue(name, static_cast<int>(i),
  //                              valueColumn->toDouble(i),
  //                              errorColumn->toDouble(i));
  //     }
  //   } else {
  //     const auto i = currentDomainIndex();
  //     setLocalParameterValue(name, static_cast<int>(i),
  //                            valueColumn->toDouble(0),
  //                            errorColumn->toDouble(0));
  //   }
  // }
} // namespace IDA

// void FQFunctionModel::setParameter(ParamID name, double value) {
//   // const auto prefix = getPrefix(name);
//   // if (prefix) {
//   //   FunctionModel::setParameter(*prefix /*+ g_paramName.at(name)*/,
//   value);
//   // }
// }

// boost::optional<double> FQFunctionModel::getParameter(ParamID name) const {
//   const auto paramName = getParameterName(name);
//   return paramName ? getParameter(*paramName) : boost::optional<double>();
// }

// boost::optional<double> FQFunctionModel::getParameterError(ParamID name)
// const {
//   const auto paramName = getParameterName(name);
//   return paramName ? getParameterError(*paramName) :
//   boost::optional<double>();
// }

// boost::optional<QString> FQFunctionModel::getParameterName(ParamID name)
// const {
//   const auto prefix = getPrefix(name);
//   return prefix ? *prefix /*+ g_paramName.at(name)*/
//                 : boost::optional<QString>();
// }

// boost::optional<QString>
// FQFunctionModel::getParameterDescription(ParamID name) const {
//   const auto paramName = getParameterName(name);
//   return paramName ? getParameterDescription(*paramName)
//                    : boost::optional<QString>();
// }

// boost::optional<QString> FQFunctionModel::getPrefix(ParamID) const {
//   return getFitTypePrefix();
// }

// QMap<FQFunctionModel::ParamID, double>
// FQFunctionModel::getCurrentValues() const {
//   QMap<ParamID, double> values;
//   auto store = [&values, this](ParamID name) {
//     values[name] = *getParameter(name);
//   };
//   applyParameterFunction(store);
//   return values;
// }

// QMap<FQFunctionModel::ParamID, double>
// FQFunctionModel::getCurrentErrors() const {
//   QMap<ParamID, double> errors;
//   auto store = [&errors, this](ParamID name) {
//     errors[name] = *getParameterError(name);
//   };
//   applyParameterFunction(store);
//   return errors;
// }

QMap<int, QString> FQFunctionModel::getParameterNameMap() const {
  QMap<int, QString> out;
  // auto addToMap = [&out, this](ParamID name) {
  //   out[static_cast<int>(name)] = *getParameterName(name);
  // };
  // applyParameterFunction(addToMap);
  return out;
}

QMap<int, std::string> FQFunctionModel::getParameterDescriptionMap() const {
  QMap<int, std::string> out;
  // auto gaussian = FunctionFactory::Instance().createInitialized(
  //     buildGaussianFunctionString());
  // out[static_cast<int>(ParamID::GAUSSIAN_HEIGHT)] =
  //     gaussian->parameterDescription(0);
  // out[static_cast<int>(ParamID::GAUSSIAN_FQ)] =
  //     gaussian->parameterDescription(1);
  // auto peters = FunctionFactory::Instance().createInitialized(
  //     buildPetersFunctionString());
  // out[static_cast<int>(ParamID::PETERS_HEIGHT)] =
  //     peters->parameterDescription(0);
  // out[static_cast<int>(ParamID::PETERS_FQ)] =
  // peters->parameterDescription(1);
  // out[static_cast<int>(ParamID::PETERS_BETA)] =
  // peters->parameterDescription(2); auto yi =
  //     FunctionFactory::Instance().createInitialized(buildYiFunctionString());
  // out[static_cast<int>(ParamID::YI_HEIGHT)] = yi->parameterDescription(0);
  // out[static_cast<int>(ParamID::YI_FQ)] = yi->parameterDescription(1);
  // out[static_cast<int>(ParamID::YI_SIGMA)] = yi->parameterDescription(2);
  return out;
}

// void FQFunctionModel::setCurrentValues(const QMap<ParamID, double> &values) {
//   for (const auto &name : values.keys()) {
//     setParameter(name, values[name]);
//   }
// }

// void FQFunctionModel::applyParameterFunction(
//     std::function<void(ParamID)> paramFun) const {
//   // if (m_fitType == "MSDGauss") {
//   //   paramFun(ParamID::GAUSSIAN_HEIGHT);
//   //   paramFun(ParamID::GAUSSIAN_FQ);
//   // }
//   // if (m_fitType == "MSDPeters") {
//   //   paramFun(ParamID::PETERS_HEIGHT);
//   //   paramFun(ParamID::PETERS_FQ);
//   //   paramFun(ParamID::PETERS_BETA);
//   // }
//   // if (m_fitType == "MSDYi") {
//   //   paramFun(ParamID::YI_HEIGHT);
//   //   paramFun(ParamID::YI_FQ);
//   //   paramFun(ParamID::YI_SIGMA);
//   // }
// }
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt