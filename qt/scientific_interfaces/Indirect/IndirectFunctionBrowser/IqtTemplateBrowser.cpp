// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IqtTemplateBrowser.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/PropertyWithValue.h"

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

namespace {
class ScopedFalse {
  bool &m_ref;
  bool m_oldValue;

public:
  ScopedFalse(bool &variable) : m_ref(variable), m_oldValue(variable) {
    m_ref = false;
  }
  ~ScopedFalse() { m_ref = m_oldValue; }
};
} // namespace

/**
 * Constructor
 * @param parent :: The parent widget.
 */
IqtTemplateBrowser::IqtTemplateBrowser(QWidget *parent)
    : FunctionTemplateBrowser(parent), m_presenter(this) {
  connect(&m_presenter, SIGNAL(functionStructureChanged()), this,
          SIGNAL(functionStructureChanged()));
}

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

  m_presenter.setViewParameterDescriptions();

  m_parameterManager->setDescription(m_exp1Height,
                                     m_parameterDescriptions[m_exp1Height]);
  m_parameterManager->setDescription(m_exp1Lifetime,
                                     m_parameterDescriptions[m_exp1Lifetime]);
  m_parameterManager->setDescription(m_exp2Height,
                                     m_parameterDescriptions[m_exp2Height]);
  m_parameterManager->setDescription(m_exp2Lifetime,
                                     m_parameterDescriptions[m_exp2Lifetime]);
  m_parameterManager->setDescription(
      m_stretchExpHeight, m_parameterDescriptions[m_stretchExpHeight]);
  m_parameterManager->setDescription(
      m_stretchExpLifetime, m_parameterDescriptions[m_stretchExpLifetime]);
  m_parameterManager->setDescription(
      m_stretchExpStretching, m_parameterDescriptions[m_stretchExpStretching]);
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
  updateState();
}

void IqtTemplateBrowser::addExponentialOne() {
  m_numberOfExponentials->addSubProperty(m_exp1Height);
  m_numberOfExponentials->addSubProperty(m_exp1Lifetime);
  ScopedFalse _false(m_emitIntChange);
  m_intManager->setValue(m_numberOfExponentials, 1);
}

void IqtTemplateBrowser::removeExponentialOne() {
  m_numberOfExponentials->removeSubProperty(m_exp1Height);
  m_numberOfExponentials->removeSubProperty(m_exp1Lifetime);
  ScopedFalse _false(m_emitIntChange);
  m_intManager->setValue(m_numberOfExponentials, 0);
}

void IqtTemplateBrowser::addExponentialTwo() {
  m_numberOfExponentials->addSubProperty(m_exp2Height);
  m_numberOfExponentials->addSubProperty(m_exp2Lifetime);
  ScopedFalse _false(m_emitIntChange);
  m_intManager->setValue(m_numberOfExponentials, 2);
}

void IqtTemplateBrowser::removeExponentialTwo() {
  m_numberOfExponentials->removeSubProperty(m_exp2Height);
  m_numberOfExponentials->removeSubProperty(m_exp2Lifetime);
  ScopedFalse _false(m_emitIntChange);
  m_intManager->setValue(m_numberOfExponentials, 1);
}

void IqtTemplateBrowser::addStretchExponential() {
  m_stretchExponential->addSubProperty(m_stretchExpHeight);
  m_stretchExponential->addSubProperty(m_stretchExpLifetime);
  m_stretchExponential->addSubProperty(m_stretchExpStretching);
  ScopedFalse _false(m_emitBoolChange);
  m_boolManager->setValue(m_stretchExponential, true);
}

void IqtTemplateBrowser::removeStretchExponential() {
  m_stretchExponential->removeSubProperty(m_stretchExpHeight);
  m_stretchExponential->removeSubProperty(m_stretchExpLifetime);
  m_stretchExponential->removeSubProperty(m_stretchExpStretching);
  ScopedFalse _false(m_emitBoolChange);
  m_boolManager->setValue(m_stretchExponential, false);
}

