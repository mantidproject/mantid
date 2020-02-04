// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "FQTemplateBrowser.h"

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

#include <limits>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
 * Constructor
 * @param parent :: The parent widget.
 */
FQTemplateBrowser::FQTemplateBrowser(QWidget *parent)
    : FunctionTemplateBrowser(parent), m_presenter(this) {
  connect(&m_presenter, SIGNAL(functionStructureChanged()), this,
          SIGNAL(functionStructureChanged()));
}

void FQTemplateBrowser::createProperties() {
  m_parameterManager->blockSignals(true);
  m_boolManager->blockSignals(true);
  m_enumManager->blockSignals(true);

  m_gaussianHeight = m_parameterManager->addProperty("Height");
  m_parameterManager->setDecimals(m_gaussianHeight, 6);
  m_gaussianFQ = m_parameterManager->addProperty("Msd");
  m_parameterManager->setDecimals(m_gaussianFQ, 6);
  m_petersHeight = m_parameterManager->addProperty("Height");
  m_parameterManager->setDecimals(m_petersHeight, 6);
  m_petersFQ = m_parameterManager->addProperty("FQ");
  m_parameterManager->setDecimals(m_petersFQ, 6);
  m_petersBeta = m_parameterManager->addProperty("Beta");
  m_parameterManager->setDecimals(m_petersBeta, 6);
  m_yiHeight = m_parameterManager->addProperty("Height");
  m_parameterManager->setDecimals(m_yiHeight, 6);
  m_yiFQ = m_parameterManager->addProperty("Msd");
  m_parameterManager->setDecimals(m_yiFQ, 6);
  m_yiSigma = m_parameterManager->addProperty("Sigma");
  m_parameterManager->setDecimals(m_yiSigma, 6);

  m_parameterMap[m_gaussianHeight] = 0;
  m_parameterMap[m_gaussianFQ] = 1;
  m_parameterMap[m_petersHeight] = 2;
  m_parameterMap[m_petersFQ] = 3;
  m_parameterMap[m_petersBeta] = 4;
  m_parameterMap[m_yiHeight] = 5;
  m_parameterMap[m_yiFQ] = 6;
  m_parameterMap[m_yiSigma] = 7;

  m_presenter.setViewParameterDescriptions();

  m_parameterManager->setDescription(m_gaussianHeight,
                                     m_parameterDescriptions[m_gaussianHeight]);
  m_parameterManager->setDescription(m_gaussianFQ,
                                     m_parameterDescriptions[m_gaussianFQ]);
  m_parameterManager->setDescription(m_petersHeight,
                                     m_parameterDescriptions[m_petersHeight]);
  m_parameterManager->setDescription(m_petersFQ,
                                     m_parameterDescriptions[m_petersFQ]);
  m_parameterManager->setDescription(m_petersBeta,
                                     m_parameterDescriptions[m_petersBeta]);
  m_parameterManager->setDescription(m_yiHeight,
                                     m_parameterDescriptions[m_yiHeight]);
  m_parameterManager->setDescription(m_yiFQ, m_parameterDescriptions[m_yiFQ]);
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

void FQTemplateBrowser::addGaussian() {
  m_fitType->addSubProperty(m_gaussianHeight);
  m_fitType->addSubProperty(m_gaussianFQ);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, 1);
}

void FQTemplateBrowser::removeGaussian() {
  m_fitType->removeSubProperty(m_gaussianHeight);
  m_fitType->removeSubProperty(m_gaussianFQ);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, 0);
}

void FQTemplateBrowser::addPeters() {
  m_fitType->addSubProperty(m_petersHeight);
  m_fitType->addSubProperty(m_petersFQ);
  m_fitType->addSubProperty(m_petersBeta);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, 2);
}

void FQTemplateBrowser::removePeters() {
  m_fitType->removeSubProperty(m_petersHeight);
  m_fitType->removeSubProperty(m_petersFQ);
  m_fitType->removeSubProperty(m_petersBeta);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, 0);
}

void FQTemplateBrowser::addYi() {
  m_fitType->addSubProperty(m_yiHeight);
  m_fitType->addSubProperty(m_yiFQ);
  m_fitType->addSubProperty(m_yiSigma);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, 3);
}

void FQTemplateBrowser::removeYi() {
  m_fitType->removeSubProperty(m_yiHeight);
  m_fitType->removeSubProperty(m_yiFQ);
  m_fitType->removeSubProperty(m_yiSigma);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, 0);
}

int FQTemplateBrowser::getCurrentDataset() {
  return m_presenter.getCurrentDataset();
}

void FQTemplateBrowser::setGaussianHeight(double value, double error) {
  setParameterPropertyValue(m_gaussianHeight, value, error);
}

void FQTemplateBrowser::setGaussianFQ(double value, double error) {
  setParameterPropertyValue(m_gaussianFQ, value, error);
}

void FQTemplateBrowser::setPetersHeight(double value, double error) {
  setParameterPropertyValue(m_petersHeight, value, error);
}

