// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/BasicFitOptionsBrowser.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

#include <QVBoxLayout>

namespace {

using namespace Mantid::API;

static int DEFAULT_MAX_ITERATIONS = 500;
static QString DEFAULT_MINIMIZER = "Levenberg-Marquardt";
static QStringList EVALUATION_TYPES = {"CentrePoint", "Histogram"};
static QStringList FITTING_MODES = {"Sequential", "Simultaneous"};

QStringList convertToQStringList(std::vector<std::string> const &vec) {
  QStringList list;
  std::transform(
      vec.cbegin(), vec.cend(), std::back_inserter(list),
      [](std::string const &str) { return QString::fromStdString(str); });
  return list;
}

QStringList minimizers() {
  return convertToQStringList(FuncMinimizerFactory::Instance().getKeys());
}

QStringList costFunctions() {
  return convertToQStringList(CostFunctionFactory::Instance().getKeys());
}

int defaultMinimizerIndex() {
  auto const index = minimizers().indexOf(DEFAULT_MINIMIZER);
  return index >= 0 ? index : 0;
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

BasicFitOptionsBrowser::BasicFitOptionsBrowser(QWidget *parent)
    : QWidget(parent), m_fittingMode(nullptr), m_maxIterations(nullptr),
      m_minimizer(nullptr), m_costFunction(nullptr), m_evaluationType(nullptr) {
  createBrowser();
  createProperties();
}

BasicFitOptionsBrowser::~BasicFitOptionsBrowser() {
  m_browser->unsetFactoryForManager(m_intManager);
  m_browser->unsetFactoryForManager(m_enumManager);
}

void BasicFitOptionsBrowser::createBrowser() {
  m_intManager = new QtIntPropertyManager(this);
  m_enumManager = new QtEnumPropertyManager(this);

  auto *spinBoxFactory = new QtSpinBoxFactory(this);
  auto *comboBoxFactory = new QtEnumEditorFactory(this);

  m_browser = new QtTreePropertyBrowser(nullptr, QStringList(), false);
  m_browser->setFactoryForManager(m_intManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_enumManager, comboBoxFactory);

  connect(m_enumManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(enumChanged(QtProperty *)));

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(m_browser);
  layout->setContentsMargins(0, 0, 0, 0);
}

void BasicFitOptionsBrowser::createProperties() {
  createFittingModeProperty();
  createMaxIterationsProperty();
  createMinimizerProperty();
  createCostFunctionProperty();
  createEvaluationTypeProperty();
}

void BasicFitOptionsBrowser::createFittingModeProperty() {
  m_fittingMode = m_enumManager->addProperty("Fitting Mode");
  m_enumManager->setEnumNames(m_fittingMode, FITTING_MODES);
  m_browser->addProperty(m_fittingMode);

  emitFittingModeChanged();
}

void BasicFitOptionsBrowser::createMaxIterationsProperty() {
  m_maxIterations = m_intManager->addProperty("Max Iterations");

  m_intManager->setValue(m_maxIterations, DEFAULT_MAX_ITERATIONS);
  m_intManager->setMinimum(m_maxIterations, 0);
  m_browser->addProperty(m_maxIterations);

  addProperty("MaxIterations", m_maxIterations,
              &BasicFitOptionsBrowser::getIntProperty,
              &BasicFitOptionsBrowser::setIntProperty);
}

void BasicFitOptionsBrowser::createMinimizerProperty() {
  m_minimizer = m_enumManager->addProperty("Minimizer");

  m_enumManager->setEnumNames(m_minimizer, minimizers());
  m_enumManager->setValue(m_minimizer, defaultMinimizerIndex());
  m_browser->addProperty(m_minimizer);

  addProperty("Minimizer", m_minimizer,
              &BasicFitOptionsBrowser::getStringEnumProperty,
              &BasicFitOptionsBrowser::setStringEnumProperty);
}

void BasicFitOptionsBrowser::createCostFunctionProperty() {
  m_costFunction = m_enumManager->addProperty("Cost Function");

  m_enumManager->setEnumNames(m_costFunction, costFunctions());
  m_browser->addProperty(m_costFunction);

  addProperty("CostFunction", m_costFunction,
              &BasicFitOptionsBrowser::getStringEnumProperty,
              &BasicFitOptionsBrowser::setStringEnumProperty);
}

void BasicFitOptionsBrowser::createEvaluationTypeProperty() {
  m_evaluationType = m_enumManager->addProperty("Evaluation Type");

  m_enumManager->setEnumNames(m_evaluationType, EVALUATION_TYPES);
  m_browser->addProperty(m_evaluationType);

  addProperty("EvaluationType", m_evaluationType,
              &BasicFitOptionsBrowser::getStringEnumProperty,
              &BasicFitOptionsBrowser::setStringEnumProperty);
}

void BasicFitOptionsBrowser::addProperty(
    const QString &name, QtProperty *prop,
    QString (BasicFitOptionsBrowser::*getter)(QtProperty *) const,
    void (BasicFitOptionsBrowser::*setter)(QtProperty *, const QString &)) {
  m_propertyNameMap[name] = prop;
  m_getters[prop] = getter;
  m_setters[prop] = setter;
}

void BasicFitOptionsBrowser::setProperty(const QString &name,
                                         const QString &value) {
  if (!m_propertyNameMap.contains(name)) {
    throw std::runtime_error("Property " + name.toStdString() +
                             " isn't supported by the browser.");
  }
  auto prop = m_propertyNameMap[name];
  auto f = m_setters[prop];
  (this->*f)(prop, value);
}

QString BasicFitOptionsBrowser::getProperty(const QString &name) const {
  if (!m_propertyNameMap.contains(name)) {
    throw std::runtime_error("Property " + name.toStdString() +
                             " isn't supported by the browser.");
  }
  auto prop = m_propertyNameMap[name];
  auto f = m_getters[prop];
  return (this->*f)(prop);
}

void BasicFitOptionsBrowser::enumChanged(QtProperty *prop) {
  if (prop == m_fittingMode) {
    emitFittingModeChanged();
  }
}

void BasicFitOptionsBrowser::emitFittingModeChanged() {
  switch (getFittingMode()) {
  case FittingMode::SEQUENTIAL:
    emit changedToSequentialFitting();
    return;
  case FittingMode::SIMULTANEOUS:
    emit changedToSimultaneousFitting();
    return;
  default:
    throw std::runtime_error(
        "Fitting mode must be SEQUENTIAL or SIMULTANEOUS.");
  }
}

void BasicFitOptionsBrowser::setIntProperty(QtProperty *prop,
                                            QString const &value) {
  m_intManager->setValue(prop, value.toInt());
}

QString BasicFitOptionsBrowser::getIntProperty(QtProperty *prop) const {
  return QString::number(m_intManager->value(prop));
}

void BasicFitOptionsBrowser::setStringEnumProperty(QtProperty *prop,
                                                   QString const &value) {
  auto const i = m_enumManager->enumNames(prop).indexOf(value);
  if (i >= 0)
    m_enumManager->setValue(prop, i);
}

QString BasicFitOptionsBrowser::getStringEnumProperty(QtProperty *prop) const {
  auto const i = m_enumManager->value(prop);
  if (i >= 0)
    return m_enumManager->enumNames(prop)[i];
  return "";
}

void BasicFitOptionsBrowser::setFittingMode(FittingMode fittingMode) {
  switch (fittingMode) {
  case FittingMode::SEQUENTIAL:
    m_enumManager->setValue(m_fittingMode, 0);
    return;
  case FittingMode::SIMULTANEOUS:
    m_enumManager->setValue(m_fittingMode, 1);
    return;
  default:
    throw std::invalid_argument(
        "Fitting mode must be SEQUENTIAL or SIMULTANEOUS.");
  }
}

FittingMode BasicFitOptionsBrowser::getFittingMode() const {
  auto const value = m_enumManager->value(m_fittingMode);
  return static_cast<FittingMode>(value);
}

} // namespace MantidWidgets
} // namespace MantidQt
