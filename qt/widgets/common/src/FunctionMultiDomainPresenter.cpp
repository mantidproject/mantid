// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionMultiDomainPresenter.h"
#include "MantidQtWidgets/Common/FunctionTreeView.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/make_unique.h"

#include "FunctionBrowser/FunctionBrowserUtils.h"

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;
using namespace Mantid::Kernel;

FunctionMultiDomainPresenter::FunctionMultiDomainPresenter(IFunctionView *view)
  : m_view(view), m_model(make_unique<MultiDomainFunctionModel>())
{
  connect(m_view, SIGNAL(parameterChanged(const QString &)), this, SLOT(viewChangedParameter(const QString &)));
  connect(m_view, SIGNAL(functionReplaced(const QString &)), this, SLOT(viewPastedFunction(const QString &)));
}

void FunctionMultiDomainPresenter::setFunction(IFunction_sptr fun)
{
  m_model->setFunction(fun);
  m_view->setFunction(m_model->getCurrentFunction());
}

void FunctionMultiDomainPresenter::setFunctionString(const QString & funStr)
{
  m_model->setFunctionString(funStr);
  m_view->setFunction(m_model->getCurrentFunction());
  emit functionStructureChanged();
}

IFunction_sptr FunctionMultiDomainPresenter::getFitFunction() const
{
  return m_model->getFitFunction();
}

QString FunctionMultiDomainPresenter::getFitFunctionString() const {
  return m_model->getFitFunctionString();
}

bool FunctionMultiDomainPresenter::hasFunction() const
{
  return m_model->hasFunction();
}

IFunction_sptr FunctionMultiDomainPresenter::getFunctionByIndex(const QString & index)
{
  return getFunctionWithPrefix(index, m_model->getCurrentFunction());
}

void FunctionMultiDomainPresenter::setParameter(const QString & paramName, double value)
{
  m_model->setParameter(paramName, value);
  m_view->setParameter(paramName, value);
}

void FunctionMultiDomainPresenter::setParamError(const QString & paramName, double value)
{
  m_model->setParamError(paramName, value);
  m_view->setParamError(paramName, value);
}

double FunctionMultiDomainPresenter::getParameter(const QString & paramName)
{
  return m_model->getParameter(paramName);
}

void FunctionMultiDomainPresenter::updateParameters(const IFunction & fun)
{
  const auto paramNames = fun.getParameterNames();
  for (const auto &parameter : paramNames) {
    const QString qName = QString::fromStdString(parameter);
    setParameter(qName, fun.getParameter(parameter));
    const size_t index = fun.parameterIndex(parameter);
    setParamError(qName, fun.getError(index));
  }
}

void FunctionMultiDomainPresenter::clearErrors()
{
  m_view->clearErrors();
}

boost::optional<QString> FunctionMultiDomainPresenter::currentFunctionIndex() const
{
  return m_view->currentFunctionIndex();
}

void FunctionMultiDomainPresenter::setNumberOfDatasets(int n)
{
  m_model->setNumberDomains(n);
}

int FunctionMultiDomainPresenter::getNumberOfDatasets() const
{
  return m_model->getNumberDomains();
}

int FunctionMultiDomainPresenter::getCurrentDataset() const
{
  return m_model->currentDomainIndex();
}

void FunctionMultiDomainPresenter::setCurrentDataset(int index)
{
  m_model->setCurrentDomainIndex(index);
}

void FunctionMultiDomainPresenter::addDatasets(int)
{
}

void FunctionMultiDomainPresenter::removeDatasets(QList<int> indices)
{
}

void FunctionMultiDomainPresenter::viewPastedFunction(const QString & funStr)
{
  m_model->setFunctionString(funStr);
  emit functionStructureChanged();
}

QString FunctionMultiDomainPresenter::getFunctionString() const
{
  return m_model->getFunctionString();
}

IFunction_sptr FunctionMultiDomainPresenter::getFunction() const
{
  return m_model->getCurrentFunction();
}

void FunctionMultiDomainPresenter::clear()
{
  m_model->clear();
  m_view->clear();
}

void FunctionMultiDomainPresenter::setColumnSizes(int s0, int s1, int s2)
{
  auto treeView = dynamic_cast<FunctionTreeView*>(m_view);
  if (treeView)
    treeView->setColumnSizes(s0, s1, s2);
}

void FunctionMultiDomainPresenter::setErrorsEnabled(bool enabled)
{
  m_view->setErrorsEnabled(enabled);
}

void FunctionMultiDomainPresenter::viewChangedParameter(const QString &paramName) {
  auto const value = m_view->getParameter(paramName);
  m_model->setParameter(paramName, value);
  auto const parts = splitParameterName(paramName);
  emit parameterChanged(parts.first, parts.second);
}

} // namespace API
} // namespace MantidQt
