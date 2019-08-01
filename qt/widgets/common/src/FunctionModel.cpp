// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionModel.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"

namespace {
Mantid::Kernel::Logger g_log("FitFunction");
}

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

void FunctionModel::setFunction(IFunction_sptr fun) {
  m_globalParameterNames.clear();
  m_function = boost::dynamic_pointer_cast<MultiDomainFunction>(fun);
  if (m_function) {
    return;
  }
  m_function = MultiDomainFunction_sptr(new MultiDomainFunction);
  if (fun) {
    auto const nf = m_numberDomains > 0 ? static_cast<int>(m_numberDomains) : 1;
    for (int i = 0; i < nf; ++i) {
      m_function->addFunction(fun->clone());
      m_function->setDomainIndex(i, i);
    }
  }
}

IFunction_sptr FunctionModel::getFitFunction() const {
  if (!m_function) {
    return m_function;
  }
  auto const nf = m_function->nFunctions();
  if (nf > 1) {
    auto fun =
        boost::dynamic_pointer_cast<MultiDomainFunction>(m_function->clone());
    auto const singleFun = m_function->getFunction(0);
    for (auto par = m_globalParameterNames.begin();
         par != m_globalParameterNames.end();) {
      if (singleFun->hasParameter(par->toStdString())) {
        QStringList ties;
        for (size_t i = 1; i < nf; ++i) {
          ties << "f" + QString::number(i) + "." + *par;
        }
        ties << "f0." + *par;
        fun->addTies(ties.join("=").toStdString());
        ++par;
      } else {
        par = m_globalParameterNames.erase(par);
      }
    }
    return fun;
  }
  if (nf == 1) {
    auto fun = m_function->getFunction(0);
    auto compFun = boost::dynamic_pointer_cast<CompositeFunction>(fun);
    if (compFun && compFun->nFunctions() == 1) {
      return compFun->getFunction(0);
    }
    return fun;
  }
  return IFunction_sptr();
}

bool FunctionModel::hasFunction() const {
  if (!m_function || m_function->nFunctions() == 0)
    return false;
  return true;
}

void FunctionModel::addFunction(const QString &prefix, const QString &funStr) {
  if (!hasFunction()) {
    setFunctionString(funStr);
    return;
  }
  auto newFun =
      FunctionFactory::Instance().createInitialized(funStr.toStdString());
  auto const nf = m_numberDomains > 0 ? static_cast<int>(m_numberDomains) : 1;
  for (int i = 0; i < nf; ++i) {
    auto fun = getSingleFunction(i);
    auto parentFun = getFunctionWithPrefix(prefix, fun);
    auto cf = boost::dynamic_pointer_cast<CompositeFunction>(parentFun);
    if (cf) {
      cf->addFunction(newFun->clone());
    } else if (i == 0 && prefix.isEmpty()) {
      setFunctionString(getFunctionString() + ";" + funStr);
      break;
    } else {
      throw std::runtime_error("Function at " + prefix.toStdString() +
                               " is not composite.");
    }
  }
  m_function->checkFunction();
  updateGlobals();
}

void FunctionModel::removeFunction(const QString &functionIndex) {
  QString prefix;
  int index;
  std::tie(prefix, index) = splitFunctionPrefix(functionIndex);
  if (prefix.isEmpty() && index == -1) {
    clear();
    return;
  }
  auto const nf = m_numberDomains > 0 ? static_cast<int>(m_numberDomains) : 1;
  for (int i = 0; i < nf; ++i) {
    auto fun = getSingleFunction(i);
    auto parentFun = getFunctionWithPrefix(prefix, fun);
    auto cf = boost::dynamic_pointer_cast<CompositeFunction>(parentFun);
    if (!cf) {
      throw std::runtime_error("Function at " + prefix.toStdString() +
                               " is not composite.");
    }
    cf->removeFunction(index);
    if (cf->nFunctions() == 1 && prefix.isEmpty() &&
        cf->name() == "CompositeFunction") {
      m_function->replaceFunction(i, cf->getFunction(0));
      m_function->checkFunction();
    } else {
      cf->checkFunction();
    }
  }
  updateGlobals();
}

void FunctionModel::setParameter(const QString &paramName, double value) {
  auto fun = getCurrentFunction();
  if (!fun) {
    throw std::logic_error("Function is undefined.");
  }
  fun->setParameter(paramName.toStdString(), value);
}

