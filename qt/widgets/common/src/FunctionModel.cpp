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

namespace {

CompositeFunction_sptr initFunction(const QString & funStr, int nDomains) {
  if (funStr.isEmpty()) {
    return CompositeFunction_sptr();
  }
  auto fun = FunctionFactory::Instance().createInitialized(funStr.toStdString());
  if (nDomains == 0) {
    auto compositeFun = boost::dynamic_pointer_cast<CompositeFunction>(fun);
    if (compositeFun) {
      return compositeFun;
    }
    compositeFun = CompositeFunction_sptr(new CompositeFunction);
    compositeFun->addFunction(fun);
    return compositeFun;
  }
  auto multiDomainFun = MultiDomainFunction_sptr(new MultiDomainFunction);
  for (int i = 0; i < nDomains; ++i) {
    multiDomainFun->addFunction(fun->clone());
    multiDomainFun->setDomainIndex(i, i);
  }
  return multiDomainFun;
}

} // namespace
  
FunctionModel::FunctionModel(const QString & funStr, int nDomains) 
  : m_function(initFunction(funStr, nDomains)), m_currentDomainIndex(0)
{
}

bool FunctionModel::isMultiDomain() const
{
  return getNumberDomains() > 1;
}

int FunctionModel::getNumberDomains() const
{
  return m_function ? static_cast<int>(m_function->getNumberDomains()) : 0;
}

int FunctionModel::currentDomainIndex() const
{
  return static_cast<int>(m_currentDomainIndex);
}

void FunctionModel::setCurrentDomainIndex(int index)
{
  checkIndex(index);
  m_currentDomainIndex = static_cast<size_t>(index);
}

Mantid::API::IFunction_sptr FunctionModel::getSingleFunction(int index) const
{
  checkIndex(index);
  if (isMultiDomain()) {
    return m_function->getFunction(index);
  }
  if (!m_function || m_function->nFunctions() > 1) {
    return m_function;
  }
  if (m_function->nFunctions() == 1) {
    return m_function->getFunction(0);
  }
  return Mantid::API::IFunction_sptr();
}

Mantid::API::IFunction_sptr FunctionModel::getCurrentFunction() const
{
  return getSingleFunction(m_currentDomainIndex);
}

Mantid::API::IFunction_sptr FunctionModel::getGlobalFunction() const
{
  if (isMultiDomain()) {
    return m_function;
  }
  if (!m_function || m_function->nFunctions() > 1) {
    return m_function;
  }
  if (m_function->nFunctions() == 1) {
    return m_function->getFunction(0);
  }
  return Mantid::API::IFunction_sptr();
}

/// Check a domain/function index to be in range.
void FunctionModel::checkIndex(int index) const{
  if (index < 0 || index >= getNumberDomains()) {
    throw std::runtime_error("Domain index is out of range: " + std::to_string(index) + " out of " + std::to_string(getNumberDomains()));
  }
}

} // namespace API
} // namespace MantidQt
