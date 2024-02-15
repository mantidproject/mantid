// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SingleFunctionTemplateBrowser.h"
#include "Analysis/IDAFunctionParameterEstimation.h"

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

namespace MantidQt::CustomInterfaces::IDA {

/**
 * Constructor
 * @param parent :: The parent widget.
 */
SingleFunctionTemplateBrowser::SingleFunctionTemplateBrowser(QWidget *parent) : FunctionTemplateBrowser(parent) {
  init();
}

void SingleFunctionTemplateBrowser::createProperties() {
  m_parameterManager->blockSignals(true);
  m_boolManager->blockSignals(true);
  m_enumManager->blockSignals(true);

  m_fitType = m_enumManager->addProperty("Fit Type");
  m_browser->addProperty(m_fitType);

  m_parameterManager->blockSignals(false);
  m_enumManager->blockSignals(false);
  m_boolManager->blockSignals(false);
}

void SingleFunctionTemplateBrowser::setDataType(std::vector<std::string> const &allowedFunctionsList) {
  MantidQt::MantidWidgets::ScopedFalse _enumBlock(m_emitEnumChange);
  m_enumManager->setEnumNames(m_fitType, stdVectorToQStringList(allowedFunctionsList));
  m_enumManager->setValue(m_fitType, 0);
}

void SingleFunctionTemplateBrowser::setEnumValue(int enumIndex) { setEnumSilent(m_fitType, enumIndex); }

void SingleFunctionTemplateBrowser::addParameter(std::string const &parameterName,
                                                 std::string const &parameterDescription) {
  auto newParameter = m_parameterManager->addProperty(QString::fromStdString(parameterName));
  m_parameterManager->setDescription(newParameter, parameterDescription);
  m_parameterManager->setDecimals(newParameter, 6);

  m_fitType->addSubProperty(newParameter);
  m_parameterMap.insert(parameterName, newParameter);
  m_parameterNames.insert(newParameter, parameterName);
}

void SingleFunctionTemplateBrowser::enumChanged(QtProperty *prop) {
  if (!m_emitEnumChange)
    return;
  if (prop == m_fitType) {
    auto fitType = m_enumManager->enumNames(prop)[m_enumManager->value(prop)].toStdString();
    m_presenter->setFitType(fitType);
  }
}

void SingleFunctionTemplateBrowser::globalChanged(QtProperty *, const QString &, bool) {}

void SingleFunctionTemplateBrowser::parameterChanged(QtProperty *prop) {
  auto isGlobal = m_parameterManager->isGlobal(prop);
  m_presenter->setGlobal(m_parameterNames[prop], isGlobal);

  if (m_emitParameterValueChange) {
    m_presenter->handleParameterValueChanged(m_parameterNames[prop], m_parameterManager->value(prop));
  }
}

void SingleFunctionTemplateBrowser::updateMultiDatasetParameters(const ITableWorkspace &) {}

void SingleFunctionTemplateBrowser::updateParameters(const IFunction &fun) { m_presenter->updateParameters(fun); }

void SingleFunctionTemplateBrowser::setParameterValue(std::string const &parameterName, double parameterValue,
                                                      double parameterError) {
  m_parameterManager->setValue(m_parameterMap[parameterName], parameterValue);
  m_parameterManager->setError(m_parameterMap[parameterName], parameterError);
}

void SingleFunctionTemplateBrowser::setParameterValueQuietly(std::string const &parameterName, double parameterValue,
                                                             double parameterError) {
  setParameterSilent(m_parameterMap[parameterName], parameterValue, parameterError);
}

void SingleFunctionTemplateBrowser::updateParameterNames(const QMap<int, std::string> &) {}

void SingleFunctionTemplateBrowser::updateParameterDescriptions(const QMap<int, std::string> &) {}

void SingleFunctionTemplateBrowser::updateAvailableFunctions(
    const std::map<std::string, std::string> &functionInitialisationStrings) {
  m_presenter->updateAvailableFunctions(functionInitialisationStrings);
}

void SingleFunctionTemplateBrowser::clear() {
  m_parameterManager->clear();
  m_parameterMap.clear();
  m_parameterNames.clear();
}

EstimationDataSelector SingleFunctionTemplateBrowser::getEstimationDataSelector() const {
  return m_presenter->getEstimationDataSelector();
}

void SingleFunctionTemplateBrowser::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_presenter->updateParameterEstimationData(std::move(data));
}

void SingleFunctionTemplateBrowser::estimateFunctionParameters() { m_presenter->estimateFunctionParameters(); }

void SingleFunctionTemplateBrowser::popupMenu(const QPoint &) {}

void SingleFunctionTemplateBrowser::setGlobalParametersQuiet(std::vector<std::string> const &globals) {
  MantidQt::MantidWidgets::ScopedFalse _parameterBlock(m_emitParameterValueChange);
  for (auto const &parameterName : m_parameterMap.keys()) {
    auto const findIter = std::find(globals.cbegin(), globals.cend(), parameterName);
    m_parameterManager->setGlobal(m_parameterMap[parameterName], findIter != globals.cend());
  }
}

void SingleFunctionTemplateBrowser::setBackgroundA0(double) {}
void SingleFunctionTemplateBrowser::setResolution(const std::vector<std::pair<std::string, size_t>> &) {}
void SingleFunctionTemplateBrowser::setQValues(const std::vector<double> &) {}

} // namespace MantidQt::CustomInterfaces::IDA
