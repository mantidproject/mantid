// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvTemplateBrowser.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

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

ConvTemplateBrowser::ConvTemplateBrowser(QWidget *parent)
    : FunctionTemplateBrowser(parent), m_presenter(this) {
  connect(&m_presenter, SIGNAL(functionStructureChanged()), this,
          SIGNAL(functionStructureChanged()));
}

void ConvTemplateBrowser::createProperties() {
  m_parameterManager->blockSignals(true);
  m_boolManager->blockSignals(true);
  m_enumManager->blockSignals(true);

  m_fitType = m_enumManager->addProperty("Fit Type");
  m_fitTypeNames = getFitTypes();
  m_enumManager->setEnumNames(m_fitType, m_fitTypeNames);
  m_browser->addProperty(m_fitType);

  m_backgroundType = m_enumManager->addProperty("Background");
  QStringList backgrounds;
  backgrounds << "None"
              << "FlatBackground"
              << "LinearBackground";
  m_enumManager->setEnumNames(m_backgroundType, backgrounds);
  m_browser->addProperty(m_backgroundType);

  m_parameterManager->blockSignals(false);
  m_enumManager->blockSignals(false);
  m_boolManager->blockSignals(false);
  //updateState();
}


void ConvTemplateBrowser::setFunction(const QString &funStr) {
  m_presenter.setFunction(funStr);
}

IFunction_sptr ConvTemplateBrowser::getGlobalFunction() const {
  return m_presenter.getGlobalFunction();
}

IFunction_sptr ConvTemplateBrowser::getFunction() const {
  return m_presenter.getFunction();
}

void ConvTemplateBrowser::setNumberOfDatasets(int n) {
  m_presenter.setNumberOfDatasets(n);
}

int ConvTemplateBrowser::getNumberOfDatasets() const {
  return m_presenter.getNumberOfDatasets();
}

void ConvTemplateBrowser::setDatasetNames(const QStringList &names) {
  m_presenter.setDatasetNames(names);
}

QStringList ConvTemplateBrowser::getGlobalParameters() const {
  return m_presenter.getGlobalParameters();
}

QStringList ConvTemplateBrowser::getLocalParameters() const {
  return m_presenter.getLocalParameters();
}

void ConvTemplateBrowser::setGlobalParameters(const QStringList &globals) {
  m_presenter.setGlobalParameters(globals);
}

void ConvTemplateBrowser::intChanged(QtProperty *prop) {
}

void ConvTemplateBrowser::boolChanged(QtProperty *prop) {
}

void ConvTemplateBrowser::enumChanged(QtProperty *prop) {
  if (prop == m_fitType) {
    auto const index = m_enumManager->value(prop);
    m_presenter.setFitType(m_fitTypeNames[index]);
  }
  if (prop == m_backgroundType) {
    auto background =
        m_enumManager->enumNames(prop)[m_enumManager->value(prop)];
    m_presenter.setBackground(background);
  }
}

void ConvTemplateBrowser::globalChanged(QtProperty *prop, const QString &name,
                                       bool on) {
  std::cerr << "Global " << name.toStdString() << ' ' << on << std::endl;
}

void ConvTemplateBrowser::parameterChanged(QtProperty *prop) {
  auto isGlobal = m_parameterManager->isGlobal(prop);
  m_presenter.setGlobal(m_actualParameterNames[prop], isGlobal);
  if (m_emitParameterValueChange) {
    emit parameterValueChanged(m_actualParameterNames[prop],
                               m_parameterManager->value(prop));
  }
}

void ConvTemplateBrowser::parameterButtonClicked(QtProperty *prop) {
  emit localParameterButtonClicked(m_actualParameterNames[prop]);
}

void ConvTemplateBrowser::updateMultiDatasetParameters(const IFunction &fun) {
  m_presenter.updateMultiDatasetParameters(fun);
}

void ConvTemplateBrowser::updateMultiDatasetParameters(
    const ITableWorkspace &paramTable) {
  m_presenter.updateMultiDatasetParameters(paramTable);
}

void ConvTemplateBrowser::updateParameters(const IFunction &fun) {
  m_presenter.updateParameters(fun);
}

void ConvTemplateBrowser::setCurrentDataset(int i) {
  m_presenter.setCurrentDataset(i);
}

void ConvTemplateBrowser::updateParameterNames(
    const QMap<int, QString> &parameterNames) {
  m_actualParameterNames.clear();
  ScopedFalse _(m_emitParameterValueChange);
  for (auto const prop : m_parameterMap.keys()) {
    auto const i = m_parameterMap[prop];
    auto const name = parameterNames[i];
    m_actualParameterNames[prop] = name;
    if (!name.isEmpty()) {
      prop->setPropertyName(name);
    }
  }
}

void ConvTemplateBrowser::updateParameterDescriptions(
    const QMap<int, std::string> &parameterDescriptions) {
  m_parameterDescriptions.clear();
  for (auto const prop : m_parameterMap.keys()) {
    auto const i = m_parameterMap[prop];
    m_parameterDescriptions[prop] = parameterDescriptions[i];
  }
}

void ConvTemplateBrowser::setErrorsEnabled(bool enabled) {
  ScopedFalse _(m_emitParameterValueChange);
  m_parameterManager->setErrorsEnabled(enabled);
}

void ConvTemplateBrowser::clear() {
  //removeBackground();
  //removeStretchExponential();
  //removeExponentialTwo();
  //removeExponentialOne();
}

void ConvTemplateBrowser::popupMenu(const QPoint &) {
  std::cerr << "Popup" << std::endl;
}

void ConvTemplateBrowser::setParameterPropertyValue(QtProperty *prop,
                                                   double value, double error) {
  if (prop) {
    ScopedFalse _(m_emitParameterValueChange);
    m_parameterManager->setValue(prop, value);
    m_parameterManager->setError(prop, error);
  }
}

void ConvTemplateBrowser::setGlobalParametersQuiet(const QStringList &globals) {
  ScopedFalse _(m_emitParameterValueChange);
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

void ConvTemplateBrowser::createFunctionParameterProperties() {
  for(auto const fitType: m_fitTypeNames) {
    QStringList names;
    QStringList descriptions;
    QList<int> ids;
    fillParameterLists(fitType, names, descriptions, ids);
  }

  //QList<QtProperty *> props;
  //auto const fun =
  //    FunctionFactory::Instance().createInitialized(funStr.toStdString());
  //auto const np = fun->nParams();
  //for (size_t i = 0; i < np; ++i) {
  //  auto prop = m_parameterManager->addProperty(QString::fromStdString(fun->parameterName(i)));
  //  m_parameterManager->setDecimals(prop, 6);
  //}
  //return props;
}

void ConvTemplateBrowser::
    updateParameterEstimationData(DataForParameterEstimationCollection &&data) {}

void ConvTemplateBrowser::setBackgroundA0(double value) {}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
