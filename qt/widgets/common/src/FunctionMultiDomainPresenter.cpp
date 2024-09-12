// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionMultiDomainPresenter.h"
#include "MantidAPI/IFunction.h"
#include "MantidQtWidgets/Common/FunctionTreeView.h"

#include "MantidQtWidgets/Common/FunctionTreeView.h"

#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QWidget>
#include <utility>

namespace {
Mantid::Kernel::Logger g_log("FunctionMultiDomain");
}

namespace MantidQt::MantidWidgets {

using namespace Mantid::API;
using namespace Mantid::Kernel;

FunctionMultiDomainPresenter::FunctionMultiDomainPresenter(IFunctionView *view)
    : m_view(view), m_model(std::make_unique<FunctionModel>()), m_editLocalParameterDialog(nullptr) {
  connect(m_view, SIGNAL(parameterChanged(std::string const &)), this, SLOT(viewChangedParameter(std::string const &)));
  connect(m_view, SIGNAL(functionReplaced(std::string const &)), this, SLOT(viewPastedFunction(std::string const &)));
  connect(m_view, SIGNAL(functionAdded(std::string const &)), this, SLOT(viewAddedFunction(std::string const &)));
  connect(m_view, SIGNAL(functionRemoved(std::string const &)), this, SLOT(viewRemovedFunction(std::string const &)));
  connect(m_view, SIGNAL(parameterTieChanged(std::string const &, std::string const &)), this,
          SLOT(viewChangedTie(std::string const &, std::string const &)));
  connect(m_view, SIGNAL(parameterConstraintAdded(std::string const &, std::string const &)), this,
          SLOT(viewAddedConstraint(std::string const &, std::string const &)));
  connect(m_view, SIGNAL(parameterConstraintRemoved(std::string const &)), this,
          SLOT(viewRemovedConstraint(std::string const &)));
  connect(m_view, SIGNAL(localParameterButtonClicked(std::string const &)), this,
          SLOT(editLocalParameter(std::string const &)));
  connect(m_view, SIGNAL(copyToClipboardRequest()), this, SLOT(viewRequestedCopyToClipboard()));
  connect(m_view, SIGNAL(globalsChanged(const std::vector<std::string> &)), this,
          SLOT(viewChangedGlobals(const std::vector<std::string> &)));
  connect(m_view, SIGNAL(functionHelpRequest()), this, SLOT(viewRequestedFunctionHelp()));
  connect(m_view, SIGNAL(attributePropertyChanged(std::string const &)), this,
          SLOT(viewChangedAttribute(std::string const &)));
}

void FunctionMultiDomainPresenter::setFunction(IFunction_sptr fun) {
  m_model->setFunction(std::move(fun));
  m_view->setFunction(m_model->getCurrentFunction());
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::setFunctionString(std::string const &funStr) {
  m_model->setFunctionString(funStr);
  m_view->setFunction(m_model->getCurrentFunction());
  emit functionStructureChanged();
}

IFunction_sptr FunctionMultiDomainPresenter::getFitFunction() const { return m_model->getFitFunction(); }

std::string FunctionMultiDomainPresenter::getFitFunctionString() const { return m_model->getFitFunctionString(); }

bool FunctionMultiDomainPresenter::hasFunction() const { return m_model->hasFunction(); }

IFunction_sptr FunctionMultiDomainPresenter::getFunctionByIndex(std::string const &index) {
  return getFunctionWithPrefix(index, m_model->getCurrentFunction());
}

void FunctionMultiDomainPresenter::setParameter(std::string const &parameterName, double value) {
  m_model->setParameter(parameterName, value);
  m_view->setParameter(parameterName, value);
}

void FunctionMultiDomainPresenter::setParameterError(std::string const &parameterName, double value) {
  m_model->setParameterError(parameterName, value);
  m_view->setParameterError(parameterName, value);
}

double FunctionMultiDomainPresenter::getParameter(std::string const &parameterName) {
  return m_model->getParameter(parameterName);
}

bool FunctionMultiDomainPresenter::isParameterFixed(std::string const &parameterName) const {
  return m_model->isParameterFixed(parameterName);
}

std::string FunctionMultiDomainPresenter::getParameterTie(std::string const &parameterName) const {
  return m_model->getParameterTie(parameterName);
}

void FunctionMultiDomainPresenter::updateParameters(const IFunction &fun) {
  m_model->updateParameters(fun);
  updateViewFromModel();
}

void FunctionMultiDomainPresenter::updateMultiDatasetParameters(const IFunction &fun) {
  m_model->updateMultiDatasetParameters(fun);
  updateViewFromModel();
}

void FunctionMultiDomainPresenter::updateMultiDatasetAttributes(const IFunction &fun) {
  m_model->updateMultiDatasetAttributes(fun);
  updateViewFromModel();
}

void FunctionMultiDomainPresenter::clearErrors() { m_view->clearErrors(); }

std::optional<std::string> FunctionMultiDomainPresenter::currentFunctionIndex() const {
  return m_view->currentFunctionIndex();
}

void FunctionMultiDomainPresenter::setNumberOfDatasets(int n) { m_model->setNumberDomains(n); }

void FunctionMultiDomainPresenter::setDatasets(const std::vector<std::string> &datasetNames) {
  m_model->setDatasets(datasetNames);
}

void FunctionMultiDomainPresenter::setDatasets(const QList<FunctionModelDataset> &datasets) {
  m_model->setDatasets(datasets);
}

void FunctionMultiDomainPresenter::addDatasets(const std::vector<std::string> &datasetNames) {
  m_model->addDatasets(datasetNames);
}

std::vector<std::string> FunctionMultiDomainPresenter::getDatasetNames() const { return m_model->getDatasetNames(); }

std::vector<std::string> FunctionMultiDomainPresenter::getDatasetDomainNames() const {
  return m_model->getDatasetDomainNames();
}

int FunctionMultiDomainPresenter::getNumberOfDatasets() const { return m_model->getNumberDomains(); }

int FunctionMultiDomainPresenter::getCurrentDataset() const { return m_model->currentDomainIndex(); }

void FunctionMultiDomainPresenter::setCurrentDataset(int index) {
  m_model->setCurrentDomainIndex(index);
  updateViewFromModel();
}

void FunctionMultiDomainPresenter::removeDatasets(QList<int> indices) {
  m_model->removeDatasets(indices);
  setCurrentDataset(m_model->currentDomainIndex());
}

double FunctionMultiDomainPresenter::getLocalParameterValue(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterValue(parameterName, i);
}

bool FunctionMultiDomainPresenter::isLocalParameterFixed(std::string const &parameterName, int i) const {
  return m_model->isLocalParameterFixed(parameterName, i);
}

std::string FunctionMultiDomainPresenter::getLocalParameterTie(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterTie(parameterName, i);
}

std::string FunctionMultiDomainPresenter::getLocalParameterConstraint(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterConstraint(parameterName, i);
}

void FunctionMultiDomainPresenter::setLocalParameterValue(std::string const &parameterName, int i, double value) {
  m_model->setLocalParameterValue(parameterName, i, value);
  if (m_model->currentDomainIndex() == i) {
    m_view->setParameter(parameterName, value);
  }
}

void FunctionMultiDomainPresenter::setLocalParameterValue(std::string const &parameterName, int i, double value,
                                                          double error) {
  m_model->setLocalParameterValue(parameterName, i, value, error);
  if (m_model->currentDomainIndex() == i) {
    m_view->setParameter(parameterName, value);
    m_view->setParameterError(parameterName, error);
  }
}

void FunctionMultiDomainPresenter::setLocalParameterFixed(std::string const &parameterName, int i, bool fixed) {
  m_model->setLocalParameterFixed(parameterName, i, fixed);
  if (m_model->currentDomainIndex() == i) {
    if (fixed) {
      m_view->setParameterTie(parameterName, QString::number(m_model->getParameter(parameterName)).toStdString());
    } else {
      m_view->setParameterTie(parameterName, "");
    }
  }
}

void FunctionMultiDomainPresenter::setLocalParameterTie(std::string const &parameterName, int i,
                                                        std::string const &tie) {
  m_model->setLocalParameterTie(parameterName, i, tie);
  if (m_model->currentDomainIndex() == i) {
    m_view->setParameterTie(parameterName, tie);
  }
}

void FunctionMultiDomainPresenter::setLocalParameterConstraint(std::string const &parameterName, int i,
                                                               std::string const &constraint) {
  m_model->setLocalParameterConstraint(parameterName, i, constraint);
  if (m_model->currentDomainIndex() == i) {
    m_view->setParameterConstraint(parameterName, constraint);
  }
}

std::vector<std::string> FunctionMultiDomainPresenter::getGlobalParameters() const {
  return m_model->getGlobalParameters();
}

void FunctionMultiDomainPresenter::setGlobalParameters(std::vector<std::string> const &globals) {
  m_model->setGlobalParameters(globals);
  m_view->setGlobalParameters(m_model->getGlobalParameters());
}

std::vector<std::string> FunctionMultiDomainPresenter::getLocalParameters() const {
  return m_model->getLocalParameters();
}

void FunctionMultiDomainPresenter::setBackgroundA0(double value) {
  auto const paramName = m_model->setBackgroundA0(value);
  if (!paramName.empty())
    m_view->setParameter(paramName, value);
}

void FunctionMultiDomainPresenter::viewPastedFunction(std::string const &funStr) {
  m_model->setFunctionString(funStr);
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::viewAddedFunction(std::string const &funStr) {
  auto const prefix = m_view->currentFunctionIndex();
  auto const prefixValue = prefix ? *prefix : "";
  m_model->addFunction(prefixValue, funStr);
  m_view->setGlobalParameters(m_model->getGlobalParameters());
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::viewRemovedFunction(std::string const &functionIndex) {
  m_model->removeFunction(functionIndex);
  m_view->setGlobalParameters(m_model->getGlobalParameters());
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::viewChangedTie(std::string const &parameterName, std::string const &tie) {
  m_model->changeTie(parameterName, tie);
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::viewAddedConstraint(std::string const &functionIndex,
                                                       std::string const &constraint) {
  m_model->addConstraint(functionIndex, constraint);
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::viewRemovedConstraint(std::string const &parameterName) {
  m_model->removeConstraint(parameterName);
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::viewRequestedCopyToClipboard() {
  auto fun = getFunction();
  if (fun) {
    QApplication::clipboard()->setText(QString::fromStdString(fun->asString()));
  }
}

void FunctionMultiDomainPresenter::viewChangedGlobals(const std::vector<std::string> &globalParameters) {
  m_model->setGlobalParameters(globalParameters);
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::viewRequestedFunctionHelp() {
  auto func = m_view->getSelectedFunction();
  if (func)
    m_view->showFunctionHelp(func->name());
}

std::string FunctionMultiDomainPresenter::getFunctionString() const { return m_model->getFunctionString(); }

IFunction_sptr FunctionMultiDomainPresenter::getFunction() const { return m_model->getCurrentFunction(); }

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

void FunctionMultiDomainPresenter::setStretchLastColumn(bool stretch) {
  if (auto treeView = dynamic_cast<FunctionTreeView *>(m_view))
    treeView->setStretchLastColumn(stretch);
}

void FunctionMultiDomainPresenter::setErrorsEnabled(bool enabled) { m_view->setErrorsEnabled(enabled); }

void FunctionMultiDomainPresenter::viewChangedParameter(std::string const &parameterName) {
  auto const value = m_view->getParameter(parameterName);
  m_model->setParameter(parameterName, value);
  auto const parts = splitParameterName(parameterName);
  emit parameterChanged(parts.first, parts.second);
}

void FunctionMultiDomainPresenter::viewChangedAttribute(std::string const &attrName) {
  try {
    auto value = m_view->getAttribute(attrName);
    m_model->setAttribute(attrName, value);
    emit attributeChanged(attrName);
  } catch (const std::invalid_argument &e) {
    updateViewAttributesFromModel();
    g_log.error(e.what());
  }
}

/**
 * Launches the Edit Local Parameter dialog and deals with the input from it.
 * @param parameterName :: Name of parameter that button was clicked for.
 */
void FunctionMultiDomainPresenter::editLocalParameter(std::string const &parameterName) {
  auto const datasetNames = getDatasetNames();
  auto const domainNames = getDatasetDomainNames();
  QList<double> values;
  QList<bool> fixes;
  QStringList ties;
  QStringList constraints;
  for (int i = 0; i < static_cast<int>(domainNames.size()); ++i) {
    const double value = getLocalParameterValue(parameterName, i);
    values.push_back(value);
    const bool fixed = isLocalParameterFixed(parameterName, i);
    fixes.push_back(fixed);
    const auto tie = getLocalParameterTie(parameterName, i);
    ties.push_back(QString::fromStdString(tie));
    const auto constraint = getLocalParameterConstraint(parameterName, i);
    constraints.push_back(QString::fromStdString(constraint));
  }

  m_editLocalParameterDialog =
      new EditLocalParameterDialog(m_view, parameterName, datasetNames, domainNames, values, fixes, ties, constraints);
  connect(m_editLocalParameterDialog, SIGNAL(finished(int)), this, SLOT(editLocalParameterFinish(int)));
  m_editLocalParameterDialog->open();
}

void FunctionMultiDomainPresenter::editLocalParameterFinish(int result) {
  if (result == QDialog::Accepted) {
    auto parName = m_editLocalParameterDialog->getParameterName();
    auto values = m_editLocalParameterDialog->getValues();
    auto fixes = m_editLocalParameterDialog->getFixes();
    auto ties = m_editLocalParameterDialog->getTies();
    auto constraints = m_editLocalParameterDialog->getConstraints();
    assert(values.size() == getNumberOfDatasets());
    for (int i = 0; i < values.size(); ++i) {
      setLocalParameterValue(parName, i, values[i]);
      if (!ties[i].isEmpty()) {
        setLocalParameterTie(parName, i, ties[i].toStdString());
      } else if (fixes[i]) {
        setLocalParameterFixed(parName, i, fixes[i]);
      } else {
        setLocalParameterTie(parName, i, "");
      }
      setLocalParameterConstraint(parName, i, constraints[i].toStdString());
    }
  }
  m_editLocalParameterDialog = nullptr;
  emit functionStructureChanged();
}

void FunctionMultiDomainPresenter::updateViewFromModel() {
  const auto index = m_model->currentDomainIndex();
  for (auto const &name : m_model->getParameterNames()) {
    auto const value = m_model->getParameter(name);
    m_view->setParameter(name, value);
    m_view->setParameterError(name, m_model->getParameterError(name));
    if (m_model->isLocalParameterFixed(name, index)) {
      m_view->setParameterTie(name, QString::number(value).toStdString());
    } else {
      m_view->setParameterTie(name, m_model->getLocalParameterTie(name, index));
    }
    m_view->setParameterConstraint(name, m_model->getLocalParameterConstraint(name, index));
  }
  updateViewAttributesFromModel();
}

void FunctionMultiDomainPresenter::updateViewAttributesFromModel() {
  for (const auto &name : m_model->getAttributeNames()) {
    // Create a single lambda expression capable of visiting each attribute
    auto visitAttribute = [&](auto val) { m_view->setAttributeValue(name, val); };
    auto value = m_model->getAttribute(name);
    auto visitor = AttributeLambdaVisitor{visitAttribute};
    value.apply(visitor);
  }
}

void FunctionMultiDomainPresenter::hideGlobals() {
  auto treeView = dynamic_cast<FunctionTreeView *>(m_view);
  if (treeView) {
    treeView->hideGlobals();
  }
}

void FunctionMultiDomainPresenter::showGlobals() {
  auto treeView = dynamic_cast<FunctionTreeView *>(m_view);
  if (treeView) {
    treeView->showGlobals();
  }
}

} // namespace MantidQt::MantidWidgets
