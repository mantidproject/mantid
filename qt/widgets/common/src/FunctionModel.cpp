// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionModel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"

#include <algorithm>
#include <utility>

namespace {
Mantid::Kernel::Logger g_log("FitFunction");
}

namespace MantidQt::MantidWidgets {

using namespace Mantid::API;

void FunctionModel::setFunction(IFunction_sptr fun) {
  m_globalParameterNames.clear();
  m_function = std::dynamic_pointer_cast<MultiDomainFunction>(fun);
  if (m_function) {
    setResolutionFromWorkspace(m_function);
    return;
  }
  m_function = MultiDomainFunction_sptr(new MultiDomainFunction);
  if (fun) {
    auto const nf = m_numberDomains > 0 ? static_cast<int>(m_numberDomains) : 1;
    setResolutionFromWorkspace(fun);
    for (int i = 0; i < nf; ++i) {
      m_function->addFunction(fun->clone());
      m_function->setDomainIndex(i, i);
    }
  }
}

IFunction_sptr FunctionModel::getFullFunction() const {
  // It is important that this does not return a copy/clone of the function
  return m_function;
}

IFunction_sptr FunctionModel::getFitFunction() const {
  if (!m_function)
    return m_function;

  auto const numberOfFunctions = m_function->nFunctions();

  if (numberOfFunctions > 1) {
    if (m_currentDomainIndex < numberOfFunctions)
      return getFitFunctionWithGlobals(m_currentDomainIndex);
    return getFitFunctionWithGlobals(0);

  } else if (numberOfFunctions == 1) {
    auto const function = m_function->getFunction(0)->clone();
    auto const composite = std::dynamic_pointer_cast<CompositeFunction>(function);

    if (composite && composite->nFunctions() == 1)
      return composite->getFunction(0);
    return function;
  }
  return IFunction_sptr();
}

IFunction_sptr FunctionModel::getFitFunctionWithGlobals(std::size_t const &index) const {
  auto function = std::dynamic_pointer_cast<MultiDomainFunction>(m_function->clone());

  auto const singleFun = m_function->getFunction(index);
  for (auto paramIter = m_globalParameterNames.begin(); paramIter != m_globalParameterNames.end();) {
    if (singleFun->hasParameter(*paramIter)) {
      QStringList ties;
      for (auto i = 0u; i < m_function->nFunctions(); ++i)
        if (i != index)
          ties << "f" + QString::number(i) + "." + QString::fromStdString(*paramIter);

      ties << "f" + QString::number(index) + "." + QString::fromStdString(*paramIter);
      function->addTies(ties.join("=").toStdString());
      ++paramIter;
    } else {
      paramIter = m_globalParameterNames.erase(paramIter);
    }
  }
  return function;
}

bool FunctionModel::hasFunction() const {
  if (!m_function || m_function->nFunctions() == 0)
    return false;
  return true;
}

void FunctionModel::addFunction(std::string const &prefix, std::string const &funStr) {
  if (!hasFunction()) {
    setFunctionString(funStr);
    return;
  }
  auto newFun = FunctionFactory::Instance().createInitialized(funStr);
  auto const nf = m_numberDomains > 0 ? static_cast<int>(m_numberDomains) : 1;
  for (int i = 0; i < nf; ++i) {
    auto fun = getSingleFunction(i);
    auto parentFun = getFunctionWithPrefix(prefix, fun);
    auto cf = std::dynamic_pointer_cast<CompositeFunction>(parentFun);
    if (cf) {
      cf->addFunction(newFun->clone());
    } else if (i == 0 && prefix.empty()) {
      setFunctionString(getFunctionString() + ";" + funStr);
      break;
    } else {
      throw std::runtime_error("Function at " + prefix + " is not composite.");
    }
  }
  m_function->checkFunction();
  updateGlobals();
}

void FunctionModel::removeFunction(std::string const &functionIndex) {
  std::string prefix;
  int index;
  std::tie(prefix, index) = splitFunctionPrefix(functionIndex);
  if (prefix.empty() && index == -1) {
    clear();
    return;
  }
  auto const nf = m_numberDomains > 0 ? static_cast<int>(m_numberDomains) : 1;
  for (int i = 0; i < nf; ++i) {
    auto fun = getSingleFunction(i);
    auto parentFun = getFunctionWithPrefix(prefix, fun);
    auto cf = std::dynamic_pointer_cast<CompositeFunction>(parentFun);
    if (!cf) {
      throw std::runtime_error("Function at " + prefix + " is not composite.");
    }
    cf->removeFunction(index);
    if (cf->nFunctions() == 1 && prefix.empty() && cf->name() == "CompositeFunction") {
      m_function->replaceFunction(i, cf->getFunction(0));
      m_function->checkFunction();
    } else {
      cf->checkFunction();
    }
  }
  m_function->checkFunction();
  updateGlobals();
}

void FunctionModel::setParameter(std::string const &parameterName, double value) {
  if (isGlobal(parameterName))
    setGlobalParameterValue(parameterName, value);
  else
    setLocalParameterValue(parameterName, static_cast<int>(m_currentDomainIndex), value);
}

void FunctionModel::setAttribute(std::string const &attrName, const IFunction::Attribute &value) {
  auto fun = getCurrentFunction();
  if (!fun) {
    throw std::logic_error("Function is undefined.");
  }
  if (fun->hasAttribute(attrName)) {
    fun->setAttribute(attrName, value);
  }
}

void FunctionModel::setParameterError(std::string const &parameterName, double value) {
  auto fun = getCurrentFunction();
  auto const index = fun->parameterIndex(parameterName);
  fun->setError(index, value);
}

double FunctionModel::getParameter(std::string const &parameterName) const {
  auto const fun = getCurrentFunction();
  return fun && fun->hasParameter(parameterName) ? fun->getParameter(parameterName) : 0.0;
}

IFunction::Attribute FunctionModel::getAttribute(std::string const &attrName) const {
  auto const fun = getCurrentFunction();
  return fun && fun->hasAttribute(attrName) ? fun->getAttribute(attrName) : IFunction::Attribute();
}

double FunctionModel::getParameterError(std::string const &parameterName) const {
  auto fun = getCurrentFunction();
  return fun && fun->hasParameter(parameterName) ? fun->getError(fun->parameterIndex(parameterName)) : 0.0;
}

std::string FunctionModel::getParameterDescription(std::string const &parameterName) const {
  auto fun = getCurrentFunction();
  return fun && fun->hasParameter(parameterName) ? fun->parameterDescription(fun->parameterIndex(parameterName)) : "";
}

bool FunctionModel::isParameterFixed(std::string const &parameterName) const {
  return isLocalParameterFixed(parameterName, static_cast<int>(m_currentDomainIndex));
}

std::string FunctionModel::getParameterTie(std::string const &parameterName) const {
  return getLocalParameterTie(parameterName, static_cast<int>(m_currentDomainIndex));
}

void FunctionModel::setParameterFixed(std::string const &parameterName, bool fixed) {
  setLocalParameterFixed(parameterName, static_cast<int>(m_currentDomainIndex), fixed);
}

void FunctionModel::setParameterTie(std::string const &parameterName, std::string const &tie) {
  setLocalParameterTie(parameterName, static_cast<int>(m_currentDomainIndex), tie);
}

std::vector<std::string> FunctionModel::getParameterNames() const {
  if (hasFunction()) {
    return getCurrentFunction()->getParameterNames();
  }
  return std::vector<std::string>{};
}
std::vector<std::string> FunctionModel::getAttributeNames() const {
  if (hasFunction()) {
    return getCurrentFunction()->getAttributeNames();
  }
  return std::vector<std::string>{};
}

IFunction_sptr FunctionModel::getSingleFunction(int index) const {
  if (!checkIndex(index) || !hasFunction()) {
    return IFunction_sptr();
  }
  return m_function->getFunction(index);
}

IFunction_sptr FunctionModel::getCurrentFunction() const {
  return getSingleFunction(static_cast<int>(m_currentDomainIndex));
}

void FunctionModel::setNumberDomains(int nDomains) {
  if (nDomains < 0) {
    throw std::runtime_error("Number of domains shouldn't be less than 0.");
  }
  auto const nd = static_cast<size_t>(nDomains);
  if (nd == m_numberDomains) {
    return;
  }
  if (!hasFunction()) {
    m_numberDomains = nd;
  } else {
    auto const nfOld = m_numberDomains > 0 ? m_numberDomains : 1;
    auto const lastIndex = nfOld - 1;
    auto const nf = nd > 0 ? nd : 1;
    if (nd > m_numberDomains) {
      auto fun = m_function->getFunction(lastIndex);
      for (size_t i = nfOld; i < nf; ++i) {
        m_function->addFunction(fun->clone());
        m_function->setDomainIndex(i, i);
      }
    } else {
      for (size_t i = lastIndex; i >= nf; --i) {
        m_function->removeFunction(i);
      }
      m_function->checkFunction();
      m_function->clearDomainIndices();
      for (size_t i = 0; i < m_function->nFunctions(); ++i) {
        m_function->setDomainIndex(i, i);
      }
    }
    m_numberDomains = nDomains;
  }
  if (m_currentDomainIndex >= m_numberDomains) {
    m_currentDomainIndex = m_numberDomains > 0 ? m_numberDomains - 1 : 0;
  }
}

/// Sets the datasets based on their workspace names. This assumes there is only
/// a single spectrum in the workspaces being fitted.
/// @param datasetNames :: Names of the workspaces to be fitted.
void FunctionModel::setDatasets(const std::vector<std::string> &datasetNames) {
  QList<FunctionModelDataset> datasets;
  for (const auto &datasetName : datasetNames)
    datasets.append(FunctionModelDataset(datasetName, FunctionModelSpectra("0")));

  setDatasets(datasets);
}

/// Sets the datasets using a map of <workspace name, spectra list>. This
/// should be used when the workspaces being fitted have multiple spectra.
/// @param datasets :: Names of workspaces to be fitted paired to a spectra
/// list.
void FunctionModel::setDatasets(const QList<FunctionModelDataset> &datasets) {
  checkNumberOfDomains(datasets);
  m_datasets = datasets;
}

/// Adds datasets based on their workspace names. This assumes there is only
/// a single spectrum in the added workspaces.
/// @param datasetNames :: Names of the workspaces to be added.
void FunctionModel::addDatasets(const std::vector<std::string> &datasetNames) {
  for (const auto &datasetName : datasetNames)
    m_datasets.append(FunctionModelDataset(datasetName, FunctionModelSpectra("0")));

  setNumberDomains(numberOfDomains(m_datasets));
}

/// Removes datasets (i.e. workspaces) from the Function model based on the
/// index of the dataset in the QList.
void FunctionModel::removeDatasets(QList<int> &indices) {
  checkDatasets();

  // Sort in reverse order
  using std::sort;
  sort(indices.begin(), indices.end(), [](int a, int b) { return a > b; });
  for (auto i = indices.constBegin(); i != indices.constEnd(); ++i)
    m_datasets.erase(m_datasets.begin() + *i);

  const auto nDomains = numberOfDomains(m_datasets);
  setNumberDomains(nDomains);

  auto currentIndex = currentDomainIndex();
  if (currentIndex >= nDomains)
    currentIndex = m_datasets.isEmpty() ? 0 : nDomains - 1;

  setCurrentDomainIndex(currentIndex);
}

/// Returns the workspace names of the datasets. If a dataset has N spectra,
/// then the workspace name is multiplied by N. This is required for
/// EditLocalParameterDialog.
std::vector<std::string> FunctionModel::getDatasetNames() const {
  std::vector<std::string> allDatasetNames;
  for (const auto &dataset : m_datasets)
    for (auto i = 0u; i < dataset.numberOfSpectra(); ++i) {
      UNUSED_ARG(i);
      allDatasetNames.emplace_back(dataset.datasetName());
    }
  return allDatasetNames;
}

/// Returns names for the domains of each dataset. If a dataset has multiple
/// spectra, then a domain name will include the spectrum number of a domain in
/// a workspace. This is required for EditLocalParameterDialog.
std::vector<std::string> FunctionModel::getDatasetDomainNames() const {
  std::vector<std::string> allDomainNames;
  for (const auto &dataset : m_datasets) {
    auto const domainNames = dataset.domainNames();
    allDomainNames.insert(allDomainNames.end(), domainNames.cbegin(), domainNames.cend());
  }
  return allDomainNames;
}

int FunctionModel::getNumberDomains() const { return static_cast<int>(m_numberDomains); }

int FunctionModel::currentDomainIndex() const { return static_cast<int>(m_currentDomainIndex); }

void FunctionModel::setCurrentDomainIndex(int index) {
  if (checkIndex(index)) {
    m_currentDomainIndex = static_cast<size_t>(index);
  }
}

double FunctionModel::getLocalParameterValue(std::string const &parameterName, int i) const {
  return getSingleFunction(i)->getParameter(parameterName);
}

bool FunctionModel::isLocalParameterFixed(std::string const &parameterName, int i) const {
  auto fun = getSingleFunction(i);
  auto const parIndex = fun->parameterIndex(parameterName);
  return fun->isFixed(parIndex);
}

std::string FunctionModel::getLocalParameterTie(std::string const &parameterName, int i) const {
  auto fun = getSingleFunction(i);
  auto const parIndex = fun->parameterIndex(parameterName);
  auto const tie = fun->getTie(parIndex);
  if (!tie)
    return "";
  auto const tieStr = QString::fromStdString(tie->asString());
  auto const j = tieStr.indexOf('=');
  return tieStr.mid(j + 1).toStdString();
}

std::string FunctionModel::getLocalParameterConstraint(std::string const &parameterName, int i) const {
  auto fun = getSingleFunction(i);
  auto const parIndex = fun->parameterIndex(parameterName);
  auto const constraint = fun->getConstraint(parIndex);
  auto const out = (!constraint) ? "" : constraint->asString();
  return out;
}

void FunctionModel::setLocalParameterValue(std::string const &parameterName, int i, double value) {
  auto function = getSingleFunction(i);
  if (function && function->hasParameter(parameterName))
    function->setParameter(parameterName, value);
}

void FunctionModel::setLocalParameterValue(std::string const &parameterName, int i, double value, double error) {
  auto fun = getSingleFunction(i);
  auto const parIndex = fun->parameterIndex(parameterName);
  fun->setParameter(parIndex, value);
  fun->setError(parIndex, error);
}

void FunctionModel::setLocalParameterFixed(std::string const &parameterName, int i, bool fixed) {
  auto fun = getSingleFunction(i);
  auto const parIndex = fun->parameterIndex(parameterName);
  if (fixed) {
    fun->fix(parIndex);
  } else if (fun->isFixed(parIndex)) {
    fun->unfix(parIndex);
  }
}

void FunctionModel::setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie) {
  auto logError = [&parameterName](const std::string &msg) {
    g_log.error() << "Tie " << parameterName << ": " << msg << '\n';
  };
  auto fun = getSingleFunction(i);
  auto const name = parameterName;
  if (tie.empty()) {
    fun->removeTie(fun->parameterIndex(name));
  } else {
    auto const j = QString::fromStdString(tie).indexOf('=');
    try {
      fun->tie(name, QString::fromStdString(tie).mid(j + 1).toStdString());
    } catch (const std::invalid_argument &e) {
      logError(e.what());
    } catch (const std::runtime_error &e) {
      logError(e.what());
    }
  }
}

