// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FQFunctionModel.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace MantidWidgets;
using namespace Mantid::API;

FQFunctionModel::FQFunctionModel() {}

void FQFunctionModel::updateAvailableFunctions(
    std::unordered_map<std::string, std::string>
        functionInitialisationStrings) {
  m_functionStore.clear();
  m_globalParameterStore.clear();
  for (auto functionInfo : functionInitialisationStrings) {
    auto function =
        FunctionFactory::Instance().createInitialized(functionInfo.second);
    m_functionStore.insert(QString::fromStdString(functionInfo.first),
                           function);
    m_globalParameterStore.insert(QString::fromStdString(functionInfo.first),
                                  QStringList());
  }
  m_fitType = m_functionStore.keys().first();
}

QStringList FQFunctionModel::getFunctionList() {
  return m_functionStore.keys();
}

int FQFunctionModel::getEnumIndex() {
  return m_functionStore.keys().indexOf(m_fitType);
}

void FQFunctionModel::setFunction(IFunction_sptr fun) {
  if (!fun)
    return;
  if (fun->nFunctions() == 0) {
    const auto name = fun->name();
    const auto &functionNameList = m_functionStore.keys();
    if (functionNameList.contains(QString::fromStdString(name))) {
      setFitType(QString::fromStdString(name));
    } else {
      throw std::runtime_error("Cannot set function " + name);
    }
  } else {
    throw std::runtime_error("Function has wrong structure.");
  }
}

void FQFunctionModel::setFitType(const QString &name) {
  if (m_function) {
    m_globalParameterStore[m_fitType] = getGlobalParameters();
  }
  m_fitType = name;
  if (name == "None") {
    clear();
    return;
  }
  setGlobalParameters(m_globalParameterStore[name]);
  FunctionModel::setFunction(m_functionStore[name]->clone());
}

QString FQFunctionModel::getFitType() { return m_fitType; }

void FQFunctionModel::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_estimationData = std::move(data);
}

void FQFunctionModel::setGlobal(const QString &name, bool isGlobal) {
  auto globalParameters = getGlobalParameters();
  if (isGlobal && !globalParameters.contains(name)) {
    globalParameters << name;
  } else if (!isGlobal && globalParameters.contains(name)) {
    globalParameters.removeAll(name);
  }
  globalParameters.removeDuplicates();
  setGlobalParameters(globalParameters);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt