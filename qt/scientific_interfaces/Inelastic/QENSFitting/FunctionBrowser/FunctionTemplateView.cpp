// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FunctionTemplateView.h"

#include "ITemplatePresenter.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/PropertyWithValue.h"

#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/ButtonEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/CompositeEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleDialogEditor.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>
#include <limits>

namespace MantidQt::CustomInterfaces::Inelastic {

FunctionTemplateView::FunctionTemplateView()
    : QWidget(), m_boolManager(), m_intManager(), m_doubleManager(), m_stringManager(), m_enumManager(),
      m_groupManager(), m_parameterManager(), m_parameterNames(), m_browser(), m_presenter() {}

FunctionTemplateView::~FunctionTemplateView() {
  m_browser->unsetFactoryForManager(m_stringManager);
  m_browser->unsetFactoryForManager(m_doubleManager);
  m_browser->unsetFactoryForManager(m_intManager);
  m_browser->unsetFactoryForManager(m_boolManager);
  m_browser->unsetFactoryForManager(m_enumManager);
  m_browser->unsetFactoryForManager(m_parameterManager);
}

void FunctionTemplateView::createBrowser() {
  m_stringManager = new QtStringPropertyManager(this);
  m_doubleManager = new QtDoublePropertyManager(this);
  m_intManager = new QtIntPropertyManager(this);
  m_boolManager = new QtBoolPropertyManager(this);
  m_enumManager = new QtEnumPropertyManager(this);
  m_groupManager = new QtGroupPropertyManager(this);
  m_parameterManager = new ParameterPropertyManager(this, true);

  // create editor factories
  // Here we create a spin box factory with a custom timer method
  // This avoids the slot double incrementing the box
  auto *spinBoxFactoryNoTimer = new QtSpinBoxFactoryNoTimer(this);
  auto *doubleEditorFactory = new DoubleEditorFactory(this);
  auto *lineEditFactory = new QtLineEditFactory(this);
  auto *checkBoxFactory = new QtCheckBoxFactory(this);
  auto *comboBoxFactory = new QtEnumEditorFactory(this);
  auto *doubleDialogFactory = new DoubleDialogEditorFactory(this, true);

  m_browser = new QtTreePropertyBrowser(nullptr, QStringList(), false);
  // assign factories to property managers
  m_browser->setFactoryForManager(m_stringManager, lineEditFactory);
  m_browser->setFactoryForManager(m_doubleManager, doubleEditorFactory);
  m_browser->setFactoryForManager(m_intManager, spinBoxFactoryNoTimer);
  m_browser->setFactoryForManager(m_boolManager, checkBoxFactory);
  m_browser->setFactoryForManager(m_enumManager, comboBoxFactory);
  m_browser->setFactoryForManager(m_parameterManager, doubleDialogFactory);

  connect(m_intManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(intChanged(QtProperty *)));
  connect(m_boolManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(boolChanged(QtProperty *)));
  connect(m_enumManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(enumChanged(QtProperty *)));
  connect(m_parameterManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(parameterChanged(QtProperty *)));

  connect(doubleDialogFactory, SIGNAL(buttonClicked(QtProperty *)), this, SLOT(parameterButtonClicked(QtProperty *)));
  connect(doubleDialogFactory, SIGNAL(closeEditor()), m_browser, SLOT(closeEditor()));

  m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_browser, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));

  connect(m_browser, SIGNAL(optionChanged(QtProperty *, const QString &, bool)), this,
          SLOT(globalChanged(QtProperty *, const QString &, bool)));
}

void FunctionTemplateView::init() {
  createBrowser();
  createProperties();
  auto *layout = new QVBoxLayout(this);
  layout->addWidget(m_browser);
  layout->setContentsMargins(0, 0, 0, 0);
}

void FunctionTemplateView::subscribePresenter(ITemplatePresenter *presenter) { m_presenter = presenter; }

void FunctionTemplateView::clear() { m_browser->clear(); }

void FunctionTemplateView::setEnumSilent(QtProperty *prop, int enumIndex) {
  MantidQt::MantidWidgets::ScopedFalse _enumBlock(m_emitEnumChange);
  m_enumManager->setValue(prop, enumIndex);
}

