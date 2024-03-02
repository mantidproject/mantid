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

  for (auto &prop : m_subTypeProperties)
    m_browser->addProperty(prop);

  m_parameterManager->blockSignals(false);
  m_enumManager->blockSignals(false);
  m_boolManager->blockSignals(false);
  m_intManager->blockSignals(false);
}

void MultiFunctionTemplateBrowser::boolChanged(QtProperty *prop) {
  if (!m_emitBoolChange)
    return;

  if (auto const index = propertySubTypeIndex(prop)) {
    m_presenter->setSubType(*index, m_boolManager->value(prop));
  }
}

void MultiFunctionTemplateBrowser::enumChanged(QtProperty *prop) {
  if (!m_emitEnumChange)
    return;

  if (auto const index = propertySubTypeIndex(prop)) {
    m_presenter->setSubType(*index, m_enumManager->value(prop));
  }
}

void MultiFunctionTemplateBrowser::parameterChanged(QtProperty *prop) {
  if (!m_emitParameterValueChange) {
    return;
  }

  m_presenter->setGlobal(m_parameterNames[prop], m_parameterManager->isGlobal(prop));
  m_presenter->handleParameterValueChanged(m_parameterNames[prop], m_parameterManager->value(prop));
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

    if (m_templateSubTypes[isub]->isType(typeid(int))) {
      auto subtypeProp = m_intManager->addProperty(QString::fromStdString(subType->name()));
      m_intManager->setMinimum(subtypeProp, 0);
      m_intManager->setMaximum(subtypeProp, m_templateSubTypes[isub]->getNTypes() - 1);
      m_subTypeProperties.push_back(subtypeProp);

    } else if (m_templateSubTypes[isub]->isType(typeid(bool))) {
      auto subtypeProp = m_boolManager->addProperty(QString::fromStdString(subType->name()));
      m_subTypeProperties.push_back(subtypeProp);
    } else {
      auto subTypeProp = m_enumManager->addProperty(QString::fromStdString(subType->name()));
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

void MultiFunctionTemplateBrowser::intChanged(QtProperty *prop) {
  if (!m_emitIntChange)
    return;

  if (auto const index = propertySubTypeIndex(prop)) {
    m_presenter->setSubType(*index, m_intManager->value(prop));
  }
}

std::optional<std::size_t> MultiFunctionTemplateBrowser::propertySubTypeIndex(QtProperty *prop) {
  auto const it = std::find(m_subTypeProperties.cbegin(), m_subTypeProperties.cend(), prop);
  if (it != m_subTypeProperties.cend()) {
    std::size_t index = std::distance(m_subTypeProperties.cbegin(), it);
    return index;
  }
  return std::nullopt;
}

} // namespace MantidQt::CustomInterfaces::IDA
