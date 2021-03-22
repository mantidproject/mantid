// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/BasicFitOptionsBrowser.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/PropertyWithValue.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

#include <QVBoxLayout>

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 * @param parent :: The parent widget.
 * @param fitType :: The type of the underlying fitting algorithm.
 */
BasicFitOptionsBrowser::BasicFitOptionsBrowser(QWidget *parent,
                                               FittingMode fitType)
    : QWidget(parent), m_fittingTypeProp(nullptr), m_minimizer(nullptr),
      m_decimals(6), m_fittingType(fitType) {
  // create m_browser
  createBrowser();
  createProperties();

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(m_browser);
  layout->setContentsMargins(0, 0, 0, 0);
}

BasicFitOptionsBrowser::~BasicFitOptionsBrowser() {
  m_browser->unsetFactoryForManager(m_intManager);
  m_browser->unsetFactoryForManager(m_enumManager);
}

/**
 * Create the Qt property browser and set up property managers.
 */
void BasicFitOptionsBrowser::createBrowser() {
  /* Create property managers: they create, own properties, get and set values
   */
  m_intManager = new QtIntPropertyManager(this);
  m_enumManager = new QtEnumPropertyManager(this);

  // create editor factories
  auto *spinBoxFactory = new QtSpinBoxFactory(this);
  auto *comboBoxFactory = new QtEnumEditorFactory(this);

  m_browser = new QtTreePropertyBrowser(nullptr, QStringList(), false);
  // assign factories to property managers
  m_browser->setFactoryForManager(m_intManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_enumManager, comboBoxFactory);

  connect(m_enumManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(enumChanged(QtProperty *)));
}

/**
 * @brief Initialize the fitting type property. Show in the browser
 * only if user can switch fit type.
 */
void BasicFitOptionsBrowser::initFittingTypeProp() {
  m_fittingTypeProp = m_enumManager->addProperty("Fitting Mode");
  QStringList types;
  types << "Sequential"
        << "Simultaneous";
  m_enumManager->setEnumNames(m_fittingTypeProp, types);
  m_browser->addProperty(m_fittingTypeProp);
}

/**
 * Create browser's QtProperties
 */
void BasicFitOptionsBrowser::createProperties() {
  initFittingTypeProp();
  createCommonProperties();
  switchFitType();
}

void BasicFitOptionsBrowser::createCommonProperties() {
  // Create MaxIterations property
  m_maxIterations = m_intManager->addProperty("Max Iterations");
  {
    m_intManager->setValue(m_maxIterations, 500);
    m_intManager->setMinimum(m_maxIterations, 0);
    m_browser->addProperty(m_maxIterations);

    addProperty("MaxIterations", m_maxIterations,
                &BasicFitOptionsBrowser::getIntProperty,
                &BasicFitOptionsBrowser::setIntProperty);
  }

  // Set up the minimizer property.
  m_minimizer = m_enumManager->addProperty("Minimizer");
  {
    // Get names of registered minimizers from the factory
    std::vector<std::string> minimizerOptions =
        Mantid::API::FuncMinimizerFactory::Instance().getKeys();
    QStringList minimizers;
    for (auto &minimizerOption : minimizerOptions) {
      minimizers << QString::fromStdString(minimizerOption);
    }

    m_enumManager->setEnumNames(m_minimizer, minimizers);
    int i =
        m_enumManager->enumNames(m_minimizer).indexOf("Levenberg-Marquardt");
    if (i >= 0) {
      m_enumManager->setValue(m_minimizer, i);
    }
    m_browser->addProperty(m_minimizer);
    addProperty("Minimizer", m_minimizer,
                &BasicFitOptionsBrowser::getStringEnumProperty,
                &BasicFitOptionsBrowser::setStringEnumProperty);
  }

  // Create cost function property
  m_costFunction = m_enumManager->addProperty("Cost Function");
  {
    // Get names of registered cost functions from the factory
    std::vector<std::string> costOptions =
        Mantid::API::CostFunctionFactory::Instance().getKeys();
    QStringList costFunctions;
    // Store them in the m_minimizer enum property
    for (auto &costOption : costOptions) {
      costFunctions << QString::fromStdString(costOption);
    }
    m_enumManager->setEnumNames(m_costFunction, costFunctions);
    m_browser->addProperty(m_costFunction);
    addProperty("CostFunction", m_costFunction,
                &BasicFitOptionsBrowser::getStringEnumProperty,
                &BasicFitOptionsBrowser::setStringEnumProperty);
  }

  // Create EvaluationType property
  m_evaluationType = m_enumManager->addProperty("Evaluation Type");
  {
    QStringList evaluationTypes;
    evaluationTypes << "CentrePoint"
                    << "Histogram";
    m_enumManager->setEnumNames(m_evaluationType, evaluationTypes);
    m_browser->addProperty(m_evaluationType);
    addProperty("EvaluationType", m_evaluationType,
                &BasicFitOptionsBrowser::getStringEnumProperty,
                &BasicFitOptionsBrowser::setStringEnumProperty);
  }
}

