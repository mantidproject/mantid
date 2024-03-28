// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SingleFunctionTemplateView.h"
#include "../ParameterEstimation.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/PropertyWithValue.h"

#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
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

namespace MantidQt::CustomInterfaces::Inelastic {

SingleFunctionTemplateView::SingleFunctionTemplateView(TemplateBrowserCustomizations customizations)
    : FunctionTemplateView() {
  (void)customizations;
  init();
}

void SingleFunctionTemplateView::createProperties() {
  m_parameterManager->blockSignals(true);
  m_boolManager->blockSignals(true);
  m_enumManager->blockSignals(true);

  m_fitType = m_enumManager->addProperty("Fit Type");
  m_browser->addProperty(m_fitType);

  m_parameterManager->blockSignals(false);
  m_enumManager->blockSignals(false);
  m_boolManager->blockSignals(false);
}

void SingleFunctionTemplateView::setDataType(std::vector<std::string> const &allowedFunctionsList) {
  MantidQt::MantidWidgets::ScopedFalse _enumBlock(m_emitEnumChange);
  m_enumManager->setEnumNames(m_fitType, stdVectorToQStringList(allowedFunctionsList));
  m_enumManager->setValue(m_fitType, 0);
}

void SingleFunctionTemplateView::setEnumValue(int enumIndex) { setEnumSilent(m_fitType, enumIndex); }

void SingleFunctionTemplateView::addParameter(std::string const &parameterName,
                                              std::string const &parameterDescription) {
  auto newParameter = m_parameterManager->addProperty(QString::fromStdString(parameterName));
  m_parameterManager->setDescription(newParameter, parameterDescription);
  m_parameterManager->setDecimals(newParameter, 6);

  m_fitType->addSubProperty(newParameter);
  m_parameterMap.insert(parameterName, newParameter);
  m_parameterNames.insert(newParameter, parameterName);
}

void SingleFunctionTemplateView::enumChanged(QtProperty *prop) {
  if (!m_emitEnumChange)
    return;
  if (prop == m_fitType) {
    auto fitType = m_enumManager->enumNames(prop)[m_enumManager->value(prop)].toStdString();
    m_presenter->setFitType(fitType);
  }
}

void SingleFunctionTemplateView::parameterChanged(QtProperty *prop) {
  auto isGlobal = m_parameterManager->isGlobal(prop);
  m_presenter->setGlobal(m_parameterNames[prop], isGlobal);

  if (m_emitParameterValueChange) {
    m_presenter->handleParameterValueChanged(m_parameterNames[prop], m_parameterManager->value(prop));
  }
}

void SingleFunctionTemplateView::setParameterValue(std::string const &parameterName, double parameterValue,
                                                   double parameterError) {
  m_parameterManager->setValue(m_parameterMap[parameterName], parameterValue);
  m_parameterManager->setError(m_parameterMap[parameterName], parameterError);
}

void SingleFunctionTemplateView::setParameterValueQuietly(std::string const &parameterName, double parameterValue,
                                                          double parameterError) {
  setParameterSilent(m_parameterMap[parameterName], parameterValue, parameterError);
}

void SingleFunctionTemplateView::updateParameterNames(const std::map<int, std::string> &) {}

void SingleFunctionTemplateView::updateAvailableFunctions(
    const std::map<std::string, std::string> &functionInitialisationStrings) {
  m_presenter->updateAvailableFunctions(functionInitialisationStrings);
}

void SingleFunctionTemplateView::clear() {
  m_parameterManager->clear();
  m_parameterMap.clear();
  m_parameterNames.clear();
}

void SingleFunctionTemplateView::setGlobalParametersQuiet(std::vector<std::string> const &globals) {
  MantidQt::MantidWidgets::ScopedFalse _parameterBlock(m_emitParameterValueChange);
  for (auto const &parameterName : m_parameterMap.keys()) {
    auto const findIter = std::find(globals.cbegin(), globals.cend(), parameterName);
    m_parameterManager->setGlobal(m_parameterMap[parameterName], findIter != globals.cend());
  }
}

} // namespace MantidQt::CustomInterfaces::Inelastic
