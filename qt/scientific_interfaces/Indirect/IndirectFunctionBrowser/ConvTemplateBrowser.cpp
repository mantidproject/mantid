// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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

#include <limits>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

namespace {

class ScopedFalse {
  bool &m_ref;
  bool m_oldValue;

public:
  // this sets the input bool to false whilst this object is in scope and then
  // resets it to its old value when this object drops out of scope.
  explicit ScopedFalse(bool &variable) : m_ref(variable), m_oldValue(variable) { m_ref = false; }
  ~ScopedFalse() { m_ref = m_oldValue; }
};

} // namespace

ConvTemplateBrowser::ConvTemplateBrowser(QWidget *parent) : FunctionTemplateBrowser(parent), m_presenter(this) {
  m_templateSubTypes.emplace_back(std::make_unique<LorentzianSubType>());
  m_templateSubTypes.emplace_back(std::make_unique<FitSubType>());
  m_templateSubTypes.emplace_back(std::make_unique<BackgroundSubType>());
  connect(&m_presenter, SIGNAL(functionStructureChanged()), this, SIGNAL(functionStructureChanged()));
}

void ConvTemplateBrowser::createProperties() {
  m_parameterManager->blockSignals(true);
  m_boolManager->blockSignals(true);
  m_enumManager->blockSignals(true);
  m_intManager->blockSignals(true);

  createFunctionParameterProperties();
  createDeltaFunctionProperties();
  createTempCorrectionProperties();

  m_browser->addProperty(m_subTypeProperties[SubTypeIndex::Lorentzian]);
  m_browser->addProperty(m_subTypeProperties[SubTypeIndex::Fit]);
  m_browser->addProperty(m_deltaFunctionOn);
  m_browser->addProperty(m_tempCorrectionOn);
  m_browser->addProperty(m_subTypeProperties[SubTypeIndex::Background]);

  m_parameterManager->blockSignals(false);
  m_enumManager->blockSignals(false);
  m_boolManager->blockSignals(false);
  m_intManager->blockSignals(false);
}

void ConvTemplateBrowser::setFunction(const QString &funStr) { m_presenter.setFunction(funStr); }

IFunction_sptr ConvTemplateBrowser::getGlobalFunction() const { return m_presenter.getGlobalFunction(); }

IFunction_sptr ConvTemplateBrowser::getFunction() const { return m_presenter.getFunction(); }

int ConvTemplateBrowser::getCurrentDataset() { return m_presenter.getCurrentDataset(); }

void ConvTemplateBrowser::setNumberOfDatasets(int n) { m_presenter.setNumberOfDatasets(n); }

int ConvTemplateBrowser::getNumberOfDatasets() const { return m_presenter.getNumberOfDatasets(); }

void ConvTemplateBrowser::setDatasets(const QList<FunctionModelDataset> &datasets) {
  m_presenter.setDatasets(datasets);
}

QStringList ConvTemplateBrowser::getGlobalParameters() const { return m_presenter.getGlobalParameters(); }

QStringList ConvTemplateBrowser::getLocalParameters() const { return m_presenter.getLocalParameters(); }

void ConvTemplateBrowser::setGlobalParameters(const QStringList &globals) { m_presenter.setGlobalParameters(globals); }

void ConvTemplateBrowser::boolChanged(QtProperty *prop) {
  if (!m_emitBoolChange)
    return;
  if (prop == m_deltaFunctionOn) {
    m_presenter.setDeltaFunction(m_boolManager->value(prop));
  } else if (prop == m_tempCorrectionOn) {
    m_presenter.setTempCorrection(m_boolManager->value(prop));
  }
}

void ConvTemplateBrowser::setQValues(const std::vector<double> &qValues) { m_presenter.setQValues(qValues); }

void ConvTemplateBrowser::addDeltaFunction() {
  ScopedFalse _boolBlock(m_emitBoolChange);
  m_deltaFunctionOn->addSubProperty(m_deltaFunctionHeight);
  m_deltaFunctionOn->addSubProperty(m_deltaFunctionCenter);
  m_boolManager->setValue(m_deltaFunctionOn, true);
}