void IqtTemplateBrowser::addFlatBackground() {
  m_background->addSubProperty(m_A0);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_background, 1);
}

void IqtTemplateBrowser::removeBackground() {
  m_background->removeSubProperty(m_A0);
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_background, 0);
}

void IqtTemplateBrowser::setExp1Height(double value, double error) {
  setParameterPropertyValue(m_exp1Height, value, error);
}

void IqtTemplateBrowser::setExp1Lifetime(double value, double error) {
  setParameterPropertyValue(m_exp1Lifetime, value, error);
}

void IqtTemplateBrowser::setExp2Height(double value, double error) {
  setParameterPropertyValue(m_exp2Height, value, error);
}

void IqtTemplateBrowser::setExp2Lifetime(double value, double error) {
  setParameterPropertyValue(m_exp2Lifetime, value, error);
}

void IqtTemplateBrowser::setStretchHeight(double value, double error) {
  setParameterPropertyValue(m_stretchExpHeight, value, error);
}

void IqtTemplateBrowser::setStretchLifetime(double value, double error) {
  setParameterPropertyValue(m_stretchExpLifetime, value, error);
}

void IqtTemplateBrowser::setStretchStretching(double value, double error) {
  setParameterPropertyValue(m_stretchExpStretching, value, error);
}

void IqtTemplateBrowser::setA0(double value, double error) {
  setParameterPropertyValue(m_A0, value, error);
}

void IqtTemplateBrowser::setFunction(const QString &funStr) {
  m_presenter.setFunction(funStr);
}

IFunction_sptr IqtTemplateBrowser::getGlobalFunction() const {
  return m_presenter.getGlobalFunction();
}

IFunction_sptr IqtTemplateBrowser::getFunction() const {
  return m_presenter.getFunction();
}

void IqtTemplateBrowser::setNumberOfDatasets(int n) {
  m_presenter.setNumberOfDatasets(n);
}

int IqtTemplateBrowser::getNumberOfDatasets() const {
  return m_presenter.getNumberOfDatasets();
}

void IqtTemplateBrowser::setDatasetNames(const QStringList &names) {
  m_presenter.setDatasetNames(names);
}

QStringList IqtTemplateBrowser::getGlobalParameters() const {
  return m_presenter.getGlobalParameters();
}

QStringList IqtTemplateBrowser::getLocalParameters() const {
  return m_presenter.getLocalParameters();
}

void IqtTemplateBrowser::setGlobalParameters(const QStringList &globals) {
  m_presenter.setGlobalParameters(globals);
}

void IqtTemplateBrowser::intChanged(QtProperty *prop) {
  if (prop == m_numberOfExponentials && m_emitIntChange) {
    m_presenter.setNumberOfExponentials(m_intManager->value(prop));
  }
}

void IqtTemplateBrowser::boolChanged(QtProperty *prop) {
  if (!m_emitBoolChange)
    return;
  auto const on = m_boolManager->value(prop);
  if (prop == m_stretchExponential) {
    m_presenter.setStretchExponential(on);
  }
  if (prop == m_tieIntensities) {
    m_presenter.tieIntensities(on);
  }
}

void IqtTemplateBrowser::enumChanged(QtProperty *prop) {
  if (!m_emitEnumChange)
    return;
  if (prop == m_background) {
    auto background =
        m_enumManager->enumNames(prop)[m_enumManager->value(prop)];
    m_presenter.setBackground(background);
  }
}

void IqtTemplateBrowser::globalChanged(QtProperty *prop, const QString &name,
                                       bool on) {
  std::cerr << "Global " << name.toStdString() << ' ' << on << std::endl;
}

void IqtTemplateBrowser::parameterChanged(QtProperty *prop) {
  auto isGlobal = m_parameterManager->isGlobal(prop);
  m_presenter.setGlobal(m_actualParameterNames[prop], isGlobal);
  if (m_emitParameterValueChange) {
    emit parameterValueChanged(m_actualParameterNames[prop],
                               m_parameterManager->value(prop));
  }
}