void FunctionModel::setLocalParameterConstraint(std::string const &parameterName, int i,
                                                std::string const &constraint) {
  auto const parts = splitConstraintString(constraint);
  if (constraint != "" && parts.second.first == "" && parts.second.second == "") {
    g_log.error("Constraint " + parameterName + ": " + constraint + " is not a valid constraint");
    return;
  }
  std::string prefix, name;
  std::tie(prefix, name) = splitParameterName(parameterName);
  auto fun = getFunctionWithPrefix(prefix, getSingleFunction(i));
  if (constraint.empty()) {
    fun->removeConstraint(name);
  } else {
    auto newConstraint(QString::fromStdString(constraint));
    newConstraint.replace(QString::fromStdString(parts.first), QString::fromStdString(name));
    fun->addConstraints(newConstraint.toStdString());
  }
}

void FunctionModel::setGlobalParameterValue(std::string const &parameterName, double value) {
  if (isGlobal(parameterName))
    for (auto i = 0; i < getNumberDomains(); ++i)
      setLocalParameterValue(parameterName, i, value);
}

void FunctionModel::changeTie(std::string const &parameterName, std::string const &tie) {
  try {
    setLocalParameterTie(parameterName, static_cast<int>(m_currentDomainIndex), tie);
  } catch (std::exception &) {
    // the tie is probably being edited
  }
}

