// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SingleFunctionTemplateModel.h"
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

SingleFunctionTemplateModel::SingleFunctionTemplateModel() {}

void SingleFunctionTemplateModel::updateAvailableFunctions(
    const std::map<std::string, std::string> &functionInitialisationStrings) {
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

QStringList SingleFunctionTemplateModel::getFunctionList() {
  return m_functionStore.keys();
}

int SingleFunctionTemplateModel::getEnumIndex() {
  return m_functionStore.keys().indexOf(m_fitType);
}

void SingleFunctionTemplateModel::setFunction(IFunction_sptr fun) {
  if (!fun)
    return;
  if (fun->nFunctions() == 0) {
    const auto name = fun->name();
    std::vector<std::string> functionNameList;
    std::unordered_map<std::string, QString> functionNameToDisplayNameMap;
    for (auto functionKey : m_functionStore.keys()) {
      auto functionName = m_functionStore[functionKey]->name();
      functionNameList.emplace_back(functionName);
      functionNameToDisplayNameMap.emplace(functionName, functionKey);
    }
    if (std::find(functionNameList.cbegin(), functionNameList.cend(), name) !=
        functionNameList.cend()) {
      setFitType(functionNameToDisplayNameMap.at(name));
    } else {
      throw std::runtime_error("Cannot set function " + name);
    }
  } else {
    throw std::runtime_error("Function has wrong structure.");
  }
}

void SingleFunctionTemplateModel::setFitType(const QString &name) {
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

QString SingleFunctionTemplateModel::getFitType() { return m_fitType; }

void SingleFunctionTemplateModel::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_estimationData = std::move(data);
}

void SingleFunctionTemplateModel::setGlobal(const QString &name,
                                            bool isGlobal) {
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