void ConvTemplateBrowser::removeDeltaFunction() {
  m_deltaFunctionOn->removeSubProperty(m_deltaFunctionHeight);
  m_deltaFunctionOn->removeSubProperty(m_deltaFunctionCenter);
  ScopedFalse _false(m_emitBoolChange);
  m_boolManager->setValue(m_deltaFunctionOn, false);
}

void ConvTemplateBrowser::addTempCorrection(double value) {
  ScopedFalse _boolBlock(m_emitBoolChange);

  m_tempCorrectionOn->addSubProperty(m_temperature);
  m_boolManager->setValue(m_tempCorrectionOn, true);
  m_parameterManager->setValue(m_temperature, value);
  m_parameterManager->setGlobal(m_temperature, true);
}

void ConvTemplateBrowser::updateTemperatureCorrectionAndDelta(bool tempCorrection, bool deltaFunction) {
  ScopedFalse _boolBlock(m_emitBoolChange);
  ScopedFalse _paramBlock(m_emitParameterValueChange);

  if (tempCorrection)
    addTempCorrection(100.0);
  else
    removeTempCorrection();

  if (deltaFunction)
    addDeltaFunction();
  else
    removeDeltaFunction();
}

void ConvTemplateBrowser::removeTempCorrection() {
  m_tempCorrectionOn->removeSubProperty(m_temperature);
  ScopedFalse _false(m_emitBoolChange);
  m_boolManager->setValue(m_tempCorrectionOn, false);
}

void ConvTemplateBrowser::enumChanged(QtProperty *prop) {
  if (!m_emitEnumChange)
    return;
  auto const index = m_enumManager->value(prop);
  auto propIt = std::find(m_subTypeProperties.begin(), m_subTypeProperties.end(), prop);
  if (propIt != m_subTypeProperties.end()) {
    auto const subTypeIndex = std::distance(m_subTypeProperties.begin(), propIt);
    m_presenter.setSubType(subTypeIndex, index);
  }
}

void ConvTemplateBrowser::globalChanged(QtProperty *, const QString &, bool) {}

void ConvTemplateBrowser::parameterChanged(QtProperty *prop) {
  auto isGlobal = m_parameterManager->isGlobal(prop);
  m_presenter.setGlobal(m_actualParameterNames[prop], isGlobal);
  if (m_emitParameterValueChange) {
    emit parameterValueChanged(m_actualParameterNames[prop], m_parameterManager->value(prop));
  }
}

void ConvTemplateBrowser::parameterButtonClicked(QtProperty *prop) {
  emit localParameterButtonClicked(m_actualParameterNames[prop]);
}

void ConvTemplateBrowser::updateMultiDatasetParameters(const IFunction &fun) {
  m_presenter.updateMultiDatasetParameters(fun);
}

void ConvTemplateBrowser::updateMultiDatasetParameters(const ITableWorkspace &paramTable) {
  m_presenter.updateMultiDatasetParameters(paramTable);
}

void ConvTemplateBrowser::updateParameters(const IFunction &fun) { m_presenter.updateParameters(fun); }

void ConvTemplateBrowser::setCurrentDataset(int i) { m_presenter.setCurrentDataset(i); }