void FunctionModel::addConstraint(std::string const &functionIndex, std::string const &constraint) {
  auto fun = getFunctionWithPrefix(functionIndex, getCurrentFunction());
  fun->addConstraints(constraint);
}

void FunctionModel::removeConstraint(std::string const &parameterName) {
  getCurrentFunction()->removeConstraint(parameterName);
}

std::vector<std::string> FunctionModel::getGlobalParameters() const { return m_globalParameterNames; }

void FunctionModel::setGlobal(std::string const &parameterName, bool on) {
  if (parameterName.empty())
    return;
  if (!on) {
    auto newEnd = std::remove(m_globalParameterNames.begin(), m_globalParameterNames.end(), parameterName);
    if (newEnd != m_globalParameterNames.end()) {
      m_globalParameterNames.erase(newEnd, m_globalParameterNames.end());
    }
  } else if (std::find(m_globalParameterNames.cbegin(), m_globalParameterNames.cend(), parameterName) ==
             m_globalParameterNames.cend()) {
    m_globalParameterNames.emplace_back(parameterName);
  }
}

void FunctionModel::setGlobalParameters(const std::vector<std::string> &globals) { m_globalParameterNames = globals; }

std::vector<std::string> FunctionModel::getLocalParameters() const {
  auto const parameterNames = getParameterNames();
  std::vector<std::string> locals;
  std::copy_if(parameterNames.cbegin(), parameterNames.cend(), std::back_inserter(locals),
               [&](const std::string &parameterName) { return !isGlobal(parameterName); });
  return locals;
}

