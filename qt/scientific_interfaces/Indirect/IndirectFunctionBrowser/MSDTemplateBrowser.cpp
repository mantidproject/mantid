// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MSDTemplateBrowser.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/PropertyWithValue.h"

#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/ButtonEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/CompositeEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>

#include <iostream>
#include <limits>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
 * Constructor
 * @param parent :: The parent widget.
 */
MSDTemplateBrowser::MSDTemplateBrowser(QWidget *parent)
    : FunctionTemplateBrowser(parent), m_presenter(this) {
  connect(&m_presenter, SIGNAL(functionStructureChanged()), this,
          SIGNAL(functionStructureChanged()));
}

void MSDTemplateBrowser::createProperties() {
  m_parameterManager->blockSignals(true);
  m_boolManager->blockSignals(true);
  m_enumManager->blockSignals(true);

  m_gaussianHeight = m_parameterManager->addProperty("Height");
  m_parameterManager->setDecimals(m_gaussianHeight, 6);
  m_gaussianMsd = m_parameterManager->addProperty("Msd");
  m_parameterManager->setDecimals(m_gaussianMsd, 6);
  m_petersHeight = m_parameterManager->addProperty("Height");
  m_parameterManager->setDecimals(m_petersHeight, 6);
  m_petersMsd = m_parameterManager->addProperty("Msd");
  m_parameterManager->setDecimals(m_petersMsd, 6);
  m_petersBeta = m_parameterManager->addProperty("Beta");
  m_parameterManager->setDecimals(m_petersBeta, 6);
  m_yiHeight = m_parameterManager->addProperty("Height");
  m_parameterManager->setDecimals(m_yiHeight, 6);
  m_yiMsd = m_parameterManager->addProperty("Msd");
  m_parameterManager->setDecimals(m_yiMsd, 6);
  m_yiSigma = m_parameterManager->addProperty("Sigma");
  m_parameterManager->setDecimals(m_yiSigma, 6);

  m_parameterMap[m_gaussianHeight] = 0;
  m_parameterMap[m_gaussianMsd] = 1;
  m_parameterMap[m_petersHeight] = 2;
  m_parameterMap[m_petersMsd] = 3;
  m_parameterMap[m_petersBeta] = 4;
  m_parameterMap[m_yiHeight] = 5;
  m_parameterMap[m_yiMsd] = 6;
  m_parameterMap[m_yiSigma] = 7;

  m_presenter.setViewParameterDescriptions();

  m_parameterManager->setDescription(m_gaussianHeight,
                                     m_parameterDescriptions[m_gaussianHeight]);
  m_parameterManager->setDescription(m_gaussianMsd,
                                     m_parameterDescriptions[m_gaussianMsd]);
  m_parameterManager->setDescription(m_petersHeight,
                                     m_parameterDescriptions[m_petersHeight]);
  m_parameterManager->setDescription(m_petersMsd,
                                     m_parameterDescriptions[m_petersMsd]);
  m_parameterManager->setDescription(m_petersBeta,
                                     m_parameterDescriptions[m_petersBeta]);
  m_parameterManager->setDescription(m_yiHeight,
                                     m_parameterDescriptions[m_yiHeight]);
  m_parameterManager->setDescription(m_yiMsd, m_parameterDescriptions[m_yiMsd]);
  m_parameterManager->setDescription(m_yiSigma,
                                     m_parameterDescriptions[m_yiSigma]);

  m_fitType = m_enumManager->addProperty("Fit Type");
  QStringList fitType;
  fitType << "None"
          << "Gaussian"
          << "Peters"
          << "Yi";
  m_enumManager->setEnumNames(m_fitType, fitType);
  m_browser->addProperty(m_fitType);

  m_parameterManager->blockSignals(false);
  m_enumManager->blockSignals(false);
  m_boolManager->blockSignals(false);
}

void MSDTemplateBrowser::addGaussian() {
  m_fitType->addSubProperty(m_gaussianHeight);
  m_fitType->addSubProperty(m_gaussianMsd);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, 1);
}

void MSDTemplateBrowser::removeGaussian() {
  m_fitType->removeSubProperty(m_gaussianHeight);
  m_fitType->removeSubProperty(m_gaussianMsd);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, 0);
}

void MSDTemplateBrowser::addPeters() {
  m_fitType->addSubProperty(m_petersHeight);
  m_fitType->addSubProperty(m_petersMsd);
  m_fitType->addSubProperty(m_petersBeta);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, 2);
}

void MSDTemplateBrowser::removePeters() {
  m_fitType->removeSubProperty(m_petersHeight);
  m_fitType->removeSubProperty(m_petersMsd);
  m_fitType->removeSubProperty(m_petersBeta);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, 0);
}

void MSDTemplateBrowser::addYi() {
  m_fitType->addSubProperty(m_yiHeight);
  m_fitType->addSubProperty(m_yiMsd);
  m_fitType->addSubProperty(m_yiSigma);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, 3);
}

void MSDTemplateBrowser::removeYi() {
  m_fitType->removeSubProperty(m_yiHeight);
  m_fitType->removeSubProperty(m_yiMsd);
  m_fitType->removeSubProperty(m_yiSigma);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, 0);
}

void MSDTemplateBrowser::setGaussianHeight(double value, double error) {
  setParameterPropertyValue(m_gaussianHeight, value, error);
}

void MSDTemplateBrowser::setGaussianMsd(double value, double error) {
  setParameterPropertyValue(m_gaussianMsd, value, error);
}

void MSDTemplateBrowser::setPetersHeight(double value, double error) {
  setParameterPropertyValue(m_petersHeight, value, error);
}

void MSDTemplateBrowser::setPetersMsd(double value, double error) {
  setParameterPropertyValue(m_petersMsd, value, error);
}

