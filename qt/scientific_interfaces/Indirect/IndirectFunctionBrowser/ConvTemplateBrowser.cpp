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
  m_templateSubTypes.emplace_back(std::make_unique<FitSubType>());
  m_templateSubTypes.emplace_back(std::make_unique<BackgroundSubType>());
  connect(&m_presenter, SIGNAL(functionStructureChanged()), this,
          SIGNAL(functionStructureChanged()));
}

void ConvTemplateBrowser::createProperties() {
  m_parameterManager->blockSignals(true);
  m_boolManager->blockSignals(true);
  m_enumManager->blockSignals(true);

  createFunctionParameterProperties();

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
  auto const index = m_enumManager->value(prop);
  auto propIt =
      std::find(m_subTypeProperties.begin(), m_subTypeProperties.end(), prop);
  if (propIt != m_subTypeProperties.end()) {
    auto const subTypeIndex =
        std::distance(m_subTypeProperties.begin(), propIt);
    m_presenter.setSubType(subTypeIndex, index);
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
  ScopedFalse _false(m_emitParameterValueChange);
  for (auto const prop : m_parameterMap.keys()) {
    auto const i = m_parameterMap[prop];
    auto const name = parameterNames[static_cast<int>(i)];
    m_actualParameterNames[prop] = name;
    if (!name.isEmpty()) {
      prop->setPropertyName(name);
    }
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
  m_subTypeParameters.resize(m_templateSubTypes.size());
  m_currentSubTypeParameters.resize(m_templateSubTypes.size());
  for (size_t isub = 0; isub < m_templateSubTypes.size(); ++isub) {
    auto const &subType = m_templateSubTypes[isub];
    auto &parameters = m_subTypeParameters[isub];
    for (int index = 0; index < subType->getNTypes(); ++index) {
      auto const paramIDs = subType->getParameterIDs(index);
      auto const names = subType->getParameterNames(index);
      auto const descriptions = subType->getParameterDescriptions(index);
      QList<QtProperty *> props;
      auto const np = names.size();
      for (int i = 0; i < np; ++i) {
        auto prop = m_parameterManager->addProperty(names[i]);
        m_parameterManager->setDescription(prop, descriptions[i]);
        m_parameterManager->setDecimals(prop, 6);
        props << prop;
        m_parameterMap[prop] = paramIDs[i];
      }
      parameters[index] = props;
    }
    auto subTypeProp = m_enumManager->addProperty(subType->name());
    m_enumManager->setEnumNames(subTypeProp,
                                m_templateSubTypes[isub]->getTypeNames());
    m_browser->addProperty(subTypeProp);
    m_subTypeProperties.push_back(subTypeProp);
  }
}

void ConvTemplateBrowser::setSubType(size_t subTypeIndex, int typeIndex) {
  auto subTypeProp = m_subTypeProperties[subTypeIndex];
  auto &currentParameters = m_currentSubTypeParameters[subTypeIndex];
  for (auto &&prop : currentParameters) {
    subTypeProp->removeSubProperty(prop);
  }
  currentParameters.clear();
  auto &subTypeParameters = m_subTypeParameters[subTypeIndex];
  for (auto &&prop : subTypeParameters[typeIndex]) {
    subTypeProp->addSubProperty(prop);
    currentParameters.append(prop);
  }
}

void ConvTemplateBrowser::
    updateParameterEstimationData(DataForParameterEstimationCollection &&data) {}

void ConvTemplateBrowser::setBackgroundA0(double value) {}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