/// Check that the number of domains is correct for m_datasets
void FunctionModel::checkDatasets() {
  if (numberOfDomains(m_datasets) != static_cast<int>(m_numberDomains)) {
    m_datasets.clear();
    for (auto i = 0u; i < m_numberDomains; ++i)
      m_datasets.append(FunctionModelDataset(std::to_string(i), FunctionModelSpectra("0")));
  }
}

/// Check that the datasets supplied have the expected total number of domains.
void FunctionModel::checkNumberOfDomains(const QList<FunctionModelDataset> &datasets) const {
  if (numberOfDomains(datasets) != static_cast<int>(m_numberDomains)) {
    throw std::runtime_error("Number of dataset domains doesn't match the number of domains.");
  }
}

int FunctionModel::numberOfDomains(const QList<FunctionModelDataset> &datasets) const {
  return std::accumulate(datasets.cbegin(), datasets.cend(), 0, [](int lhs, const auto &dataset) {
    return lhs + static_cast<int>(dataset.numberOfSpectra());
  });
}

bool FunctionModel::checkIndex(int const index) const {
  auto const indexInRange = index == 0 || (index > 0 && index < getNumberDomains());
  // If the domain index is out of range, this indicates a problem in the logic of our code.
  // We want the index to be ignored in Release mode (i.e. for a user), but we want an exception
  // when in Debug mode (i.e. to alert a dev of the logic issue).
  assert(indexInRange);
  return indexInRange;
}