void BasicFitOptionsBrowser::addProperty(
    const QString &name, QtProperty *prop,
    QString (BasicFitOptionsBrowser::*getter)(QtProperty *) const,
    void (BasicFitOptionsBrowser::*setter)(QtProperty *, const QString &)) {
  m_propertyNameMap[name] = prop;
  m_getters[prop] = getter;
  m_setters[prop] = setter;
}

/**
 * Set a new value to a Fit's property.
 * @param name :: The name of a Fit's property.
 * @param value :: The new value as a string.
 */
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

/**
 * Update the browser when an enum property changes.
 * @param prop :: Property that changed its value.
 */
void BasicFitOptionsBrowser::enumChanged(QtProperty *prop) {
  if (prop == m_fittingTypeProp) {
    switchFitType();
  }
}

/**
 * Switch the current fit type according to the value in the FitType property.
 */
void BasicFitOptionsBrowser::switchFitType() {
  const auto fittingMode = getCurrentFittingType();
  if (fittingMode == FittingMode::SIMULTANEOUS) {
    emit changedToSimultaneousFitting();
  } else {
    emit changedToSequentialFitting();
  }
}

/**
 * Get the value of an integer algorithm property.
 * @param prop :: The corresponding QtProperty.
 */
QString BasicFitOptionsBrowser::getIntProperty(QtProperty *prop) const {
  return QString::number(m_intManager->value(prop));
}

/**
 * Set a new value of an integer algorithm property.
 * @param prop :: The corresponding QtProperty.
 * @param value :: The new value.
 */
void BasicFitOptionsBrowser::setIntProperty(QtProperty *prop,
                                            const QString &value) {
  m_intManager->setValue(prop, value.toInt());
}

/**
 * Get the value of a string algorithm property with predefined set of values.
 * @param prop :: The corresponding QtProperty.
 */
QString BasicFitOptionsBrowser::getStringEnumProperty(QtProperty *prop) const {
  int i = m_enumManager->value(prop);
  if (i < 0)
    return "";
  return m_enumManager->enumNames(prop)[i];
}

/**
 * Set a new value of a string algorithm property with predefined set of values.
 * @param prop :: The corresponding QtProperty.
 * @param value :: The new value.
 */
void BasicFitOptionsBrowser::setStringEnumProperty(QtProperty *prop,
                                                   const QString &value) {
  int i = m_enumManager->enumNames(prop).indexOf(value);
  if (i >= 0)
    m_enumManager->setValue(prop, i);
}

/**
 * Get the current fitting type, ie which algorithm to use:
 *    Simultaneous for Fit and Sequential for PlotPeakByLogValue.
 */
FittingMode BasicFitOptionsBrowser::getCurrentFittingType() const {
  auto value = m_enumManager->value(m_fittingTypeProp);
  return static_cast<FittingMode>(value);
}

/**
 * Set the current fitting type, ie which algorithm to use:
 *    Simultaneous for Fit and Sequential for PlotPeakByLogValue.
 */
void BasicFitOptionsBrowser::setCurrentFittingType(FittingMode fitType) {
  if (fitType == FittingMode::SIMULTANEOUS) {
    m_enumManager->setValue(m_fittingTypeProp, 1);
  } else {
    m_enumManager->setValue(m_fittingTypeProp, 0);
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
