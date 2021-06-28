// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SingleFunctionTemplateBrowser.h"
#include "IDAFunctionParameterEstimation.h"

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
SingleFunctionTemplateBrowser::SingleFunctionTemplateBrowser(
    const std::map<std::string, std::string> &functionInitialisationStrings,
    std::unique_ptr<IDAFunctionParameterEstimation> parameterEstimation, QWidget *parent)
    : FunctionTemplateBrowser(parent),
      m_presenter(this, functionInitialisationStrings, std::move(parameterEstimation)) {
  connect(&m_presenter, SIGNAL(functionStructureChanged()), this, SIGNAL(functionStructureChanged()));
}

void SingleFunctionTemplateBrowser::updateAvailableFunctions(
    const std::map<std::string, std::string> &functionInitialisationStrings) {
  m_presenter.updateAvailableFunctions(functionInitialisationStrings);
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

  m_presenter.init();
}

void SingleFunctionTemplateBrowser::setDataType(const QStringList &allowedFunctionsList) {
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setEnumNames(m_fitType, allowedFunctionsList);
  m_enumManager->setValue(m_fitType, 0);
}

void SingleFunctionTemplateBrowser::setEnumValue(int enumIndex) {
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_fitType, enumIndex);
}

void SingleFunctionTemplateBrowser::addParameter(const QString &parameterName, const QString &parameterDescription) {
  auto newParameter = m_parameterManager->addProperty(parameterName);
  m_parameterManager->setDescription(newParameter, parameterDescription.toStdString());
  m_parameterManager->setDecimals(newParameter, 6);

  m_fitType->addSubProperty(newParameter);
  m_parameterMap.insert(parameterName, newParameter);
  m_parameterNames.insert(newParameter, parameterName);
}

int SingleFunctionTemplateBrowser::getCurrentDataset() { return m_presenter.getCurrentDataset(); }

void SingleFunctionTemplateBrowser::setFunction(const QString &funStr) { m_presenter.setFunction(funStr); }

IFunction_sptr SingleFunctionTemplateBrowser::getGlobalFunction() const { return m_presenter.getGlobalFunction(); }

IFunction_sptr SingleFunctionTemplateBrowser::getFunction() const { return m_presenter.getFunction(); }

void SingleFunctionTemplateBrowser::setNumberOfDatasets(int n) { m_presenter.setNumberOfDatasets(n); }

int SingleFunctionTemplateBrowser::getNumberOfDatasets() const { return m_presenter.getNumberOfDatasets(); }

void SingleFunctionTemplateBrowser::setDatasets(const QList<FunctionModelDataset> &datasets) {
  m_presenter.setDatasets(datasets);
}

QStringList SingleFunctionTemplateBrowser::getGlobalParameters() const { return m_presenter.getGlobalParameters(); }

QStringList SingleFunctionTemplateBrowser::getLocalParameters() const { return m_presenter.getLocalParameters(); }

void SingleFunctionTemplateBrowser::setGlobalParameters(const QStringList &globals) {
  m_presenter.setGlobalParameters(globals);
}

void SingleFunctionTemplateBrowser::enumChanged(QtProperty *prop) {
  if (!m_emitEnumChange)
    return;
  if (prop == m_fitType) {
    auto fitType = m_enumManager->enumNames(prop)[m_enumManager->value(prop)];
    m_presenter.setFitType(fitType);
  }
}

void SingleFunctionTemplateBrowser::globalChanged(QtProperty *, const QString &, bool) {}

void SingleFunctionTemplateBrowser::parameterChanged(QtProperty *prop) {
  auto isGlobal = m_parameterManager->isGlobal(prop);
  m_presenter.setGlobal(m_parameterNames[prop], isGlobal);
  if (m_emitParameterValueChange) {
    emit parameterValueChanged(m_parameterNames[prop], m_parameterManager->value(prop));
  }
}

void SingleFunctionTemplateBrowser::parameterButtonClicked(QtProperty *prop) {
  emit localParameterButtonClicked(m_parameterNames[prop]);
}

void SingleFunctionTemplateBrowser::updateMultiDatasetParameters(const IFunction &fun) {
  m_presenter.updateMultiDatasetParameters(fun);
}

void SingleFunctionTemplateBrowser::updateMultiDatasetParameters(const ITableWorkspace &) {}

void SingleFunctionTemplateBrowser::updateParameters(const IFunction &fun) { m_presenter.updateParameters(fun); }

void SingleFunctionTemplateBrowser::setParameterValue(const QString &parameterName, double parameterValue,
                                                      double parameterError) {
  m_parameterManager->setValue(m_parameterMap[parameterName], parameterValue);
  m_parameterManager->setError(m_parameterMap[parameterName], parameterError);
}

void SingleFunctionTemplateBrowser::setParameterValueQuietly(const QString &parameterName, double parameterValue,
                                                             double parameterError) {
  ScopedFalse _(m_emitParameterValueChange);
  m_parameterManager->setValue(m_parameterMap[parameterName], parameterValue);
  m_parameterManager->setError(m_parameterMap[parameterName], parameterError);
}

void SingleFunctionTemplateBrowser::setCurrentDataset(int i) { m_presenter.setCurrentDataset(i); }

void SingleFunctionTemplateBrowser::updateParameterNames(const QMap<int, QString> &) {}

void SingleFunctionTemplateBrowser::updateParameterDescriptions(const QMap<int, std::string> &) {}

void SingleFunctionTemplateBrowser::setErrorsEnabled(bool enabled) {
  ScopedFalse _false(m_emitParameterValueChange);
  m_parameterManager->setErrorsEnabled(enabled);
}

void SingleFunctionTemplateBrowser::clear() {
  m_parameterManager->clear();
  m_parameterMap.clear();
  m_parameterNames.clear();
}

void SingleFunctionTemplateBrowser::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_presenter.updateParameterEstimationData(std::move(data));
}
void SingleFunctionTemplateBrowser::estimateFunctionParameters() { m_presenter.estimateFunctionParameters(); }

void SingleFunctionTemplateBrowser::popupMenu(const QPoint &) {}

void SingleFunctionTemplateBrowser::setParameterPropertyValue(QtProperty *prop, double value, double error) {
  if (prop) {
    ScopedFalse _false(m_emitParameterValueChange);
    m_parameterManager->setValue(prop, value);
    m_parameterManager->setError(prop, error);
  }
}

void SingleFunctionTemplateBrowser::setGlobalParametersQuiet(const QStringList &globals) {
  ScopedFalse _false(m_emitParameterValueChange);
  for (auto &parameterName : m_parameterMap.keys()) {
    if (globals.contains(parameterName)) {
      m_parameterManager->setGlobal(m_parameterMap[parameterName], true);
    } else {
      m_parameterManager->setGlobal(m_parameterMap[parameterName], false);
    }
  }
}

void SingleFunctionTemplateBrowser::setBackgroundA0(double) {}
void SingleFunctionTemplateBrowser::setResolution(const std::vector<std::pair<std::string, size_t>> &) {}
void SingleFunctionTemplateBrowser::setQValues(const std::vector<double> &) {}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