void FunctionTemplateView::setIntSilent(QtProperty *prop, int value) {
  MantidQt::MantidWidgets::ScopedFalse _intBlock(m_emitIntChange);
  m_intManager->setValue(prop, value);
}

void FunctionTemplateView::setBoolSilent(QtProperty *prop, bool value) {
  MantidQt::MantidWidgets::ScopedFalse _boolBlock(m_emitBoolChange);
  m_boolManager->setValue(prop, value);
}

void FunctionTemplateView::setParameterSilent(QtProperty *prop, double value, double error) {
  MantidQt::MantidWidgets::ScopedFalse _parameterBlock(m_emitParameterValueChange);
  m_parameterManager->setValue(prop, value);
  m_parameterManager->setError(prop, error);
}

void FunctionTemplateView::setErrorsEnabled(bool enabled) {
  MantidQt::MantidWidgets::ScopedFalse _parameterBlock(m_emitParameterValueChange);
  m_parameterManager->setErrorsEnabled(enabled);
}

void FunctionTemplateView::setFunction(std::string const &funStr) { m_presenter->setFunction(funStr); }

IFunction_sptr FunctionTemplateView::getGlobalFunction() const { return m_presenter->getGlobalFunction(); }

IFunction_sptr FunctionTemplateView::getFunction() const { return m_presenter->getFunction(); }

void FunctionTemplateView::setCurrentDataset(int i) { m_presenter->setCurrentDataset(i); }

int FunctionTemplateView::getCurrentDataset() { return m_presenter->getCurrentDataset(); }

void FunctionTemplateView::setNumberOfDatasets(int n) { m_presenter->setNumberOfDatasets(n); }

int FunctionTemplateView::getNumberOfDatasets() const { return m_presenter->getNumberOfDatasets(); }

void FunctionTemplateView::setDatasets(const QList<FunctionModelDataset> &datasets) {
  m_presenter->setDatasets(datasets);
}

std::vector<std::string> FunctionTemplateView::getGlobalParameters() const {
  return m_presenter->getGlobalParameters();
}

std::vector<std::string> FunctionTemplateView::getLocalParameters() const { return m_presenter->getLocalParameters(); }

void FunctionTemplateView::setGlobalParameters(std::vector<std::string> const &globals) {
  m_presenter->setGlobalParameters(globals);
}

void FunctionTemplateView::updateMultiDatasetParameters(const IFunction &fun) {
  m_presenter->updateMultiDatasetParameters(fun);
}

void FunctionTemplateView::updateMultiDatasetParameters(const ITableWorkspace &table) {
  m_presenter->updateMultiDatasetParameters(table);
}

void FunctionTemplateView::updateParameters(const IFunction &fun) { m_presenter->updateParameters(fun); }

void FunctionTemplateView::openEditLocalParameterDialog(std::string const &parameterName,
                                                        std::vector<std::string> const &datasetNames,
                                                        std::vector<std::string> const &domainNames,
                                                        QList<double> const &values, QList<bool> const &fixes,
                                                        QStringList const &ties, QStringList const &constraints) {
  auto dialog =
      new EditLocalParameterDialog(this, parameterName, datasetNames, domainNames, values, fixes, ties, constraints);
  connect(dialog, SIGNAL(dialogFinished(int, EditLocalParameterDialog *)), this,
          SLOT(editLocalParameterFinished(int, EditLocalParameterDialog *)));
  dialog->open();
}

void FunctionTemplateView::parameterButtonClicked(QtProperty *prop) {
  m_presenter->handleEditLocalParameter(m_parameterNames[prop]);
}

void FunctionTemplateView::editLocalParameterFinished(int result, EditLocalParameterDialog *dialog) {
  if (result == QDialog::Accepted) {
    m_presenter->handleEditLocalParameterFinished(dialog->getParameterName(), dialog->getValues(), dialog->getFixes(),
                                                  dialog->getTies(), dialog->getConstraints());
  }
  emitFunctionStructureChanged();
}

EstimationDataSelector FunctionTemplateView::getEstimationDataSelector() const {
  return m_presenter->getEstimationDataSelector();
}

void FunctionTemplateView::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_presenter->updateParameterEstimationData(std::move(data));
}

void FunctionTemplateView::estimateFunctionParameters() { m_presenter->estimateFunctionParameters(); }

} // namespace MantidQt::CustomInterfaces::Inelastic