void FunctionModel::updateMultiDatasetParameters(const IFunction &fun) {
  if (!hasFunction())
    return;
  copyParametersAndErrors(fun, *m_function);
  updateMultiDatasetAttributes(fun);
}

void FunctionModel::updateMultiDatasetParameters(const ITableWorkspace &paramTable) {
  auto const nRows = paramTable.rowCount();
  if (nRows == 0)
    return;

  auto const globalParameterNames = getGlobalParameters();
  for (auto &&name : globalParameterNames) {
    auto valueColumn = paramTable.getColumn(name);
    auto errorColumn = paramTable.getColumn(name + "_Err");
    setParameter(name, valueColumn->toDouble(0));
    setParameterError(name, errorColumn->toDouble(0));
  }

  auto const localParameterNames = getLocalParameters();
  for (auto &&name : localParameterNames) {
    auto valueColumn = paramTable.getColumn(name);
    auto errorColumn = paramTable.getColumn(name + "_Err");
    if (nRows > 1) {
      for (size_t i = 0; i < nRows; ++i) {
        setLocalParameterValue(name, static_cast<int>(i), valueColumn->toDouble(i), errorColumn->toDouble(i));
      }
    } else {
      auto const i = currentDomainIndex();
      setLocalParameterValue(name, static_cast<int>(i), valueColumn->toDouble(0), errorColumn->toDouble(0));
    }
  }
}