void FQTemplateBrowser::setPetersFQ(double value, double error) {
  setParameterPropertyValue(m_petersFQ, value, error);
}

void FQTemplateBrowser::setPetersBeta(double value, double error) {
  setParameterPropertyValue(m_petersBeta, value, error);
}

void FQTemplateBrowser::setYiHeight(double value, double error) {
  setParameterPropertyValue(m_yiHeight, value, error);
}

void FQTemplateBrowser::setYiFQ(double value, double error) {
  setParameterPropertyValue(m_yiFQ, value, error);
}

void FQTemplateBrowser::setYiSigma(double value, double error) {
  setParameterPropertyValue(m_yiSigma, value, error);
}

void FQTemplateBrowser::setFunction(const QString &funStr) {
  m_presenter.setFunction(funStr);
}

IFunction_sptr FQTemplateBrowser::getGlobalFunction() const {
  return m_presenter.getGlobalFunction();
}

IFunction_sptr FQTemplateBrowser::getFunction() const {
  return m_presenter.getFunction();
}

void FQTemplateBrowser::setNumberOfDatasets(int n) {
  m_presenter.setNumberOfDatasets(n);
}

int FQTemplateBrowser::getNumberOfDatasets() const {
  return m_presenter.getNumberOfDatasets();
}

void FQTemplateBrowser::setDatasetNames(const QStringList &names) {
  m_presenter.setDatasetNames(names);
}

QStringList FQTemplateBrowser::getGlobalParameters() const {
  return m_presenter.getGlobalParameters();
}

QStringList FQTemplateBrowser::getLocalParameters() const {
  return m_presenter.getLocalParameters();
}

void FQTemplateBrowser::setGlobalParameters(const QStringList &globals) {
  m_presenter.setGlobalParameters(globals);
}

void FQTemplateBrowser::enumChanged(QtProperty *prop) {
  if (!m_emitEnumChange)
    return;
  if (prop == m_fitType) {
    auto fitType = m_enumManager->enumNames(prop)[m_enumManager->value(prop)];
    m_presenter.setFitType(fitType);
  }
}

void FQTemplateBrowser::globalChanged(QtProperty *, const QString &, bool) {
  //  std::cerr << "Global " << name.toStdString() << ' ' << on << std::endl;
}

void FQTemplateBrowser::parameterChanged(QtProperty *prop) {
  auto isGlobal = m_parameterManager->isGlobal(prop);
  m_presenter.setGlobal(m_actualParameterNames[prop], isGlobal);
  if (m_emitParameterValueChange) {
    emit parameterValueChanged(m_actualParameterNames[prop],
                               m_parameterManager->value(prop));
  }
}

void FQTemplateBrowser::parameterButtonClicked(QtProperty *prop) {
  emit localParameterButtonClicked(m_actualParameterNames[prop]);
}

void FQTemplateBrowser::updateMultiDatasetParameters(const IFunction &fun) {
  m_presenter.updateMultiDatasetParameters(fun);
}

void FQTemplateBrowser::updateMultiDatasetParameters(
    const ITableWorkspace &paramTable) {
  m_presenter.updateMultiDatasetParameters(paramTable);
}

void FQTemplateBrowser::updateParameters(const IFunction &fun) {
  m_presenter.updateParameters(fun);
}

void FQTemplateBrowser::setCurrentDataset(int i) {
  m_presenter.setCurrentDataset(i);
}

void FQTemplateBrowser::updateParameterNames(
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

void FQTemplateBrowser::updateParameterDescriptions(
    const QMap<int, std::string> &parameterDescriptions) {
  m_parameterDescriptions.clear();
  for (auto const &prop : m_parameterMap.keys()) {
    const auto i = m_parameterMap[prop];
    m_parameterDescriptions[prop] = parameterDescriptions[i];
  }
}

void FQTemplateBrowser::setErrorsEnabled(bool enabled) {
  ScopedFalse _false(m_emitParameterValueChange);
  m_parameterManager->setErrorsEnabled(enabled);
}

void FQTemplateBrowser::clear() {
  removeGaussian();
  removePeters();
  removeYi();
}

void FQTemplateBrowser::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_presenter.updateParameterEstimationData(std::move(data));
}

void FQTemplateBrowser::popupMenu(const QPoint &) {
  //  std::cerr << "Popup" << std::endl;
}

void FQTemplateBrowser::setParameterPropertyValue(QtProperty *prop,
                                                  double value, double error) {
  if (prop) {
    ScopedFalse _false(m_emitParameterValueChange);
    m_parameterManager->setValue(prop, value);
    m_parameterManager->setError(prop, error);
  }
}

void FQTemplateBrowser::setGlobalParametersQuiet(const QStringList &globals) {
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

void FQTemplateBrowser::setBackgroundA0(double) {}
void FQTemplateBrowser::setResolution(std::string const &,
                                      TableDatasetIndex const &) {}
void FQTemplateBrowser::setResolution(
    const std::vector<std::pair<std::string, int>> &) {}
void FQTemplateBrowser::setQValues(const std::vector<double> &) {}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt