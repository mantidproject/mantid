// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MultiFunctionTemplateBrowser.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

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

MultiFunctionTemplateBrowser::MultiFunctionTemplateBrowser(TemplateBrowserCustomizations customizations)
    : FunctionTemplateBrowser(), m_templateSubTypes(std::move(*customizations.templateSubTypes)) {
  init();
}

void MultiFunctionTemplateBrowser::createProperties() {
  m_parameterManager->blockSignals(true);
  m_boolManager->blockSignals(true);
  m_enumManager->blockSignals(true);
  m_intManager->blockSignals(true);

  createFunctionParameterProperties();
  createTempCorrectionProperties();

  m_browser->addProperty(m_subTypeProperties[ConvTypes::SubTypeIndex::Lorentzian]);
  m_browser->addProperty(m_subTypeProperties[ConvTypes::SubTypeIndex::Fit]);
  m_browser->addProperty(m_subTypeProperties[ConvTypes::SubTypeIndex::Delta]);
  m_browser->addProperty(m_tempCorrectionOn);
  m_browser->addProperty(m_subTypeProperties[ConvTypes::SubTypeIndex::Background]);

  m_parameterManager->blockSignals(false);
  m_enumManager->blockSignals(false);
  m_boolManager->blockSignals(false);
  m_intManager->blockSignals(false);
}

void MultiFunctionTemplateBrowser::boolChanged(QtProperty *prop) {
  if (!m_emitBoolChange)
    return;
  if (prop == m_subTypeProperties[ConvTypes::SubTypeIndex::Delta]) {
    m_presenter->setSubType(ConvTypes::SubTypeIndex::Delta, static_cast<int>(m_boolManager->value(prop)));
  } else if (prop == m_tempCorrectionOn) {
    m_presenter->setTempCorrection(m_boolManager->value(prop));
  }
}

void MultiFunctionTemplateBrowser::setQValues(const std::vector<double> &qValues) { m_presenter->setQValues(qValues); }

void MultiFunctionTemplateBrowser::addTempCorrection(double value) {
  m_tempCorrectionOn->addSubProperty(m_temperature);
  setBoolSilent(m_tempCorrectionOn, true);
  m_parameterManager->setValue(m_temperature, value);
  m_parameterManager->setGlobal(m_temperature, true);
}

void MultiFunctionTemplateBrowser::updateTemperatureCorrectionAndDelta(bool tempCorrection) {
  MantidQt::MantidWidgets::ScopedFalse _boolBlock(m_emitBoolChange);
  MantidQt::MantidWidgets::ScopedFalse _paramBlock(m_emitParameterValueChange);

  if (tempCorrection)
    addTempCorrection(100.0);
  else
    removeTempCorrection();
}

void MultiFunctionTemplateBrowser::removeTempCorrection() {
  m_tempCorrectionOn->removeSubProperty(m_temperature);
  setBoolSilent(m_tempCorrectionOn, false);
}

void MultiFunctionTemplateBrowser::enumChanged(QtProperty *prop) {
  if (!m_emitEnumChange)
    return;
  auto const index = m_enumManager->value(prop);
  auto propIt = std::find(m_subTypeProperties.begin(), m_subTypeProperties.end(), prop);
  if (propIt != m_subTypeProperties.end()) {
    auto const subTypeIndex = std::distance(m_subTypeProperties.begin(), propIt);
    m_presenter->setSubType(subTypeIndex, index);
  }
}

void MultiFunctionTemplateBrowser::parameterChanged(QtProperty *prop) {
  auto isGlobal = m_parameterManager->isGlobal(prop);
  m_presenter->setGlobal(m_parameterNames[prop], isGlobal);
  if (m_emitParameterValueChange) {
    m_presenter->handleParameterValueChanged(m_parameterNames[prop], m_parameterManager->value(prop));
  }
}

void MultiFunctionTemplateBrowser::updateParameterNames(const QMap<int, std::string> &parameterNames) {
  m_parameterNames.clear();
  MantidQt::MantidWidgets::ScopedFalse _paramBlock(m_emitParameterValueChange);
  for (auto const prop : m_parameterMap.keys()) {
    auto const i = m_parameterMap[prop];
    auto const name = parameterNames[static_cast<int>(i)];
    m_parameterNames[prop] = name;
    if (!name.empty()) {
      prop->setPropertyName(QString::fromStdString(name));
    }
  }
}

