// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptOptionsBrowser.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

#include <QVBoxLayout>

namespace {

using namespace Mantid::API;

int constexpr DEFAULT_MAX_ITERATIONS = 500;
QString const DEFAULT_MINIMIZER = "Levenberg-Marquardt";
QString const DEFAULT_OUTPUT_BASE_NAME = "Output_Fit";
bool const DEFAULT_PLOT_OUTPUT = true;
QStringList const EVALUATION_TYPES = {"CentrePoint", "Histogram"};
QStringList const FITTING_MODES = {"Sequential", "Simultaneous"};

QStringList convertToQStringList(std::vector<std::string> const &vec) {
  QStringList list;
  list.reserve(static_cast<int>(vec.size()));
  std::transform(vec.cbegin(), vec.cend(), std::back_inserter(list),
                 [](std::string const &str) { return QString::fromStdString(str); });
  return list;
}

QStringList minimizers() { return convertToQStringList(FuncMinimizerFactory::Instance().getKeys()); }

QStringList costFunctions() { return convertToQStringList(CostFunctionFactory::Instance().getKeys()); }

int defaultMinimizerIndex() {
  auto const index = minimizers().indexOf(DEFAULT_MINIMIZER);
  return index >= 0 ? index : 0;
}

} // namespace

namespace MantidQt::MantidWidgets {

FitScriptOptionsBrowser::FitScriptOptionsBrowser(QWidget *parent)
    : QWidget(parent), m_fittingMode(nullptr), m_maxIterations(nullptr), m_minimizer(nullptr), m_costFunction(nullptr),
      m_evaluationType(nullptr) {
  createBrowser();
  createProperties();
}

FitScriptOptionsBrowser::~FitScriptOptionsBrowser() {
  m_browser->unsetFactoryForManager(m_stringManager);
  m_browser->unsetFactoryForManager(m_intManager);
  m_browser->unsetFactoryForManager(m_enumManager);
  m_browser->unsetFactoryForManager(m_boolManager);
}

void FitScriptOptionsBrowser::createBrowser() {
  m_stringManager = new QtStringPropertyManager(this);
  m_intManager = new QtIntPropertyManager(this);
  m_enumManager = new QtEnumPropertyManager(this);
  m_boolManager = new QtBoolPropertyManager(this);

  auto *lineEditFactory = new QtLineEditFactory(this);
  auto *spinBoxFactory = new QtSpinBoxFactory(this);
  auto *comboBoxFactory = new QtEnumEditorFactory(this);
  auto *checkBoxFactory = new QtCheckBoxFactory(this);

  m_browser = new QtTreePropertyBrowser(nullptr, QStringList(), false);
  m_browser->setFactoryForManager(m_stringManager, lineEditFactory);
  m_browser->setFactoryForManager(m_intManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_enumManager, comboBoxFactory);
  m_browser->setFactoryForManager(m_boolManager, checkBoxFactory);
  m_browser->setMinimumHeight(110);

  connect(m_stringManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(stringChanged(QtProperty *)));
  connect(m_enumManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(enumChanged(QtProperty *)));

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(m_browser);
  layout->setContentsMargins(0, 0, 0, 0);
}

void FitScriptOptionsBrowser::createProperties() {
  createFittingModeProperty();
  createMaxIterationsProperty();
  createMinimizerProperty();
  createCostFunctionProperty();
  createEvaluationTypeProperty();
  createOutputBaseNameProperty();
  createPlotOutputProperty();
}

void FitScriptOptionsBrowser::createFittingModeProperty() {
  m_fittingMode = m_enumManager->addProperty("Fitting Mode");
  m_enumManager->setEnumNames(m_fittingMode, FITTING_MODES);
  m_browser->addProperty(m_fittingMode);

  emit fittingModeChanged(getFittingMode());
}

void FitScriptOptionsBrowser::createMaxIterationsProperty() {
  m_maxIterations = m_intManager->addProperty("Max Iterations");

  m_intManager->setValue(m_maxIterations, DEFAULT_MAX_ITERATIONS);
  m_intManager->setMinimum(m_maxIterations, 0);
  m_browser->addProperty(m_maxIterations);

  addProperty("Max Iterations", m_maxIterations, &FitScriptOptionsBrowser::getIntProperty,
              &FitScriptOptionsBrowser::setIntProperty);
}

void FitScriptOptionsBrowser::createMinimizerProperty() {
  m_minimizer = m_enumManager->addProperty("Minimizer");

  m_enumManager->setEnumNames(m_minimizer, minimizers());
  m_enumManager->setValue(m_minimizer, defaultMinimizerIndex());
  m_browser->addProperty(m_minimizer);

  addProperty("Minimizer", m_minimizer, &FitScriptOptionsBrowser::getStringEnumProperty,
              &FitScriptOptionsBrowser::setStringEnumProperty);
}

void FitScriptOptionsBrowser::createCostFunctionProperty() {
  m_costFunction = m_enumManager->addProperty("Cost Function");

  m_enumManager->setEnumNames(m_costFunction, costFunctions());
  m_browser->addProperty(m_costFunction);

  addProperty("Cost Function", m_costFunction, &FitScriptOptionsBrowser::getStringEnumProperty,
              &FitScriptOptionsBrowser::setStringEnumProperty);
}

void FitScriptOptionsBrowser::createEvaluationTypeProperty() {
  m_evaluationType = m_enumManager->addProperty("Evaluation Type");

  m_enumManager->setEnumNames(m_evaluationType, EVALUATION_TYPES);
  m_browser->addProperty(m_evaluationType);

  addProperty("Evaluation Type", m_evaluationType, &FitScriptOptionsBrowser::getStringEnumProperty,
              &FitScriptOptionsBrowser::setStringEnumProperty);
}

void FitScriptOptionsBrowser::createOutputBaseNameProperty() {
  m_outputBaseName = m_stringManager->addProperty("Output Base Name");

  m_stringManager->setValue(m_outputBaseName, DEFAULT_OUTPUT_BASE_NAME);
  m_browser->addProperty(m_outputBaseName);

  addProperty("Output Base Name", m_outputBaseName, &FitScriptOptionsBrowser::getStringProperty,
              &FitScriptOptionsBrowser::setStringProperty);
}

void FitScriptOptionsBrowser::createPlotOutputProperty() {
  m_plotOutput = m_boolManager->addProperty("Plot Output");

  m_boolManager->setValue(m_plotOutput, DEFAULT_PLOT_OUTPUT);
  m_browser->addProperty(m_plotOutput);

  m_propertyNameMap["Plot Output"] = m_plotOutput;
}

void FitScriptOptionsBrowser::addProperty(std::string const &name, QtProperty *prop, PropertyGetter getter,
                                          PropertySetter setter) {
  m_propertyNameMap[name] = prop;
  m_getters[prop] = getter;
  m_setters[prop] = setter;
}

void FitScriptOptionsBrowser::setProperty(std::string const &name, std::string const &value) {
  auto const prop = getQtPropertyFor(name);
  auto const f = m_setters[prop];
  (this->*f)(prop, value);
}

std::string FitScriptOptionsBrowser::getProperty(std::string const &name) const {
  auto const prop = getQtPropertyFor(name);
  auto const f = m_getters[prop];
  return (this->*f)(prop);
}

bool FitScriptOptionsBrowser::getBoolProperty(std::string const &name) const {
  auto const *prop = getQtPropertyFor(name);
  return m_boolManager->value(prop);
}

QtProperty *FitScriptOptionsBrowser::getQtPropertyFor(std::string const &name) const {
  if (!m_propertyNameMap.contains(name)) {
    throw std::runtime_error("Property " + name + " isn't supported by the browser.");
  }
  return m_propertyNameMap[name];
}

void FitScriptOptionsBrowser::stringChanged(QtProperty *prop) {
  if (prop == m_outputBaseName) {
    emit outputBaseNameChanged(prop->valueText().toStdString());
  }
}

void FitScriptOptionsBrowser::enumChanged(QtProperty *prop) {
  if (prop == m_fittingMode) {
    emit fittingModeChanged(getFittingMode());
  }
}

void FitScriptOptionsBrowser::setStringProperty(QtProperty *prop, std::string const &value) {
  m_stringManager->setValue(prop, QString::fromStdString(value));
}

std::string FitScriptOptionsBrowser::getStringProperty(QtProperty *prop) const {
  return m_stringManager->value(prop).toStdString();
}

void FitScriptOptionsBrowser::setIntProperty(QtProperty *prop, std::string const &value) {
  m_intManager->setValue(prop, QString::fromStdString(value).toInt());
}

std::string FitScriptOptionsBrowser::getIntProperty(QtProperty *prop) const {
  return QString::number(m_intManager->value(prop)).toStdString();
}

void FitScriptOptionsBrowser::setStringEnumProperty(QtProperty *prop, std::string const &value) {
  auto const i = m_enumManager->enumNames(prop).indexOf(QString::fromStdString(value));
  if (i < 0)
    throw std::invalid_argument("An invalid property value was provided.");

  m_enumManager->setValue(prop, i);
}

std::string FitScriptOptionsBrowser::getStringEnumProperty(QtProperty *prop) const {
  auto const i = m_enumManager->value(prop);
  if (i < 0)
    throw std::invalid_argument("An invalid property was provided.");

  return m_enumManager->enumNames(prop)[i].toStdString();
}

void FitScriptOptionsBrowser::setFittingMode(FittingMode fittingMode) {
  switch (fittingMode) {
  case FittingMode::SEQUENTIAL:
    m_enumManager->setValue(m_fittingMode, 0);
    return;
  case FittingMode::SIMULTANEOUS:
    m_enumManager->setValue(m_fittingMode, 1);
    return;
  default:
    throw std::invalid_argument("Fitting mode must be SEQUENTIAL or SIMULTANEOUS.");
  }
}

FittingMode FitScriptOptionsBrowser::getFittingMode() const {
  return static_cast<FittingMode>(m_enumManager->value(m_fittingMode));
}

} // namespace MantidQt::MantidWidgets