void FunctionModel::updateMultiDatasetAttributes(const IFunction &fun) {
  if (!hasFunction())
    return;
  if (m_function->nAttributes() != fun.nAttributes())
    return;
  for (const auto &name : fun.getAttributeNames()) {
    m_function->setAttribute(name, fun.getAttribute(name));
  }
}

void FunctionModel::updateParameters(const IFunction &fun) {
  if (!hasFunction())
    return;
  auto currentFun = getCurrentFunction();
  copyParametersAndErrors(fun, *currentFun);
}

void FunctionModel::updateGlobals() {
  auto const fun = getCurrentFunction();
  for (auto it = m_globalParameterNames.begin(); it != m_globalParameterNames.end();) {
    if (!fun->hasParameter(*it)) {
      it = m_globalParameterNames.erase(it);
    } else {
      ++it;
    }
  }
}

void FunctionModel::setResolutionFromWorkspace(const IFunction_sptr &fun) {
  auto n = fun->getNumberDomains();
  if (n > 1) {
    for (size_t index = 0; index < n; index++) {
      setResolutionFromWorkspace(fun->getFunction(index));
    }
  } else {
    if (fun->hasAttribute("f0.Workspace")) {
      std::string wsName = fun->getAttribute("f0.Workspace").asString();
      if (AnalysisDataService::Instance().doesExist(wsName)) {
        const auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
        setResolutionFromWorkspace(fun, ws);
      }
    }
  }
}

void FunctionModel::setResolutionFromWorkspace(const IFunction_sptr &fun, const MatrixWorkspace_sptr &workspace) {
  auto inst = workspace->getInstrument();
  auto analyser = inst->getStringParameter("analyser");
  if (!analyser.empty()) {
    auto comp = inst->getComponentByName(analyser[0]);
    if (comp && comp->hasParameter("resolution")) {
      auto params = comp->getNumberParameter("resolution", true);
      for (const auto &param : fun->getParameterNames()) {
        if (param.find("FWHM") != std::string::npos) {
          fun->setParameter(param, params[0]);
        }
      }
    }
  }
}

bool FunctionModel::isGlobal(std::string const &parameterName) const {
  auto const findIter = std::find(m_globalParameterNames.cbegin(), m_globalParameterNames.cend(), parameterName);
  return findIter != m_globalParameterNames.cend();
}

std::string FunctionModel::setBackgroundA0(double value) {
  std::string foundName;
  auto const fun = getCurrentFunction();
  auto getA0 = [](IFunction const &f) -> std::string { return f.hasParameter("A0") ? "A0" : ""; };
  auto const cf = std::dynamic_pointer_cast<CompositeFunction>(fun);
  if (cf) {
    if (fun->name() != "CompositeFunction")
      return "";
    for (size_t i = 0; i < cf->nFunctions(); ++i) {
      foundName = getA0(*cf->getFunction(i));
      if (!foundName.empty()) {
        foundName.insert(0, "f" + std::to_string(i) + ".");
        break;
      }
    }
  } else {
    foundName = getA0(*fun);
  }
  if (!foundName.empty()) {
    fun->setParameter(foundName, value);
  }
  return foundName;
}

void FunctionModel::setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  (void)fitResolutions;
}

void FunctionModel::setQValues(const std::vector<double> &qValues) { (void)qValues; }

} // namespace MantidQt::MantidWidgets
