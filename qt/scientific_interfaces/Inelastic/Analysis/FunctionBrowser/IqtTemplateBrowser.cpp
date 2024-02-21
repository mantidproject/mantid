// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IqtTemplateBrowser.h"

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

namespace MantidQt::CustomInterfaces::IDA {

/**
 * Constructor
 * @param parent :: The parent widget.
 */
IqtTemplateBrowser::IqtTemplateBrowser(QWidget *parent) : FunctionTemplateBrowser(parent) { init(); }

void IqtTemplateBrowser::createProperties() {
  m_parameterManager->blockSignals(true);
  m_boolManager->blockSignals(true);
  m_enumManager->blockSignals(true);
  m_exp1Height = m_parameterManager->addProperty("f0.Height");
  m_parameterManager->setDecimals(m_exp1Height, 6);
  m_exp1Lifetime = m_parameterManager->addProperty("f0.Lifetime");
  m_parameterManager->setDecimals(m_exp1Lifetime, 6);
  m_exp2Height = m_parameterManager->addProperty("f1.Height");
  m_parameterManager->setDecimals(m_exp2Height, 6);
  m_exp2Lifetime = m_parameterManager->addProperty("f1.Lifetime");
  m_parameterManager->setDecimals(m_exp2Lifetime, 6);
  m_stretchExpHeight = m_parameterManager->addProperty("Height");
  m_parameterManager->setDecimals(m_stretchExpHeight, 6);
  m_stretchExpLifetime = m_parameterManager->addProperty("Lifetime");
  m_parameterManager->setDecimals(m_stretchExpLifetime, 6);
  m_stretchExpStretching = m_parameterManager->addProperty("Stretching");
  m_parameterManager->setDecimals(m_stretchExpStretching, 6);
  m_A0 = m_parameterManager->addProperty("A0");
  m_parameterManager->setDecimals(m_A0, 6);

  m_parameterMap[m_exp1Height] = 0;
  m_parameterMap[m_exp1Lifetime] = 1;
  m_parameterMap[m_exp2Height] = 2;
  m_parameterMap[m_exp2Lifetime] = 3;
  m_parameterMap[m_stretchExpHeight] = 4;
  m_parameterMap[m_stretchExpLifetime] = 5;
  m_parameterMap[m_stretchExpStretching] = 6;
  m_parameterMap[m_A0] = 7;

  m_parameterManager->setDescription(m_exp1Height, m_parameterDescriptions[m_exp1Height]);
  m_parameterManager->setDescription(m_exp1Lifetime, m_parameterDescriptions[m_exp1Lifetime]);
  m_parameterManager->setDescription(m_exp2Height, m_parameterDescriptions[m_exp2Height]);
  m_parameterManager->setDescription(m_exp2Lifetime, m_parameterDescriptions[m_exp2Lifetime]);
  m_parameterManager->setDescription(m_stretchExpHeight, m_parameterDescriptions[m_stretchExpHeight]);
  m_parameterManager->setDescription(m_stretchExpLifetime, m_parameterDescriptions[m_stretchExpLifetime]);
  m_parameterManager->setDescription(m_stretchExpStretching, m_parameterDescriptions[m_stretchExpStretching]);
  m_parameterManager->setDescription(m_A0, m_parameterDescriptions[m_A0]);

  m_numberOfExponentials = m_intManager->addProperty("Exponentials");
  m_intManager->setMinimum(m_numberOfExponentials, 0);
  m_intManager->setMaximum(m_numberOfExponentials, 2);
  m_browser->addProperty(m_numberOfExponentials);

  m_stretchExponential = m_boolManager->addProperty("Stretch Exponential");
  m_browser->addProperty(m_stretchExponential);

  m_background = m_enumManager->addProperty("Background");
  QStringList backgrounds;
  backgrounds << "None"
              << "FlatBackground";
  m_enumManager->setEnumNames(m_background, backgrounds);
  m_browser->addProperty(m_background);

  m_tieIntensities = m_boolManager->addProperty("Tie Intensities");
  m_browser->addProperty(m_tieIntensities);
  m_parameterManager->blockSignals(false);
  m_enumManager->blockSignals(false);
  m_boolManager->blockSignals(false);
}

void IqtTemplateBrowser::addExponentialOne() {
  m_numberOfExponentials->addSubProperty(m_exp1Height);
  m_numberOfExponentials->addSubProperty(m_exp1Lifetime);
  setIntSilent(m_numberOfExponentials, 1);
}

void IqtTemplateBrowser::removeExponentialOne() {
  m_numberOfExponentials->removeSubProperty(m_exp1Height);
  m_numberOfExponentials->removeSubProperty(m_exp1Lifetime);
  setIntSilent(m_numberOfExponentials, 0);
}

void IqtTemplateBrowser::addExponentialTwo() {
  m_numberOfExponentials->addSubProperty(m_exp2Height);
  m_numberOfExponentials->addSubProperty(m_exp2Lifetime);
  setIntSilent(m_numberOfExponentials, 2);
}

void IqtTemplateBrowser::removeExponentialTwo() {
  m_numberOfExponentials->removeSubProperty(m_exp2Height);
  m_numberOfExponentials->removeSubProperty(m_exp2Lifetime);
  setIntSilent(m_numberOfExponentials, 1);
}

void IqtTemplateBrowser::addStretchExponential() {
  m_stretchExponential->addSubProperty(m_stretchExpHeight);
  m_stretchExponential->addSubProperty(m_stretchExpLifetime);
  m_stretchExponential->addSubProperty(m_stretchExpStretching);
  setBoolSilent(m_stretchExponential, true);
}

void IqtTemplateBrowser::removeStretchExponential() {
  m_stretchExponential->removeSubProperty(m_stretchExpHeight);
  m_stretchExponential->removeSubProperty(m_stretchExpLifetime);
  m_stretchExponential->removeSubProperty(m_stretchExpStretching);
  setBoolSilent(m_stretchExponential, false);
}

void IqtTemplateBrowser::addFlatBackground() {
  m_background->addSubProperty(m_A0);
  setEnumSilent(m_background, 1);
}

void IqtTemplateBrowser::removeBackground() {
  m_background->removeSubProperty(m_A0);
  setEnumSilent(m_background, 0);
}

void IqtTemplateBrowser::setExp1Height(double value, double error) { setParameterSilent(m_exp1Height, value, error); }

void IqtTemplateBrowser::setExp1Lifetime(double value, double error) {
  setParameterSilent(m_exp1Lifetime, value, error);
}

void IqtTemplateBrowser::setExp2Height(double value, double error) { setParameterSilent(m_exp2Height, value, error); }

void IqtTemplateBrowser::setExp2Lifetime(double value, double error) {
  setParameterSilent(m_exp2Lifetime, value, error);
}

void IqtTemplateBrowser::setStretchHeight(double value, double error) {
  setParameterSilent(m_stretchExpHeight, value, error);
}

void IqtTemplateBrowser::setStretchLifetime(double value, double error) {
  setParameterSilent(m_stretchExpLifetime, value, error);
}

void IqtTemplateBrowser::setStretchStretching(double value, double error) {
  setParameterSilent(m_stretchExpStretching, value, error);
}

void IqtTemplateBrowser::setA0(double value, double error) { setParameterSilent(m_A0, value, error); }

void IqtTemplateBrowser::intChanged(QtProperty *prop) {
  if (prop == m_numberOfExponentials && m_emitIntChange) {
    m_presenter->setNumberOfExponentials(m_intManager->value(prop));
  }
}

void IqtTemplateBrowser::boolChanged(QtProperty *prop) {
  if (!m_emitBoolChange)
    return;
  auto const on = m_boolManager->value(prop);
  if (prop == m_stretchExponential) {
    m_presenter->setStretchExponential(on);
  }
  if (prop == m_tieIntensities) {
    m_presenter->tieIntensities(on);
  }
}

void IqtTemplateBrowser::enumChanged(QtProperty *prop) {
  if (!m_emitEnumChange)
    return;
  if (prop == m_background) {
    auto background = m_enumManager->enumNames(prop)[m_enumManager->value(prop)];
    m_presenter->setBackground(background.toStdString());
  }
}

void IqtTemplateBrowser::parameterChanged(QtProperty *prop) {
  auto isGlobal = m_parameterManager->isGlobal(prop);
  m_presenter->setGlobal(m_parameterNames[prop], isGlobal);
  if (m_emitParameterValueChange) {
    m_presenter->handleParameterValueChanged(m_parameterNames[prop], m_parameterManager->value(prop));
  }
}

void IqtTemplateBrowser::updateMultiDatasetParameters(const ITableWorkspace &paramTable) {
  m_presenter->updateMultiDatasetParameters(paramTable);
}

void IqtTemplateBrowser::updateParameters(const IFunction &fun) { m_presenter->updateParameters(fun); }

void IqtTemplateBrowser::updateParameterNames(const QMap<int, std::string> &parameterNames) {
  MantidQt::MantidWidgets::ScopedFalse _parameterBlock(m_emitParameterValueChange);
  m_parameterNames.clear();
  for (auto const prop : m_parameterMap.keys()) {
    auto const i = m_parameterMap[prop];
    auto const name = parameterNames[i];
    m_parameterNames[prop] = name;
    if (!name.empty()) {
      prop->setPropertyName(QString::fromStdString(name));
    }
  }
}

void IqtTemplateBrowser::updateParameterDescriptions(const QMap<int, std::string> &parameterDescriptions) {
  m_parameterDescriptions.clear();
  for (auto const prop : m_parameterMap.keys()) {
    auto const i = m_parameterMap[prop];
    m_parameterDescriptions[prop] = parameterDescriptions[i];
  }
}

void IqtTemplateBrowser::clear() {
  removeBackground();
  removeStretchExponential();
  removeExponentialTwo();
  removeExponentialOne();
}

EstimationDataSelector IqtTemplateBrowser::getEstimationDataSelector() const {
  return m_presenter->getEstimationDataSelector();
}

void IqtTemplateBrowser::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_presenter->updateParameterEstimationData(std::move(data));
}