void FunctionModel::setParameterError(const QString &paramName, double value) {
  auto fun = getCurrentFunction();
  auto const index = fun->parameterIndex(paramName.toStdString());
  fun->setError(index, value);
}

double FunctionModel::getParameter(const QString &paramName) const {
  return getCurrentFunction()->getParameter(paramName.toStdString());
}

double FunctionModel::getParameterError(const QString &paramName) const {
  auto fun = getCurrentFunction();
  auto const index = fun->parameterIndex(paramName.toStdString());
  return fun->getError(index);
}

QString FunctionModel::getParameterDescription(const QString &paramName) const {
  auto fun = getCurrentFunction();
  auto const index = fun->parameterIndex(paramName.toStdString());
  return QString::fromStdString(fun->parameterDescription(index));
}

bool FunctionModel::isParameterFixed(const QString &parName) const {
  return isLocalParameterFixed(parName, static_cast<int>(m_currentDomainIndex));
}

QString FunctionModel::getParameterTie(const QString &parName) const {
  return getLocalParameterTie(parName, static_cast<int>(m_currentDomainIndex));
}

void FunctionModel::setParameterFixed(const QString &parName, bool fixed) {
  setLocalParameterFixed(parName, static_cast<int>(m_currentDomainIndex),
                         fixed);
}

void FunctionModel::setParameterTie(const QString &parName, QString tie) {
  setLocalParameterTie(parName, static_cast<int>(m_currentDomainIndex), tie);
}

QStringList FunctionModel::getParameterNames() const {
  QStringList names;
  const auto paramNames = getCurrentFunction()->getParameterNames();
  for (auto const name : paramNames) {
    names << QString::fromStdString(name);
  }
  return names;
}

IFunction_sptr FunctionModel::getSingleFunction(int index) const {
  checkIndex(index);
  if (!hasFunction()) {
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

void FunctionModel::setDatasetNames(const QStringList &names) {
  if (static_cast<size_t>(names.size()) != m_numberDomains) {
    throw std::runtime_error(
        "Number of dataset names doesn't match the number of domains.");
  }
  m_datasetNames = names;
}

QStringList FunctionModel::getDatasetNames() const {
  if (static_cast<size_t>(m_datasetNames.size()) != m_numberDomains) {
    m_datasetNames.clear();
    for (size_t i = 0; i < m_numberDomains; ++i) {
      m_datasetNames << QString::number(i);
    }
  }
  return m_datasetNames;
}

int FunctionModel::getNumberDomains() const {
  return static_cast<int>(m_numberDomains);
}

int FunctionModel::currentDomainIndex() const {
  return static_cast<int>(m_currentDomainIndex);
}

void FunctionModel::setCurrentDomainIndex(int index) {
  checkIndex(index);
  m_currentDomainIndex = static_cast<size_t>(index);
}

double FunctionModel::getLocalParameterValue(const QString &parName,
                                             int i) const {
  return getSingleFunction(i)->getParameter(parName.toStdString());
}

bool FunctionModel::isLocalParameterFixed(const QString &parName, int i) const {
  auto fun = getSingleFunction(i);
  auto const parIndex = fun->parameterIndex(parName.toStdString());
  return fun->isFixed(parIndex);
}

QString FunctionModel::getLocalParameterTie(const QString &parName,
                                            int i) const {
  auto fun = getSingleFunction(i);
  auto const parIndex = fun->parameterIndex(parName.toStdString());
  auto const tie = fun->getTie(parIndex);
  if (!tie)
    return "";
  auto const tieStr = QString::fromStdString(tie->asString());
  auto const j = tieStr.indexOf('=');
  return tieStr.mid(j + 1);
}

QString FunctionModel::getLocalParameterConstraint(const QString &parName,
                                                   int i) const {
  auto fun = getSingleFunction(i);
  auto const parIndex = fun->parameterIndex(parName.toStdString());
  auto const constraint = fun->getConstraint(parIndex);
  auto const out =
      (!constraint) ? "" : QString::fromStdString(constraint->asString());
  return out;
}

void FunctionModel::setLocalParameterValue(const QString &parName, int i,
                                           double value) {
  getSingleFunction(i)->setParameter(parName.toStdString(), value);
}

void FunctionModel::setLocalParameterValue(const QString &parName, int i,
                                           double value, double error) {
  auto fun = getSingleFunction(i);
  auto const parIndex = fun->parameterIndex(parName.toStdString());
  fun->setParameter(parIndex, value);
  fun->setError(parIndex, error);
}

void FunctionModel::setLocalParameterFixed(const QString &parName, int i,
                                           bool fixed) {
  auto fun = getSingleFunction(i);
  auto const parIndex = fun->parameterIndex(parName.toStdString());
  if (fixed) {
    fun->fix(parIndex);
  } else if (fun->isFixed(parIndex)) {
    fun->unfix(parIndex);
  }
}

void FunctionModel::setLocalParameterTie(const QString &parName, int i,
                                         const QString &tie) {
  auto logError = [&parName](const std::string &msg) {
    g_log.error() << "Tie " << parName.toStdString() << ": " << msg << '\n';
  };
  auto fun = getSingleFunction(i);
  auto const name = parName.toStdString();
  if (tie.isEmpty()) {
    fun->removeTie(fun->parameterIndex(name));
  } else {
    auto const j = tie.indexOf('=');
    try {
      fun->tie(name, tie.mid(j + 1).toStdString());
    } catch (const std::invalid_argument &e) {
      logError(e.what());
    } catch (const std::runtime_error &e) {
      logError(e.what());
    }
  }
}

void FunctionModel::setLocalParameterConstraint(const QString &parName, int i,
                                                const QString &constraint) {
  auto const parts = splitConstraintString(constraint);
  QString prefix, name;
  std::tie(prefix, name) = splitParameterName(parName);
  auto fun = getFunctionWithPrefix(prefix, getSingleFunction(i));
  if (constraint.isEmpty()) {
    fun->removeConstraint(name.toStdString());
  } else {
    auto newConstraint(constraint);
    newConstraint.replace(parts.first, name);
    fun->addConstraints(newConstraint.toStdString());
  }
}

void FunctionModel::changeTie(const QString &parName, const QString &tie) {
  try {
    setLocalParameterTie(parName, static_cast<int>(m_currentDomainIndex), tie);
  } catch (std::exception &) {
    // the tie is probably being edited
  }
}

void FunctionModel::addConstraint(const QString &functionIndex,
                                  const QString &constraint) {
  auto fun = getFunctionWithPrefix(functionIndex, getCurrentFunction());
  fun->addConstraints(constraint.toStdString());
}

void FunctionModel::removeConstraint(const QString &paramName) {
  getCurrentFunction()->removeConstraint(paramName.toStdString());
}

QStringList FunctionModel::getGlobalParameters() const {
  return m_globalParameterNames;
}

void FunctionModel::setGlobalParameters(const QStringList &globals) {
  m_globalParameterNames = globals;
}

QStringList FunctionModel::getLocalParameters() const {
  QStringList locals;
  for (auto const name : getParameterNames()) {
    if (!m_globalParameterNames.contains(name))
      locals << name;
  }
  return locals;
}

/// Check a domain/function index to be in range.
void FunctionModel::checkIndex(int index) const {
  if (index == 0)
    return;
  if (index < 0 || index >= getNumberDomains()) {
    throw std::runtime_error(
        "Domain index is out of range: " + std::to_string(index) + " out of " +
        std::to_string(getNumberDomains()));
  }
}

void FunctionModel::updateMultiDatasetParameters(const IFunction &fun) {
  if (!hasFunction())
    return;
  if (m_function->nParams() != fun.nParams())
    return;
  for (size_t i = 0; i < fun.nParams(); ++i) {
    m_function->setParameter(i, fun.getParameter(i));
    m_function->setError(i, fun.getError(i));
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
  for (auto it = m_globalParameterNames.begin();
       it != m_globalParameterNames.end();) {
    if (!fun->hasParameter(it->toStdString())) {
      it = m_globalParameterNames.erase(it);
    } else {
      ++it;
    }
  }
}

bool FunctionModel::isGlobal(const QString &parName) const {
  return m_globalParameterNames.contains(parName);
}

QString FunctionModel::setBackgroundA0(double value) {
  std::string foundName;
  auto const fun = getCurrentFunction();
  auto getA0 = [](IFunction const &f) -> std::string {
    return (dynamic_cast<IBackgroundFunction const *>(&f) &&
            f.hasParameter("A0"))
               ? "A0"
               : "";
  };
  auto const cf = boost::dynamic_pointer_cast<CompositeFunction>(fun);
  if (cf) {
    if (fun->name() != "CompositeFunction")
      return QString();
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
  return QString::fromStdString(foundName);
}

} // namespace MantidWidgets
} // namespace MantidQt
