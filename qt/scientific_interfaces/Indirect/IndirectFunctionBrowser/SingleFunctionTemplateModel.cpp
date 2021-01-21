// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SingleFunctionTemplateModel.h"
#include "IDAFunctionParameterEstimation.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

namespace {
// Sort the function list, keep existing order just rotate the None entry into
// the front
void sortFunctionList(QStringList &list) {
  auto ix = std::find(list.begin(), list.end(), QString("None"));
  if (ix == list.end() || ix == list.begin()) {
    return;
  } else {
    for (auto itr = ix; itr > list.begin(); --itr) {
      std::rotate(itr - 1, itr, itr + 1);
    }
  }
}
} // namespace

using namespace MantidWidgets;
using namespace Mantid::API;

SingleFunctionTemplateModel::SingleFunctionTemplateModel() {}

SingleFunctionTemplateModel::SingleFunctionTemplateModel(
    std::unique_ptr<IDAFunctionParameterEstimation> parameterEstimation)
    : m_parameterEstimation(std::move(parameterEstimation)) {}

void SingleFunctionTemplateModel::updateAvailableFunctions(
    const std::map<std::string, std::string> &functionInitialisationStrings) {
  m_fitTypeToFunctionStore.clear();
  m_globalParameterStore.clear();
  m_fitTypeList.clear();
  for (auto functionInfo : functionInitialisationStrings) {
    IFunction_sptr function;
    try {
      function = FunctionFactory::Instance().createInitialized(functionInfo.second);
    } catch (std::invalid_argument &) {
      // Error in function string, adding an empty function to map
    }
    m_fitTypeToFunctionStore.insert(QString::fromStdString(functionInfo.first), function);
    m_globalParameterStore.insert(QString::fromStdString(functionInfo.first), QStringList());
  }
  // Sort the FunctionList as None should always appear first
  m_fitTypeList = m_fitTypeToFunctionStore.keys();
  sortFunctionList(m_fitTypeList);
  m_fitType = m_fitTypeList.front();
}

QStringList SingleFunctionTemplateModel::getFunctionList() { return m_fitTypeList; }
int SingleFunctionTemplateModel::getEnumIndex() { return m_fitTypeList.indexOf(m_fitType); }

void SingleFunctionTemplateModel::setFunction(IFunction_sptr fun) {
  if (!fun)
    return;
  if (fun->nFunctions() == 0) {
    const auto name = fun->name();
    auto fitType = findFitTypeForFunctionName(QString::fromStdString(name));
    if (fitType) {
      setFitType(fitType.value());
    } else {
      throw std::runtime_error("Cannot set function " + name);
    }
  } else {
    throw std::runtime_error("Function has wrong structure.");
  }
}

void SingleFunctionTemplateModel::setFitType(const QString &type) {
  if (m_function) {
    m_globalParameterStore[m_fitType] = getGlobalParameters();
  }
  m_fitType = type;
  if (type == "None") {
    FunctionModel::setFunction(IFunction_sptr());
    return;
  }
  setGlobalParameters(m_globalParameterStore[type]);
  FunctionModel::setFunction(m_fitTypeToFunctionStore[type]->clone());
  estimateFunctionParameters();
}

QString SingleFunctionTemplateModel::getFitType() { return m_fitType; }

void SingleFunctionTemplateModel::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_estimationData = std::move(data);
}

void SingleFunctionTemplateModel::estimateFunctionParameters() {
  if (m_estimationData.size() != static_cast<size_t>(getNumberDomains())) {
    return;
  }
  // Estimate function parameters - parameters are updated in-place.
  for (int i = 0; i < getNumberDomains(); ++i) {
    auto function = getSingleFunction(i);
    m_parameterEstimation->estimateFunctionParameters(function, m_estimationData[i]);
  }
}

void SingleFunctionTemplateModel::setGlobal(const QString &name, bool isGlobal) {
  auto globalParameters = getGlobalParameters();
  if (isGlobal && !globalParameters.contains(name)) {
    globalParameters << name;
  } else if (!isGlobal && globalParameters.contains(name)) {
    globalParameters.removeAll(name);
  }
  globalParameters.removeDuplicates();
  setGlobalParameters(globalParameters);
}

boost::optional<QString> SingleFunctionTemplateModel::findFitTypeForFunctionName(const QString &name) const {
  auto nameAsString = name.toStdString();
  auto result = std::find_if(m_fitTypeToFunctionStore.constBegin(), m_fitTypeToFunctionStore.constEnd(),
                             [&nameAsString](const auto &function) -> bool {
                               if (function)
                                 return function->name() == nameAsString;
                               else
                                 return false;
                             });
  if (result != std::end(m_fitTypeToFunctionStore)) {
    return boost::optional<QString>{result.key()};
  }
  return boost::none;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
