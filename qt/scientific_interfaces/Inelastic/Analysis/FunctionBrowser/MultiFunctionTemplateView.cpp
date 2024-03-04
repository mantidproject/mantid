// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MultiFunctionTemplateView.h"
#include "MultiFunctionTemplatePresenter.h"

#include "MantidAPI/IFunction.h"

#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

namespace MantidQt::CustomInterfaces::IDA {

MultiFunctionTemplateView::MultiFunctionTemplateView(TemplateBrowserCustomizations customizations)
    : FunctionTemplateView(), m_templateSubTypes(std::move(*customizations.templateSubTypes)) {
  init();
}

void MultiFunctionTemplateView::createProperties() {
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

void MultiFunctionTemplateView::updateParameterNames(const QMap<int, std::string> &parameterNames) {
  m_parameterNames.clear();
  MantidQt::MantidWidgets::ScopedFalse _paramBlock(m_emitParameterValueChange);
  for (auto const prop : m_parameterMap.keys()) {
    auto const name = parameterNames[static_cast<int>(m_parameterMap[prop])];
    m_parameterNames[prop] = name;
    if (!name.empty()) {
      prop->setPropertyName(QString::fromStdString(name));
    }
  }
}

void MultiFunctionTemplateView::setGlobalParametersQuiet(std::vector<std::string> const &globals) {
  MantidQt::MantidWidgets::ScopedFalse _paramBlock(m_emitParameterValueChange);
  for (auto const prop : m_parameterMap.keys()) {
    auto const parameterName = m_parameterNames[prop];
    auto const findIter = std::find(globals.cbegin(), globals.cend(), parameterName);
    m_parameterManager->setGlobal(prop, findIter != globals.cend());
  }
}

void MultiFunctionTemplateView::createFunctionParameterProperties() {
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
    QtProperty *subTypeProp;
    if (subType->isType(typeid(int))) {
      subTypeProp = m_intManager->addProperty(QString::fromStdString(subType->name()));
      m_intManager->setMinimum(subTypeProp, 0);
      m_intManager->setMaximum(subTypeProp, subType->getNTypes() - 1);
    } else if (subType->isType(typeid(bool))) {
      subTypeProp = m_boolManager->addProperty(QString::fromStdString(subType->name()));
    } else {
      subTypeProp = m_enumManager->addProperty(QString::fromStdString(subType->name()));
      m_enumManager->setEnumNames(subTypeProp, subType->getTypeNames());
    }
    m_subTypeProperties.push_back(subTypeProp);
  }
}

void MultiFunctionTemplateView::setProperty(std::size_t subTypeIndex, int value) {
  auto const &subType = m_templateSubTypes[subTypeIndex];
  if (subType->isType(typeid(int))) {
    setIntSilent(m_subTypeProperties[subTypeIndex], value);
  } else if (subType->isType(typeid(bool))) {
    setBoolSilent(m_subTypeProperties[subTypeIndex], value);
  } else {
    setEnumSilent(m_subTypeProperties[subTypeIndex], value);
  }
}

void MultiFunctionTemplateView::setSubTypes(std::map<std::size_t, int> const &subTypes) {
  for (auto it = subTypes.cbegin(); it != subTypes.cend(); ++it) {
    setSubType(it->first, it->second);
  }
}

void MultiFunctionTemplateView::setSubType(std::size_t subTypeIndex, int typeIndex) {
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
  setProperty(subTypeIndex, typeIndex);
}

void MultiFunctionTemplateView::setParameterValueQuiet(ParamID id, double value, double error) {
  auto prop = m_parameterReverseMap[id];
  setParameterSilent(prop, value, error);
}

void MultiFunctionTemplateView::intChanged(QtProperty *prop) {
  if (!m_emitIntChange)
    return;

  if (auto const index = propertySubTypeIndex(prop)) {
    m_presenter->setSubType(*index, m_intManager->value(prop));
  }
}

void MultiFunctionTemplateView::boolChanged(QtProperty *prop) {
  if (!m_emitBoolChange)
    return;

  if (auto const index = propertySubTypeIndex(prop)) {
    m_presenter->setSubType(*index, m_boolManager->value(prop));
  }
}

void MultiFunctionTemplateView::enumChanged(QtProperty *prop) {
  if (!m_emitEnumChange)
    return;

  if (auto const index = propertySubTypeIndex(prop)) {
    m_presenter->setSubType(*index, m_enumManager->value(prop));
  }
}

void MultiFunctionTemplateView::parameterChanged(QtProperty *prop) {
  if (!m_emitParameterValueChange) {
    return;
  }

  m_presenter->setGlobal(m_parameterNames[prop], m_parameterManager->isGlobal(prop));
  m_presenter->handleParameterValueChanged(m_parameterNames[prop], m_parameterManager->value(prop));
}

std::optional<std::size_t> MultiFunctionTemplateView::propertySubTypeIndex(QtProperty *prop) {
  auto const it = std::find(m_subTypeProperties.cbegin(), m_subTypeProperties.cend(), prop);
  if (it != m_subTypeProperties.cend()) {
    std::size_t index = std::distance(m_subTypeProperties.cbegin(), it);
    return index;
  }
  return std::nullopt;
}

} // namespace MantidQt::CustomInterfaces::IDA