void MSDTemplateBrowser::setPetersBeta(double value, double error) {
  setParameterPropertyValue(m_petersBeta, value, error);
}

void MSDTemplateBrowser::setYiHeight(double value, double error) {
  setParameterPropertyValue(m_yiHeight, value, error);
}

void MSDTemplateBrowser::setYiMsd(double value, double error) {
  setParameterPropertyValue(m_yiMsd, value, error);
}

void MSDTemplateBrowser::setYiSigma(double value, double error) {
  setParameterPropertyValue(m_yiSigma, value, error);
}

void MSDTemplateBrowser::setFunction(const QString &funStr) {
  m_presenter.setFunction(funStr);
}

IFunction_sptr MSDTemplateBrowser::getGlobalFunction() const {
  return m_presenter.getGlobalFunction();
}

IFunction_sptr MSDTemplateBrowser::getFunction() const {
  return m_presenter.getFunction();
}

void MSDTemplateBrowser::setNumberOfDatasets(int n) {
  m_presenter.setNumberOfDatasets(n);
}

int MSDTemplateBrowser::getNumberOfDatasets() const {
  return m_presenter.getNumberOfDatasets();
}

void MSDTemplateBrowser::setDatasetNames(const QStringList &names) {
  m_presenter.setDatasetNames(names);
}

QStringList MSDTemplateBrowser::getGlobalParameters() const {
  return m_presenter.getGlobalParameters();
}

QStringList MSDTemplateBrowser::getLocalParameters() const {
  return m_presenter.getLocalParameters();
}

void MSDTemplateBrowser::setGlobalParameters(const QStringList &globals) {
  m_presenter.setGlobalParameters(globals);
}

void MSDTemplateBrowser::enumChanged(QtProperty *prop) {
  if (!m_emitEnumChange)
    return;
  if (prop == m_fitType) {
    auto fitType = m_enumManager->enumNames(prop)[m_enumManager->value(prop)];
    m_presenter.setFitType(fitType);
  }
}

void MSDTemplateBrowser::globalChanged(QtProperty *prop, const QString &name,
                                       bool on) {
  std::cerr << "Global " << name.toStdString() << ' ' << on << std::endl;
}

void MSDTemplateBrowser::parameterChanged(QtProperty *prop) {
  auto isGlobal = m_parameterManager->isGlobal(prop);
  m_presenter.setGlobal(m_actualParameterNames[prop], isGlobal);
  if (m_emitParameterValueChange) {
    emit parameterValueChanged(m_actualParameterNames[prop],
                               m_parameterManager->value(prop));
  }
}

void MSDTemplateBrowser::parameterButtonClicked(QtProperty *prop) {
  emit localParameterButtonClicked(m_actualParameterNames[prop]);
}

void MSDTemplateBrowser::updateMultiDatasetParameters(const IFunction &fun) {
  m_presenter.updateMultiDatasetParameters(fun);
}

void MSDTemplateBrowser::updateMultiDatasetParameters(
    const ITableWorkspace &paramTable) {
  m_presenter.updateMultiDatasetParameters(paramTable);
}

void MSDTemplateBrowser::updateParameters(const IFunction &fun) {
  m_presenter.updateParameters(fun);
}

void MSDTemplateBrowser::setCurrentDataset(int i) {
  m_presenter.setCurrentDataset(i);
}

void MSDTemplateBrowser::updateParameterNames(
    const QMap<int, QString> &parameterNames) {
  m_actualParameterNames.clear();
  ScopedFalse _false(m_emitParameterValueChange);
  for (const auto &prop : m_parameterMap.keys()) {
    const auto i = m_parameterMap[prop];
    const auto name = parameterNames[i];
    m_actualParameterNames[prop] = name;
    if (!name.isEmpty()) {
      prop->setPropertyName(name);
    }
  }
}

void MSDTemplateBrowser::updateParameterDescriptions(
    const QMap<int, std::string> &parameterDescriptions) {
  m_parameterDescriptions.clear();
  for (auto const &prop : m_parameterMap.keys()) {
    const auto i = m_parameterMap[prop];
    m_parameterDescriptions[prop] = parameterDescriptions[i];
  }
}

void MSDTemplateBrowser::setErrorsEnabled(bool enabled) {
  ScopedFalse _false(m_emitParameterValueChange);
  m_parameterManager->setErrorsEnabled(enabled);
}

void MSDTemplateBrowser::clear() {
  removeGaussian();
  removePeters();
  removeYi();
}

void MSDTemplateBrowser::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_presenter.updateParameterEstimationData(std::move(data));
}

void MSDTemplateBrowser::popupMenu(const QPoint &) {
  std::cerr << "Popup" << std::endl;
}

void MSDTemplateBrowser::setParameterPropertyValue(QtProperty *prop,
                                                   double value, double error) {
  if (prop) {
    ScopedFalse _false(m_emitParameterValueChange);
    m_parameterManager->setValue(prop, value);
    m_parameterManager->setError(prop, error);
  }
}

void MSDTemplateBrowser::setGlobalParametersQuiet(const QStringList &globals) {
  ScopedFalse _false(m_emitParameterValueChange);
  auto parameterProperties = m_parameterMap.keys();
  for (const auto &prop : m_parameterMap.keys()) {
    auto const name = m_actualParameterNames[prop];
    if (globals.contains(name)) {
      m_parameterManager->setGlobal(prop, true);
      parameterProperties.removeOne(prop);
    }
  }
  for (const auto &prop : parameterProperties) {
    if (!m_actualParameterNames[prop].isEmpty()) {
      m_parameterManager->setGlobal(prop, false);
    }
  }
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
