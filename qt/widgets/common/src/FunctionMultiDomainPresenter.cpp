// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionMultiDomainPresenter.h"
#include "MantidAPI/IFunction.h"

#include "MantidQtWidgets/Common/FunctionTreeView.h"

#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QWidget>
#include <iostream>

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;
using namespace Mantid::Kernel;

FunctionMultiDomainPresenter::FunctionMultiDomainPresenter(IFunctionView *view)
    : m_view(view), m_model(std::make_unique<MultiDomainFunctionModel>()),
      m_editLocalParameterDialog(nullptr) {
  connect(m_view, SIGNAL(parameterChanged(const QString &)), this,
          SLOT(viewChangedParameter(const QString &)));
  connect(m_view, SIGNAL(functionReplaced(const QString &)), this,
          SLOT(viewPastedFunction(const QString &)));
  connect(m_view, SIGNAL(functionAdded(const QString &)), this,
          SLOT(viewAddedFunction(const QString &)));
  connect(m_view, SIGNAL(functionRemoved(const QString &)), this,
          SLOT(viewRemovedFunction(const QString &)));
  connect(m_view, SIGNAL(parameterTieChanged(const QString &, const QString &)),
          this, SLOT(viewChangedTie(const QString &, const QString &)));
  connect(m_view,
          SIGNAL(parameterConstraintAdded(const QString &, const QString &)),
          this, SLOT(viewAddedConstraint(const QString &, const QString &)));
  connect(m_view, SIGNAL(parameterConstraintRemoved(const QString &)), this,
          SLOT(viewRemovedConstraint(const QString &)));
  connect(m_view, SIGNAL(localParameterButtonClicked(const QString &)), this,
          SLOT(editLocalParameter(const QString &)));
  connect(m_view, SIGNAL(copyToClipboardRequest()), this,
          SLOT(viewRequestedCopyToClipboard()));
  connect(m_view, SIGNAL(globalsChanged(const QStringList &)), this,
          SLOT(viewChangedGlobals(const QStringList &)));
}