void IqtTemplateBrowser::parameterButtonClicked(QtProperty *prop) {
  emit localParameterButtonClicked(m_actualParameterNames[prop]);
}

void IqtTemplateBrowser::updateMultiDatasetParameters(const IFunction &fun) {
  m_presenter.updateMultiDatasetParameters(fun);
}

void IqtTemplateBrowser::updateMultiDatasetParameters(
    const ITableWorkspace &paramTable) {
  m_presenter.updateMultiDatasetParameters(paramTable);
}

void IqtTemplateBrowser::updateParameters(const IFunction &fun) {
  m_presenter.updateParameters(fun);
}

void IqtTemplateBrowser::setCurrentDataset(int i) {
  m_presenter.setCurrentDataset(i);
}

void IqtTemplateBrowser::updateParameterNames(
    const QMap<int, QString> &parameterNames) {
  m_actualParameterNames.clear();
  ScopedFalse _false(m_emitParameterValueChange);
  for (auto const prop : m_parameterMap.keys()) {
    auto const i = m_parameterMap[prop];
    auto const name = parameterNames[i];
    m_actualParameterNames[prop] = name;
    if (!name.isEmpty()) {
      prop->setPropertyName(name);
    }
  }
}

void IqtTemplateBrowser::updateParameterDescriptions(
    const QMap<int, std::string> &parameterDescriptions) {
  m_parameterDescriptions.clear();
  for (auto const prop : m_parameterMap.keys()) {
    auto const i = m_parameterMap[prop];
    m_parameterDescriptions[prop] = parameterDescriptions[i];
  }
}

void IqtTemplateBrowser::setErrorsEnabled(bool enabled) {
  ScopedFalse _false(m_emitParameterValueChange);
  m_parameterManager->setErrorsEnabled(enabled);
}

void IqtTemplateBrowser::clear() {
  removeBackground();
  removeStretchExponential();
  removeExponentialTwo();
  removeExponentialOne();
}

void IqtTemplateBrowser::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_presenter.updateParameterEstimationData(std::move(data));
}

void IqtTemplateBrowser::setBackgroundA0(double value) { m_presenter.setBackgroundA0(value); }

void IqtTemplateBrowser::popupMenu(const QPoint &) {
  std::cerr << "Popup" << std::endl;
}

double IqtTemplateBrowser::getParameterPropertyValue(QtProperty *prop) const {
  return prop ? m_parameterManager->value(prop) : 0.0;
}

void IqtTemplateBrowser::setParameterPropertyValue(QtProperty *prop,
                                                   double value, double error) {
  if (prop) {
    ScopedFalse _false(m_emitParameterValueChange);
    m_parameterManager->setValue(prop, value);
    m_parameterManager->setError(prop, error);
  }
}

void IqtTemplateBrowser::setGlobalParametersQuiet(const QStringList &globals) {
  ScopedFalse _false(m_emitParameterValueChange);
  auto parameterProperies = m_parameterMap.keys();
  for (auto const prop : m_parameterMap.keys()) {
    auto const name = m_actualParameterNames[prop];
    if (globals.contains(name)) {
      m_parameterManager->setGlobal(prop, true);
      parameterProperies.removeOne(prop);
    }
  }
  for (auto const prop : parameterProperies) {
    if (!m_actualParameterNames[prop].isEmpty()) {
      m_parameterManager->setGlobal(prop, false);
    }
  }
}

void IqtTemplateBrowser::setTieIntensitiesQuiet(bool on) {
  ScopedFalse _false(m_emitBoolChange);
  m_boolManager->setValue(m_tieIntensities, on);
}

void IqtTemplateBrowser::updateState() {
  auto const on = m_presenter.canTieIntensities();
  if (!on && m_boolManager->value(m_tieIntensities)) {
    ScopedFalse _false(m_emitBoolChange);
    m_boolManager->setValue(m_tieIntensities, false);
  }
  m_tieIntensities->setEnabled(on);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