void MultiFunctionTemplateBrowser::setGlobalParametersQuiet(std::vector<std::string> const &globals) {
  MantidQt::MantidWidgets::ScopedFalse _paramBlock(m_emitParameterValueChange);
  for (auto const prop : m_parameterMap.keys()) {
    auto const parameterName = m_parameterNames[prop];
    auto const findIter = std::find(globals.cbegin(), globals.cend(), parameterName);
    m_parameterManager->setGlobal(prop, findIter != globals.cend());
  }
}

void MultiFunctionTemplateBrowser::createFunctionParameterProperties() {
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
      for (auto i = 0u; i < names.size(); ++i) {
        auto prop = m_parameterManager->addProperty(QString::fromStdString(names[i]));
        m_parameterManager->setDescription(prop, descriptions[i]);
        m_parameterManager->setDecimals(prop, 6);
        props.append(prop);
        auto const id = paramIDs[i];
        m_parameterMap[prop] = id;
        m_parameterReverseMap[id] = prop;
      }
      parameters[index] = props;
    }
    if (isub == ConvTypes::SubTypeIndex::Lorentzian) {
      auto subtypeProp = m_intManager->addProperty(QString::fromStdString(subType->name()));
      m_intManager->setMinimum(subtypeProp, 0);
      m_intManager->setMaximum(subtypeProp, 2);
      m_subTypeProperties.push_back(subtypeProp);

    } else if (isub == ConvTypes::SubTypeIndex::Delta) {
      auto subtypeProp = m_boolManager->addProperty(QString::fromStdString(subType->name()));
      m_subTypeProperties.push_back(subtypeProp);
    } else {
      auto subTypeProp = m_enumManager->addProperty(QString::fromStdString(subType->name()));
      m_enumManager->setEnumNames(subTypeProp, m_templateSubTypes[isub]->getTypeNames());
      m_enumManager->setEnumNames(subTypeProp, m_templateSubTypes[isub]->getTypeNames());
      m_subTypeProperties.push_back(subTypeProp);
    }
  }
}

void MultiFunctionTemplateBrowser::setEnum(size_t subTypeIndex, int enumIndex) {
  setEnumSilent(m_subTypeProperties[subTypeIndex], enumIndex);
}

void MultiFunctionTemplateBrowser::setBool(size_t subTypeIndex, int enumIndex) {
  setBoolSilent(m_subTypeProperties[subTypeIndex], enumIndex);
}

void MultiFunctionTemplateBrowser::setInt(size_t subTypeIndex, int value) {
  setIntSilent(m_subTypeProperties[subTypeIndex], value);
}

void MultiFunctionTemplateBrowser::createTempCorrectionProperties() {
  m_tempCorrectionOn = m_boolManager->addProperty("Temp Correction");
  m_temperature = m_parameterManager->addProperty("Temperature");
  m_parameterManager->setDescription(m_temperature, "Temperature");
  m_parameterMap[m_temperature] = ParamID::TEMPERATURE;
  m_parameterReverseMap[ParamID::TEMPERATURE] = m_temperature;
}

void MultiFunctionTemplateBrowser::setSubType(size_t subTypeIndex, int typeIndex) {
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

void MultiFunctionTemplateBrowser::setParameterValueQuiet(ParamID id, double value, double error) {
  auto prop = m_parameterReverseMap[id];
  setParameterSilent(prop, value, error);
}

void MultiFunctionTemplateBrowser::setBackgroundA0(double value) { m_presenter->setBackgroundA0(value); }

void MultiFunctionTemplateBrowser::setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  m_presenter->setResolution(fitResolutions);
}

void MultiFunctionTemplateBrowser::intChanged(QtProperty *prop) {
  if (prop == m_subTypeProperties[ConvTypes::SubTypeIndex::Lorentzian] && m_emitIntChange) {
    m_presenter->setSubType(ConvTypes::SubTypeIndex::Lorentzian, m_intManager->value(prop));
  }
}
} // namespace MantidQt::CustomInterfaces::IDA