void ConvTemplateBrowser::updateParameterNames(const QMap<int, QString> &parameterNames) {
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

void ConvTemplateBrowser::clear() {}

void ConvTemplateBrowser::popupMenu(const QPoint &) {}

void ConvTemplateBrowser::setParameterPropertyValue(QtProperty *prop, double value, double error) {
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
        props.append(prop);
        auto const id = paramIDs[i];
        m_parameterMap[prop] = id;
        m_parameterReverseMap[id] = prop;
      }
      parameters[index] = props;
    }
    if (isub == SubTypeIndex::Lorentzian) {
      auto subtypeProp = m_intManager->addProperty(subType->name());
      m_intManager->setMinimum(subtypeProp, 0);
      m_intManager->setMaximum(subtypeProp, 2);
      m_subTypeProperties.push_back(subtypeProp);

    } else {
      auto subTypeProp = m_enumManager->addProperty(subType->name());
      m_enumManager->setEnumNames(subTypeProp, m_templateSubTypes[isub]->getTypeNames());
      m_enumManager->setEnumNames(subTypeProp, m_templateSubTypes[isub]->getTypeNames());
      m_subTypeProperties.push_back(subTypeProp);
    }
  }
}

void ConvTemplateBrowser::setEnum(size_t subTypeIndex, int enumIndex) {
  ScopedFalse _false(m_emitEnumChange);
  m_enumManager->setValue(m_subTypeProperties[subTypeIndex], enumIndex);
}

void ConvTemplateBrowser::setInt(size_t subTypeIndex, int value) {
  ScopedFalse _false(m_emitIntChange);
  m_intManager->setValue(m_subTypeProperties[subTypeIndex], value);
}

void ConvTemplateBrowser::createDeltaFunctionProperties() {
  m_deltaFunctionOn = m_boolManager->addProperty("Delta Function");
  m_deltaFunctionHeight = m_parameterManager->addProperty("DeltaFunctionHeight");
  m_parameterManager->setDecimals(m_deltaFunctionHeight, 6);
  m_parameterManager->setMinimum(m_deltaFunctionHeight, 0.0);
  m_parameterManager->setDescription(m_deltaFunctionHeight, "Delta Function Height");
  m_parameterMap[m_deltaFunctionHeight] = ParamID::DELTA_HEIGHT;
  m_parameterReverseMap[ParamID::DELTA_HEIGHT] = m_deltaFunctionHeight;

  m_deltaFunctionCenter = m_parameterManager->addProperty("DeltaFunctionCenter");
  m_parameterManager->setDecimals(m_deltaFunctionCenter, 6);
  m_parameterManager->setDescription(m_deltaFunctionCenter, "Delta Function Height");
  m_parameterMap[m_deltaFunctionCenter] = ParamID::DELTA_CENTER;
  m_parameterReverseMap[ParamID::DELTA_CENTER] = m_deltaFunctionCenter;
}

void ConvTemplateBrowser::createTempCorrectionProperties() {
  m_tempCorrectionOn = m_boolManager->addProperty("Temp Correction");
  m_temperature = m_parameterManager->addProperty("Temperature");
  m_parameterManager->setDescription(m_temperature, "Temperature");
  m_parameterMap[m_temperature] = ParamID::TEMPERATURE;
  m_parameterReverseMap[ParamID::TEMPERATURE] = m_temperature;
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

void ConvTemplateBrowser::setParameterValueQuiet(ParamID id, double value, double error) {
  ScopedFalse _(m_emitParameterValueChange);
  auto prop = m_parameterReverseMap[id];
  m_parameterManager->setValue(prop, value);
  m_parameterManager->setError(prop, error);
}

void ConvTemplateBrowser::updateParameterEstimationData(DataForParameterEstimationCollection &&) {}

void ConvTemplateBrowser::estimateFunctionParameters() {}

void ConvTemplateBrowser::setBackgroundA0(double value) { m_presenter.setBackgroundA0(value); }

void ConvTemplateBrowser::setResolution(std::string const &name, TableDatasetIndex const &index) {
  m_presenter.setResolution(name, index);
}

void ConvTemplateBrowser::setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  m_presenter.setResolution(fitResolutions);
}

void ConvTemplateBrowser::intChanged(QtProperty *prop) {
  if (prop == m_subTypeProperties[SubTypeIndex::Lorentzian] && m_emitIntChange) {
    m_presenter.setSubType(SubTypeIndex::Lorentzian, m_intManager->value(prop));
  }
}
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