void FunctionMultiDomainPresenter::setFunction(IFunction_sptr fun) {
  m_model->setFunction(fun);
  m_view->setFunction(m_model->getCurrentFunction());
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::setFunctionString(const QString &funStr) {
  m_model->setFunctionString(funStr);
  m_view->setFunction(m_model->getCurrentFunction());
  emit functionStructureChanged();
}

IFunction_sptr FunctionMultiDomainPresenter::getFitFunction() const {
  return m_model->getFitFunction();
}

QString FunctionMultiDomainPresenter::getFitFunctionString() const {
  return m_model->getFitFunctionString();
}

bool FunctionMultiDomainPresenter::hasFunction() const {
  return m_model->hasFunction();
}

IFunction_sptr
FunctionMultiDomainPresenter::getFunctionByIndex(const QString &index) {
  return getFunctionWithPrefix(index, m_model->getCurrentFunction());
}

void FunctionMultiDomainPresenter::setParameter(const QString &paramName,
                                                double value) {
  m_model->setParameter(paramName, value);
  m_view->setParameter(paramName, value);
}

void FunctionMultiDomainPresenter::setParamError(const QString &paramName,
                                                 double value) {
  m_model->setParamError(paramName, value);
  m_view->setParamError(paramName, value);
}

double FunctionMultiDomainPresenter::getParameter(const QString &paramName) {
  return m_model->getParameter(paramName);
}

bool FunctionMultiDomainPresenter::isParameterFixed(
    const QString &parName) const {
  return m_model->isParameterFixed(parName);
}

QString
FunctionMultiDomainPresenter::getParameterTie(const QString &parName) const {
  return m_model->getParameterTie(parName);
}

void FunctionMultiDomainPresenter::updateParameters(const IFunction &fun) {
  const auto paramNames = fun.getParameterNames();
  for (const auto &parameter : paramNames) {
    const QString qName = QString::fromStdString(parameter);
    setParameter(qName, fun.getParameter(parameter));
    const size_t index = fun.parameterIndex(parameter);
    setParamError(qName, fun.getError(index));
  }
}

void FunctionMultiDomainPresenter::updateMultiDatasetParameters(
    const IFunction &fun) {
  m_model->updateMultiDatasetParameters(fun);
  auto currentFun = m_model->getCurrentFunction();
  const auto paramNames = currentFun->getParameterNames();
  for (const auto &parameter : paramNames) {
    const QString qName = QString::fromStdString(parameter);
    m_view->setParameter(qName, currentFun->getParameter(parameter));
    const size_t index = currentFun->parameterIndex(parameter);
    m_view->setParamError(qName, currentFun->getError(index));
  }
}

void FunctionMultiDomainPresenter::clearErrors() { m_view->clearErrors(); }

boost::optional<QString>
FunctionMultiDomainPresenter::currentFunctionIndex() const {
  return m_view->currentFunctionIndex();
}

void FunctionMultiDomainPresenter::setNumberOfDatasets(int n) {
  m_model->setNumberDomains(n);
}

void FunctionMultiDomainPresenter::setDatasetNames(const QStringList &names) {
  m_model->setDatasetNames(names);
}

QStringList FunctionMultiDomainPresenter::getDatasetNames() const {
  return m_model->getDatasetNames();
}

int FunctionMultiDomainPresenter::getNumberOfDatasets() const {
  return m_model->getNumberDomains();
}

int FunctionMultiDomainPresenter::getCurrentDataset() const {
  return m_model->currentDomainIndex();
}

void FunctionMultiDomainPresenter::setCurrentDataset(int index) {
  if (!m_model->hasFunction())
    return;
  m_model->setCurrentDomainIndex(index);
  for (auto const name : m_model->getParameterNames()) {
    auto const value = m_model->getParameter(name);
    m_view->setParameter(name, value);
    m_view->setParamError(name, m_model->getParamError(name));
    if (m_model->isLocalParameterFixed(name, index)) {
      m_view->setParameterTie(name, QString::number(value));
    } else {
      m_view->setParameterTie(name, m_model->getLocalParameterTie(name, index));
    }
  }
}

void FunctionMultiDomainPresenter::removeDatasets(QList<int> indices) {
  auto datasetNames = getDatasetNames();
  // Sort in reverse order
  qSort(indices.begin(), indices.end(), [](int a, int b) { return a > b; });
  for (auto i = indices.constBegin(); i != indices.constEnd(); ++i) {
    datasetNames.removeAt(*i);
  }
  m_model->setNumberDomains(datasetNames.size());
  m_model->setDatasetNames(datasetNames);
  auto currentIndex = m_model->currentDomainIndex();
  if (currentIndex >= datasetNames.size()) {
    currentIndex = datasetNames.isEmpty() ? 0 : datasetNames.size() - 1;
  }
  setCurrentDataset(currentIndex);
}

double
FunctionMultiDomainPresenter::getLocalParameterValue(const QString &parName,
                                                     int i) const {
  return m_model->getLocalParameterValue(parName, i);
}

bool FunctionMultiDomainPresenter::isLocalParameterFixed(const QString &parName,
                                                         int i) const {
  return m_model->isLocalParameterFixed(parName, i);
}

QString
FunctionMultiDomainPresenter::getLocalParameterTie(const QString &parName,
                                                   int i) const {
  return m_model->getLocalParameterTie(parName, i);
}

void FunctionMultiDomainPresenter::setLocalParameterValue(
    const QString &parName, int i, double value) {
  m_model->setLocalParameterValue(parName, i, value);
  if (m_model->currentDomainIndex() == i) {
    m_view->setParameter(parName, value);
  }
}

void FunctionMultiDomainPresenter::setLocalParameterValue(
    const QString &parName, int i, double value, double error) {
  m_model->setLocalParameterValue(parName, i, value, error);
  if (m_model->currentDomainIndex() == i) {
    m_view->setParameter(parName, value);
    m_view->setParamError(parName, error);
  }
}

void FunctionMultiDomainPresenter::setLocalParameterFixed(
    const QString &parName, int i, bool fixed) {
  m_model->setLocalParameterFixed(parName, i, fixed);
  if (m_model->currentDomainIndex() == i) {
    if (fixed) {
      m_view->setParameterTie(parName,
                              QString::number(m_model->getParameter(parName)));
    } else {
      m_view->setParameterTie(parName, "");
    }
  }
}

void FunctionMultiDomainPresenter::setLocalParameterTie(const QString &parName,
                                                        int i, QString tie) {
  m_model->setLocalParameterTie(parName, i, tie);
  if (m_model->currentDomainIndex() == i) {
    m_view->setParameterTie(parName, tie);
  }
}

QStringList FunctionMultiDomainPresenter::getGlobalParameters() const {
  return m_model->getGlobalParameters();
}

void FunctionMultiDomainPresenter::setGlobalParameters(
    const QStringList &globals) {
  m_model->setGlobalParameters(globals);
  m_view->setGlobalParameters(m_model->getGlobalParameters());
}

QStringList FunctionMultiDomainPresenter::getLocalParameters() const {
  return m_model->getLocalParameters();
}

void FunctionMultiDomainPresenter::viewPastedFunction(const QString &funStr) {
  m_model->setFunctionString(funStr);
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::viewAddedFunction(const QString &funStr) {
  auto const prefix = m_view->currentFunctionIndex();
  auto const prefixValue = prefix ? *prefix : "";
  m_model->addFunction(prefixValue, funStr);
  m_view->setGlobalParameters(m_model->getGlobalParameters());
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::viewRemovedFunction(
    const QString &functionIndex) {
  m_model->removeFunction(functionIndex);
  m_view->setGlobalParameters(m_model->getGlobalParameters());
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::viewChangedTie(const QString &paramName,
                                                  const QString &tie) {
  m_model->changeTie(paramName, tie);
}

void FunctionMultiDomainPresenter::viewAddedConstraint(
    const QString &functionIndex, const QString &constraint) {
  m_model->addConstraint(functionIndex, constraint);
}

void FunctionMultiDomainPresenter::viewRemovedConstraint(
    const QString &parName) {
  m_model->removeConstraint(parName);
}

void FunctionMultiDomainPresenter::viewRequestedCopyToClipboard() {
  auto fun = getFunction();
  if (fun) {
    QApplication::clipboard()->setText(QString::fromStdString(fun->asString()));
  }
}

void FunctionMultiDomainPresenter::viewChangedGlobals(
    const QStringList &globalParameters) {
  m_model->setGlobalParameters(globalParameters);
}

QString FunctionMultiDomainPresenter::getFunctionString() const {
  return m_model->getFunctionString();
}

IFunction_sptr FunctionMultiDomainPresenter::getFunction() const {
  return m_model->getCurrentFunction();
}

void FunctionMultiDomainPresenter::clear() {
  m_model->clear();
  m_view->clear();
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::setColumnSizes(int s0, int s1, int s2) {
  auto treeView = dynamic_cast<FunctionTreeView *>(m_view);
  if (treeView)
    treeView->setColumnSizes(s0, s1, s2);
}

void FunctionMultiDomainPresenter::setErrorsEnabled(bool enabled) {
  m_view->setErrorsEnabled(enabled);
}

void FunctionMultiDomainPresenter::viewChangedParameter(
    const QString &paramName) {
  auto const value = m_view->getParameter(paramName);
  m_model->setParameter(paramName, value);
  auto const parts = splitParameterName(paramName);
  emit parameterChanged(parts.first, parts.second);
}

/**
 * Launches the Edit Local Parameter dialog and deals with the input from it.
 * @param parName :: Name of parameter that button was clicked for.
 */
void FunctionMultiDomainPresenter::editLocalParameter(const QString &parName) {
  m_editLocalParameterDialog = new EditLocalParameterDialog(
      m_view, this, parName, m_model->getDatasetNames());
  connect(m_editLocalParameterDialog, SIGNAL(finished(int)), this,
          SLOT(editLocalParameterFinish(int)));
  m_editLocalParameterDialog->open();
}

void FunctionMultiDomainPresenter::editLocalParameterFinish(int result) {
  if (result == QDialog::Accepted) {
    auto parName = m_editLocalParameterDialog->getParameterName();
    auto values = m_editLocalParameterDialog->getValues();
    auto fixes = m_editLocalParameterDialog->getFixes();
    auto ties = m_editLocalParameterDialog->getTies();
    assert(values.size() == getNumberOfDatasets());
    for (int i = 0; i < values.size(); ++i) {
      setLocalParameterValue(parName, i, values[i]);
      if (!ties[i].isEmpty()) {
        setLocalParameterTie(parName, i, ties[i]);
      } else if (fixes[i]) {
        setLocalParameterFixed(parName, i, fixes[i]);
      } else {
        setLocalParameterTie(parName, i, "");
      }
    }
  }
  m_editLocalParameterDialog = nullptr;
}

} // namespace MantidWidgets
} // namespace MantidQt