void IqtTemplateBrowser::estimateFunctionParameters() { m_presenter->estimateFunctionParameters(); }

void IqtTemplateBrowser::setBackgroundA0(double value) { m_presenter->setBackgroundA0(value); }

double IqtTemplateBrowser::getParameterPropertyValue(QtProperty *prop) const {
  return prop ? m_parameterManager->value(prop) : 0.0;
}

void IqtTemplateBrowser::setGlobalParametersQuiet(std::vector<std::string> const &globals) {
  MantidQt::MantidWidgets::ScopedFalse _paramBlock(m_emitParameterValueChange);
  for (auto const prop : m_parameterMap.keys()) {
    auto const parameterName = m_parameterNames[prop];
    auto const findIter = std::find(globals.cbegin(), globals.cend(), parameterName);
    m_parameterManager->setGlobal(prop, findIter != globals.cend());
  }
}

void IqtTemplateBrowser::setTieIntensitiesQuiet(bool on) { setBoolSilent(m_tieIntensities, on); }

void IqtTemplateBrowser::updateState() {
  auto const on = m_presenter->canTieIntensities();
  if (!on && m_boolManager->value(m_tieIntensities)) {
    setBoolSilent(m_tieIntensities, false);
  }
  m_tieIntensities->setEnabled(on);
}

} // namespace MantidQt::CustomInterfaces::IDA
