// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionModel.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

void IFunctionModel::setFunctionString(const QString & funStr)
{
  setFunction(FunctionFactory::Instance().createInitialized(funStr.toStdString()));
}

QString IFunctionModel::getFunctionString() const
{
  auto fun = getCurrentFunction();
  if (!fun)
    return "";
  return QString::fromStdString(fun->asString());
}

QString IFunctionModel::getFitFunctionString() const
{
  auto fun = getFitFunction();
  if (!fun)
    return "";
  return QString::fromStdString(fun->asString());
}

void IFunctionModel::clear()
{
  setFunction(IFunction_sptr());
}

void MultiDomainFunctionModel::setFunction(IFunction_sptr fun) {
  m_function = boost::dynamic_pointer_cast<MultiDomainFunction>(fun);
  if (m_function) {
    return;
  }
  m_function = MultiDomainFunction_sptr(new MultiDomainFunction);
  if (fun) {
    for (int i = 0; i < m_numberDomains; ++i) {
      m_function->addFunction(fun);
    }
  }
}

IFunction_sptr MultiDomainFunctionModel::getFitFunction() const
{
  if (!m_function || m_function->nFunctions() > 1) {
    return m_function;
  }
  if (m_function->nFunctions() == 1) {
    auto fun = m_function->getFunction(0);
    auto compFun = boost::dynamic_pointer_cast<CompositeFunction>(fun);
    if (compFun && compFun->nFunctions() == 1) {
      return compFun->getFunction(0);
    }
    return fun;
  }
  return IFunction_sptr();
}

bool MultiDomainFunctionModel::hasFunction() const
{
  if (!m_function || m_function->nFunctions() == 0) return false;
  return true;
}

void MultiDomainFunctionModel::setParameter(const QString & paramName, double value)
{
  getCurrentFunction()->setParameter(paramName.toStdString(), value);
}

void MultiDomainFunctionModel::setParamError(const QString & paramName, double value)
{
  auto fun = getCurrentFunction();
  auto const index = fun->parameterIndex(paramName.toStdString());
  fun->setError(index, value);
}

double MultiDomainFunctionModel::getParameter(const QString & paramName) const
{
  return getCurrentFunction()->getParameter(paramName.toStdString());
}

double MultiDomainFunctionModel::getParamError(const QString & paramName) const
{
  auto fun = getCurrentFunction();
  auto const index = fun->parameterIndex(paramName.toStdString());
  return fun->getError(index);
}

IFunction_sptr MultiDomainFunctionModel::getSingleFunction(int index) const
{
  checkIndex(index);
  return m_function->getFunction(index);
}

IFunction_sptr MultiDomainFunctionModel::getCurrentFunction() const
{
  return getSingleFunction(static_cast<int>(m_currentDomainIndex));
}

void MultiDomainFunctionModel::setNumberDomains(int nDomains)
{
  auto const nd = static_cast<size_t>(nDomains);
  if (nd < 1) {
    throw std::runtime_error("Number of domains shouldn't be less than 1.");
  }
  if (nd == m_numberDomains) {
    return;
  }
  if (!m_function) {
    m_numberDomains = nd;
    return;
  }
  auto const lastIndex = m_numberDomains - 1;
  if (nd > m_numberDomains) {
    auto fun = m_function->getFunction(lastIndex);
    for (size_t i = m_numberDomains; i < nd; ++i) {
      m_function->addFunction(fun->clone());
      m_function->setDomainIndex(i, i);
    }
  } else {
    for (size_t i = lastIndex; i >= nd; --i) {
      m_function->removeFunction(i);
    }
  }
  m_numberDomains = nDomains;
}

int MultiDomainFunctionModel::getNumberDomains() const
{
  return static_cast<int>(m_numberDomains);
}

int MultiDomainFunctionModel::currentDomainIndex() const
{
  return static_cast<int>(m_currentDomainIndex);
}

void MultiDomainFunctionModel::setCurrentDomainIndex(int index)
{
  checkIndex(index);
  m_currentDomainIndex = static_cast<size_t>(index);
}

double MultiDomainFunctionModel::getLocalParameterValue(const QString & parName, int i) const
{
  return getSingleFunction(i)->getParameter(parName.toStdString());
}

bool MultiDomainFunctionModel::isLocalParameterFixed(const QString & parName, int i) const
{
  auto fun = getSingleFunction(i);
  auto const parIndex = fun->parameterIndex(parName.toStdString());
  return fun->isFixed(parIndex);
}

QString MultiDomainFunctionModel::getLocalParameterTie(const QString & parName, int i) const
{
  auto fun = getSingleFunction(i);
  auto const parIndex = fun->parameterIndex(parName.toStdString());
  auto const tie = fun->getTie(parIndex);
  return QString::fromStdString(tie->asString());
}

void MultiDomainFunctionModel::setLocalParameterValue(const QString & parName, int i, double value)
{
  getSingleFunction(i)->setParameter(parName.toStdString(), value);
}

void MultiDomainFunctionModel::setLocalParameterFixed(const QString & parName, int i, bool fixed)
{
  auto fun = getSingleFunction(i);
  auto const parIndex = fun->parameterIndex(parName.toStdString());
  getSingleFunction(i)->fix(parIndex);
}

void MultiDomainFunctionModel::setLocalParameterTie(const QString & parName, int i, QString tie)
{
  getSingleFunction(i)->tie(parName.toStdString(), tie.toStdString());
}

/// Check a domain/function index to be in range.
void MultiDomainFunctionModel::checkIndex(int index) const {
  if (index < 0 || index >= getNumberDomains()) {
    throw std::runtime_error("Domain index is out of range: " + std::to_string(index) + " out of " + std::to_string(getNumberDomains()));
  }
}

} // namespace API
} // namespace MantidQt
