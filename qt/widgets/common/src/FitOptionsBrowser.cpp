// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitOptionsBrowser.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/PropertyWithValue.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"
// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/ButtonEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/CompositeEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>
#include <limits>

namespace MantidQt::MantidWidgets {

/**
 * Constructor
 * @param parent :: The parent widget.
 * @param fitType :: The type of the underlying fitting algorithm.
 */
FitOptionsBrowser::FitOptionsBrowser(QWidget *parent, FittingMode fitType)
    : QWidget(parent), m_fittingTypeProp(nullptr), m_minimizer(nullptr), m_decimals(6), m_fittingType(fitType) {
  // create m_browser
  createBrowser();
  createProperties();

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(m_browser);
  layout->setContentsMargins(0, 0, 0, 0);
}

FitOptionsBrowser::~FitOptionsBrowser() {
  m_browser->unsetFactoryForManager(m_stringManager);
  m_browser->unsetFactoryForManager(m_doubleManager);
  m_browser->unsetFactoryForManager(m_intManager);
  m_browser->unsetFactoryForManager(m_boolManager);
  m_browser->unsetFactoryForManager(m_enumManager);
}

/**
 * Create the Qt property browser and set up property managers.
 */
void FitOptionsBrowser::createBrowser() {
  /* Create property managers: they create, own properties, get and set values
   */
  m_stringManager = new QtStringPropertyManager(this);
  m_doubleManager = new QtDoublePropertyManager(this);
  m_intManager = new QtIntPropertyManager(this);
  m_boolManager = new QtBoolPropertyManager(this);
  m_enumManager = new QtEnumPropertyManager(this);
  m_groupManager = new QtGroupPropertyManager(this);

  // create editor factories
  auto *spinBoxFactory = new QtSpinBoxFactory(this);
  auto *doubleEditorFactory = new DoubleEditorFactory(this);
  auto *lineEditFactory = new QtLineEditFactory(this);
  auto *checkBoxFactory = new QtCheckBoxFactory(this);
  auto *comboBoxFactory = new QtEnumEditorFactory(this);

  m_browser = new QtTreePropertyBrowser(nullptr, QStringList(), false);
  // assign factories to property managers
  m_browser->setFactoryForManager(m_stringManager, lineEditFactory);
  m_browser->setFactoryForManager(m_doubleManager, doubleEditorFactory);
  m_browser->setFactoryForManager(m_intManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_boolManager, checkBoxFactory);
  m_browser->setFactoryForManager(m_enumManager, comboBoxFactory);

  // m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
  // connect(m_browser, SIGNAL(customContextMenuRequested(const QPoint &)),
  // this, SLOT(popupMenu(const QPoint &)));

  connect(m_enumManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(enumChanged(QtProperty *)));
  connect(m_doubleManager, SIGNAL(propertyChanged(QtProperty *)), this, SLOT(doubleChanged(QtProperty *)));
  // Fill in getter and setter maps
}

/**
 * @brief Initialize the fitting type property. Show in the browser
 * only if user can switch fit type.
 */
void FitOptionsBrowser::initFittingTypeProp() {
  m_fittingTypeProp = m_enumManager->addProperty("Fitting");
  QStringList types;
  types << "Sequential"
        << "Simultaneous";
  m_enumManager->setEnumNames(m_fittingTypeProp, types);
  if (m_fittingType == FittingMode::SEQUENTIAL_AND_SIMULTANEOUS) {
    m_browser->addProperty(m_fittingTypeProp);
  } else if (m_fittingType == FittingMode::SIMULTANEOUS || m_fittingType == FittingMode::SEQUENTIAL) {
    this->lockCurrentFittingType(m_fittingType);
  }
}

/**
 * Create browser's QtProperties
 */
void FitOptionsBrowser::createProperties() {
  initFittingTypeProp();
  createCommonProperties();
  if (m_fittingType == FittingMode::SIMULTANEOUS || m_fittingType == FittingMode::SEQUENTIAL_AND_SIMULTANEOUS) {
    createSimultaneousFitProperties();
  }
  if (m_fittingType == FittingMode::SEQUENTIAL || m_fittingType == FittingMode::SEQUENTIAL_AND_SIMULTANEOUS) {
    createSequentialFitProperties();
  }
  switchFitType();
}

void FitOptionsBrowser::createCommonProperties() {
  // Create MaxIterations property
  m_maxIterations = m_intManager->addProperty("Max Iterations");
  {
    m_intManager->setValue(m_maxIterations, 500);
    m_intManager->setMinimum(m_maxIterations, 0);
    m_browser->addProperty(m_maxIterations);

    addProperty("MaxIterations", m_maxIterations, &FitOptionsBrowser::getIntProperty,
                &FitOptionsBrowser::setIntProperty);
  }

  // Set up the minimizer property.
  //
  // Create the enclosing group property. This group contains
  // the minimizer name plus any properties of its own
  m_minimizerGroup = m_groupManager->addProperty("Minimizer");
  {
    // Add the name property to the group
    m_minimizer = m_enumManager->addProperty("Name");
    m_minimizerGroup->addSubProperty(m_minimizer);

    // Get names of registered minimizers from the factory
    std::vector<std::string> minimizerOptions = Mantid::API::FuncMinimizerFactory::Instance().getKeys();
    QStringList minimizers;

    // Store them in the m_minimizer enum property
    for (auto &minimizerOption : minimizerOptions) {
      minimizers << QString::fromStdString(minimizerOption);
    }
    m_enumManager->setEnumNames(m_minimizer, minimizers);
    int i = m_enumManager->enumNames(m_minimizer).indexOf("Levenberg-Marquardt");
    if (i >= 0) {
      m_enumManager->setValue(m_minimizer, i);
    }
    m_browser->addProperty(m_minimizerGroup);
    addProperty("Minimizer", m_minimizer, &FitOptionsBrowser::getMinimizer, &FitOptionsBrowser::setMinimizer);
  }

  // Create cost function property
  m_costFunction = m_enumManager->addProperty("Cost Function");
  {
    // Get names of registered cost functions from the factory
    std::vector<std::string> costOptions = Mantid::API::CostFunctionFactory::Instance().getKeys();
    QStringList costFunctions;
    // Store them in the m_minimizer enum property
    for (auto &costOption : costOptions) {
      costFunctions << QString::fromStdString(costOption);
    }
    m_enumManager->setEnumNames(m_costFunction, costFunctions);
    m_browser->addProperty(m_costFunction);
    addProperty("CostFunction", m_costFunction, &FitOptionsBrowser::getStringEnumProperty,
                &FitOptionsBrowser::setStringEnumProperty);
  }

  // Create EvaluationType property
  m_evaluationType = m_enumManager->addProperty("Evaluate Function As");
  {
    QStringList evaluationTypes;
    evaluationTypes << "CentrePoint"
                    << "Histogram";
    m_enumManager->setEnumNames(m_evaluationType, evaluationTypes);
    m_browser->addProperty(m_evaluationType);
    addProperty("EvaluationType", m_evaluationType, &FitOptionsBrowser::getStringEnumProperty,
                &FitOptionsBrowser::setStringEnumProperty);
  }
  // Create PeakRadius property
  m_peakRadius = m_intManager->addProperty("Peak Radius");
  {
    m_intManager->setValue(m_peakRadius, 0);
    m_intManager->setMinimum(m_peakRadius, 0);
    m_browser->addProperty(m_peakRadius);
    addProperty("PeakRadius", m_peakRadius, &FitOptionsBrowser::getIntProperty, &FitOptionsBrowser::setIntProperty);
  }

  m_ignoreInvalidData = m_boolManager->addProperty("Ignore Invalid Data");
  {
    m_browser->addProperty(m_ignoreInvalidData);
    addProperty("IgnoreInvalidData", m_ignoreInvalidData, &FitOptionsBrowser::getBoolProperty,
                &FitOptionsBrowser::setBoolProperty);
  }
}

void FitOptionsBrowser::createSimultaneousFitProperties() {
  // Create Output property
  m_output = m_stringManager->addProperty("Output");
  {
    m_browser->addProperty(m_output);
    addProperty("Output", m_output, &FitOptionsBrowser::getStringProperty, &FitOptionsBrowser::setStringProperty);
    m_simultaneousProperties << m_output;
  }
}

void FitOptionsBrowser::createSequentialFitProperties() {
  // Create FitType property, a property of algorithm PlotPeakByLogValue
  m_plotPeakByLogValueFitType = m_enumManager->addProperty("Fit Type");
  {
    QStringList types;
    types << "Sequential"
          << "Individual";
    m_enumManager->setEnumNames(m_plotPeakByLogValueFitType, types);
    m_enumManager->setValue(m_plotPeakByLogValueFitType, 0);
    addProperty("FitType", m_plotPeakByLogValueFitType, &FitOptionsBrowser::getStringEnumProperty,
                &FitOptionsBrowser::setStringEnumProperty);
    m_sequentialProperties << m_plotPeakByLogValueFitType;
  }

  // Create OutputWorkspace property
  m_outputWorkspace = m_stringManager->addProperty("OutputWorkspace");
  {
    addProperty("OutputWorkspace", m_outputWorkspace, &FitOptionsBrowser::getStringProperty,
                &FitOptionsBrowser::setStringProperty);
    m_sequentialProperties << m_outputWorkspace;
  }

  // Create CreateOutput property
  auto prop = m_boolManager->addProperty("Create Output");
  {
    addProperty("CreateOutput", prop, &FitOptionsBrowser::getBoolProperty, &FitOptionsBrowser::setBoolProperty);
    m_sequentialProperties << prop;
  }

  // Create OutputCompositeMembers property
  prop = m_boolManager->addProperty("Output Composite Members");
  {
    addProperty("OutputCompositeMembers", prop, &FitOptionsBrowser::getBoolProperty,
                &FitOptionsBrowser::setBoolProperty);
    m_sequentialProperties << prop;
  }

  // Create ConvolveMembers property
  prop = m_boolManager->addProperty("Convolve Members");
  {
    addProperty("ConvolveMembers", prop, &FitOptionsBrowser::getBoolProperty, &FitOptionsBrowser::setBoolProperty);
    m_sequentialProperties << prop;
  }

  // Create PassWSIndexToFunction property
  prop = m_boolManager->addProperty("Pass WS Index To Function");
  {
    addProperty("PassWSIndexToFunction", prop, &FitOptionsBrowser::getBoolProperty,
                &FitOptionsBrowser::setBoolProperty);
    m_sequentialProperties << prop;
  }

  // Create LogValue property
  m_logValue = m_enumManager->addProperty("Log Value");
  {
    // m_enumManager->setValue(m_logValue,0);
    addProperty("LogValue", m_logValue, &FitOptionsBrowser::getStringEnumProperty,
                &FitOptionsBrowser::setStringEnumProperty);
    m_sequentialProperties << m_logValue;
  }

  // Create LogValue property
  m_plotParameter = m_enumManager->addProperty("Plot parameter");
  {
    addProperty("PlotParameter", m_plotParameter, &FitOptionsBrowser::getStringEnumProperty,
                &FitOptionsBrowser::setStringEnumProperty);
    m_sequentialProperties << m_plotParameter;
  }
}

void FitOptionsBrowser::addProperty(const QString &name, QtProperty *prop,
                                    QString (FitOptionsBrowser::*getter)(QtProperty *) const,
                                    void (FitOptionsBrowser::*setter)(QtProperty *, const QString &)) {
  m_propertyNameMap[name] = prop;
  m_getters[prop] = getter;
  m_setters[prop] = setter;
}

/**
 * Remove a property previously added with addProperty
 * (If property doesn't exist, does nothing).
 * @param name :: [input] Name of property to remove
 */
void FitOptionsBrowser::removeProperty(const QString &name) {
  if (m_propertyNameMap.contains(name)) {
    const auto prop = m_propertyNameMap[name];
    m_getters.remove(prop);
    m_setters.remove(prop);
    m_propertyNameMap.remove(name);
  }
}

/*                *********************
 *                **  Private Slots  **
 *                *********************/

/**
 * Update the browser when an enum property changes.
 * @param prop :: Property that changed its value.
 */
void FitOptionsBrowser::enumChanged(QtProperty *prop) {
  if (prop == m_minimizer) {
    updateMinimizer();
  } else if (prop == m_fittingTypeProp) {
    switchFitType();
  }
}

/**
 * @brief pass the signal emitted by m_doubleManager
 */
void FitOptionsBrowser::doubleChanged(QtProperty *property) { emit doublePropertyChanged(property->propertyName()); }

/**
 * Update the browser when minimizer changes.
 */
void FitOptionsBrowser::updateMinimizer() {
  int i = m_enumManager->value(m_minimizer);
  QString minimizerName = m_enumManager->enumNames(m_minimizer)[i];
  m_minimizerGroup->setPropertyName("Minimizer " + minimizerName);

  // Remove properties of the old minimizer
  auto subProperties = m_minimizerGroup->subProperties();
  foreach (QtProperty *prop, subProperties) {
    if (prop != m_minimizer) {
      m_minimizerGroup->removeSubProperty(prop);
      removeProperty(prop->propertyName());
    }
  }

  // Check if the new minimizer has its own properties
  auto minimizer = Mantid::API::FuncMinimizerFactory::Instance().createMinimizer(minimizerName.toStdString());

  // Create and add properties to the minimizer group
  auto minimizerProperties = minimizer->getProperties();
  for (auto &minimizerProperty : minimizerProperties) {
    auto prop = createPropertyProperty(minimizerProperty);
    if (prop) {
      m_minimizerGroup->addSubProperty(prop);
    }
  }
}

/**
 * Switch the current fit type according to the value in the FitType property.
 */
void FitOptionsBrowser::switchFitType() {
  const auto fittingMode = getCurrentFittingType();
  if (fittingMode == FittingMode::SIMULTANEOUS) {
    displayNormalFitProperties();
  } else {
    displaySequentialFitProperties();
  }
}

/**
 * Show normal Fit properties and hide the others.
 */
void FitOptionsBrowser::displayNormalFitProperties() {
  foreach (QtProperty *prop, m_simultaneousProperties) {
    m_browser->addProperty(prop);
  }
  foreach (QtProperty *prop, m_sequentialProperties) {
    m_browser->removeProperty(prop);
  }
  emit changedToSimultaneousFitting();
}

/**
 * Create a QtProperty for an Algorithm Property
 * and attach it to the correct manager.
 * @param property :: An algorithm property.
 */
QtProperty *FitOptionsBrowser::createPropertyProperty(Mantid::Kernel::Property *property) {
  if (!property) {
    throw std::runtime_error("Unable to create a QtProperty.");
  }
  QString propName = QString::fromStdString(property->name());
  QtProperty *prop = nullptr;
  if (auto prp = dynamic_cast<Mantid::Kernel::PropertyWithValue<bool> *>(property)) {
    prop = m_boolManager->addProperty(propName);
    bool val = *prp;
    m_boolManager->setValue(prop, val);
  } else if (auto prp = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(property)) {
    prop = this->addDoubleProperty(propName);
    double val = *prp;
    m_doubleManager->setValue(prop, val);
  } else if (auto prp = dynamic_cast<Mantid::Kernel::PropertyWithValue<int> *>(property)) {
    prop = m_intManager->addProperty(propName);
    int val = *prp;
    m_intManager->setValue(prop, val);
  } else if (auto prp = dynamic_cast<Mantid::Kernel::PropertyWithValue<size_t> *>(property)) {
    prop = m_intManager->addProperty(propName);
    size_t val = *prp;
    m_intManager->setValue(prop, static_cast<int>(val));
  } else if (auto prp = dynamic_cast<Mantid::Kernel::PropertyWithValue<std::string> *>(property)) {
    prop = m_stringManager->addProperty(propName);
    QString val = QString::fromStdString(prp->value());
    m_stringManager->setValue(prop, val);
  } else if (dynamic_cast<Mantid::API::IWorkspaceProperty *>(property)) {
    prop = m_stringManager->addProperty(propName);
    m_stringManager->setValue(prop, QString::fromStdString(property->value()));
  } else {
    QMessageBox::warning(this, "Mantid - Error",
                         "Type of minimizer's property " + propName + " is not yet supported by the browser.");
    return nullptr;
  }

  // Something bad happened in QtPropertyBrowser.
  if (!prop) {
    throw std::runtime_error("Failed to create a QtProperty.");
  }

  // set the tooltip from property doc string
  QString toolTip = QString::fromStdString(property->documentation());
  if (!toolTip.isEmpty()) {
    prop->setToolTip(toolTip);
  }

  return prop;
}

/**
 * Copy values of the properties to an algorithm.
 * @param fit :: An instance of the Fit algorithm.
 */
void FitOptionsBrowser::copyPropertiesToAlgorithm(Mantid::API::IAlgorithm &fit) const {
  for (auto p = m_propertyNameMap.constBegin(); p != m_propertyNameMap.constEnd(); ++p) {
    auto propertyName = p.key().toStdString();
    if (fit.existsProperty(propertyName)) {
      auto prop = p.value();
      auto f = m_getters[prop];
      fit.setPropertyValue(propertyName, (this->*f)(prop).toStdString());
    }
  }
}

/**
 * Get a string representation of a Fit's property value.
 * @param name :: The name of a Fit's property.
 */
QString FitOptionsBrowser::getProperty(const QString &name) const {
  if (!m_propertyNameMap.contains(name)) {
    throw std::runtime_error("Property " + name.toStdString() + " isn't supported by the browser.");
  }
  auto prop = m_propertyNameMap[name];
  auto f = m_getters[prop];
  return (this->*f)(prop);
}

/**
 * Set a new value to a Fit's property.
 * @param name :: The name of a Fit's property.
 * @param value :: The new value as a string.
 */
void FitOptionsBrowser::setProperty(const QString &name, const QString &value) {
  if (!m_propertyNameMap.contains(name)) {
    throw std::runtime_error("Property " + name.toStdString() + " isn't supported by the browser.");
  }
  auto prop = m_propertyNameMap[name];
  auto f = m_setters[prop];
  (this->*f)(prop, value);
}

/**
 * Get the value of the Minimizer property.
 */
QString FitOptionsBrowser::getMinimizer(QtProperty * /*unused*/) const {
  int i = m_enumManager->value(m_minimizer);
  QString minimStr = m_enumManager->enumNames(m_minimizer)[i];

  auto subProperties = m_minimizerGroup->subProperties();
  if (subProperties.size() > 1) {
    foreach (QtProperty *prop, subProperties) {
      if (prop == m_minimizer)
        continue;
      if (prop->propertyManager() == m_stringManager) {
        QString value = m_stringManager->value(prop);
        if (!value.isEmpty()) {
          minimStr += "," + prop->propertyName() + "=" + value;
        }
      } else {
        minimStr += "," + prop->propertyName() + "=";
        if (prop->propertyManager() == m_intManager) {
          minimStr += QString::number(m_intManager->value(prop));
        } else if (prop->propertyManager() == m_doubleManager) {
          minimStr += QString::number(m_doubleManager->value(prop));
        } else if (prop->propertyManager() == m_boolManager) {
          minimStr += QString::number(m_boolManager->value(prop));
        } else {
          throw std::runtime_error("The fit browser doesn't support the type "
                                   "of minimizer's property " +
                                   prop->propertyName().toStdString());
        }
      }
    } // foreach
  }
  return minimStr;
}

/**
 * Set new value to the Minimizer property.
 * @param value :: The new value.
 */
void FitOptionsBrowser::setMinimizer(QtProperty * /*unused*/, const QString &value) {
  QStringList terms = value.split(',');
  int i = m_enumManager->enumNames(m_minimizer).indexOf(terms[0]);
  m_enumManager->setValue(m_minimizer, i);
}

// ------------------------- Generic setters and getters
// ------------------------------//

/**
 * Get the value of an integer algorithm property.
 * @param prop :: The corresponding QtProperty.
 */
QString FitOptionsBrowser::getIntProperty(QtProperty *prop) const { return QString::number(m_intManager->value(prop)); }

/**
 * Set a new value of an integer algorithm property.
 * @param prop :: The corresponding QtProperty.
 * @param value :: The new value.
 */
void FitOptionsBrowser::setIntProperty(QtProperty *prop, const QString &value) {
  m_intManager->setValue(prop, value.toInt());
}

/**
 * Get the value of a double algorithm property.
 * @param prop :: The corresponding QtProperty.
 * @return the stored value
 */
QString FitOptionsBrowser::getDoubleProperty(QtProperty *prop) const {
  return QString::number(m_doubleManager->value(prop));
}

/**
 * Set a new value of a double algorithm property.
 * @param prop :: The corresponding QtProperty.
 * @param value :: The new value.
 */
void FitOptionsBrowser::setDoubleProperty(QtProperty *prop, const QString &value) {
  m_doubleManager->setValue(prop, value.toDouble());
}

/**
 * Get the value of a bool algorithm property.
 * @param prop :: The corresponding QtProperty.
 */
QString FitOptionsBrowser::getBoolProperty(QtProperty *prop) const {
  return QString::number(m_boolManager->value(prop));
}

/**
 * Set a new value of a bool algorithm property.
 * @param prop :: The corresponding QtProperty.
 * @param value :: The new value.
 */
void FitOptionsBrowser::setBoolProperty(QtProperty *prop, const QString &value) {
  bool boolValue = (value == "1") || (value.toLower() == "true");
  m_boolManager->setValue(prop, boolValue);
}

/**
 * Get the value of a string algorithm property with predefined set of values.
 * @param prop :: The corresponding QtProperty.
 */
QString FitOptionsBrowser::getStringEnumProperty(QtProperty *prop) const {
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
void FitOptionsBrowser::setStringEnumProperty(QtProperty *prop, const QString &value) {
  int i = m_enumManager->enumNames(prop).indexOf(value);
  if (i >= 0)
    m_enumManager->setValue(prop, i);
}

/**
 * Get the value of a string algorithm property.
 * @param prop :: The corresponding QtProperty.
 */
QString FitOptionsBrowser::getStringProperty(QtProperty *prop) const { return m_stringManager->value(prop); }

/**
 * Set a new value of a string algorithm property.
 * @param prop :: The corresponding QtProperty.
 * @param value :: The new value.
 */
void FitOptionsBrowser::setStringProperty(QtProperty *prop, const QString &value) {
  m_stringManager->setValue(prop, value);
}

// ------------------------------------------------------------------------------------//

/**
 * Save the last property values in settings.
 * @param settings :: A QSettings instance provided by the user of this class.
 */
void FitOptionsBrowser::saveSettings(QSettings &settings) const {
  for (auto p = m_propertyNameMap.constBegin(); p != m_propertyNameMap.constEnd(); ++p) {
    auto prop = p.value();
    auto f = m_getters[prop];
    settings.setValue(p.key(), (this->*f)(prop));
  }
}

/**
 * Load property values from settings.
 * @param settings :: A QSettings instance provided by the user of this class.
 */
void FitOptionsBrowser::loadSettings(const QSettings &settings) {
  for (auto p = m_propertyNameMap.constBegin(); p != m_propertyNameMap.constEnd(); ++p) {
    QString value = settings.value(p.key()).toString();
    if (!value.isEmpty()) {
      auto prop = p.value();
      auto f = m_setters[prop];
      (this->*f)(prop, value);
    }
  }
}

/**
 * Get the current fitting type, ie which algorithm to use:
 *    Simultaneous for Fit and Sequential for PlotPeakByLogValue.
 */
FittingMode FitOptionsBrowser::getCurrentFittingType() const {
  auto value = m_enumManager->value(m_fittingTypeProp);
  return static_cast<FittingMode>(value);
}

/**
 * Set the current fitting type, ie which algorithm to use:
 *    Simultaneous for Fit and Sequential for PlotPeakByLogValue.
 */
void FitOptionsBrowser::setCurrentFittingType(FittingMode fitType) {
  if (fitType == FittingMode::SIMULTANEOUS) {
    m_enumManager->setValue(m_fittingTypeProp, 1);
  } else {
    m_enumManager->setValue(m_fittingTypeProp, 0);
  }
}

/**
 * Lock the browser in a particular fitting type state. Disable the switch
 * option.
 * @param fitType :: Fitting type to lock the browser in.
 */
void FitOptionsBrowser::lockCurrentFittingType(FittingMode fitType) {
  setCurrentFittingType(fitType);
  m_fittingTypeProp->setEnabled(false);
}

/**
 * Make the fitting type changeable again.
 */
void FitOptionsBrowser::unlockCurrentFittingType() { m_fittingTypeProp->setEnabled(true); }

/**
 * Set values for an enum property.
 * @param prop :: A property to set the values to.
 * @param values :: New enum values.
 */
void FitOptionsBrowser::setPropertyEnumValues(QtProperty *prop, const QStringList &values) {
  auto i = m_enumManager->value(prop);
  if (!values.isEmpty() && values.front().isEmpty()) {
    m_enumManager->setEnumNames(prop, values);
  } else {
    QStringList names = values;
    names.insert(0, "");
    m_enumManager->setEnumNames(prop, names);
  }
  if (i < values.size()) {
    m_enumManager->setValue(prop, i);
  } else {
    m_enumManager->setValue(prop, 0);
  }
}

/**
 * Define log names to use with the LogValue property.
 * @param logNames :: The log names
 */
void FitOptionsBrowser::setLogNames(const QStringList &logNames) { setPropertyEnumValues(m_logValue, logNames); }

/**
 * Define names of function parameters that can be plotted against the LogValue.
 */
void FitOptionsBrowser::setParameterNamesForPlotting(const QStringList &parNames) {
  setPropertyEnumValues(m_plotParameter, parNames);
}

/**
 * Get name of a function parameter to plot against LogValue after sequential
 * fitting.
 */
QString FitOptionsBrowser::getParameterToPlot() const {
  auto i = m_enumManager->value(m_plotParameter);
  if (i < 0)
    i = 0;
  return m_enumManager->enumNames(m_plotParameter)[i];
}

/*                *************************
 *                **  Protected Members  **
 *                *************************/

/**
 * @brief Declares a property of type double, inserting it in the QMap
 * attributes.
 * Note: It does not add it to the browser. Use displayProperty() for this.
 * @exception std::runtime_error if property already declared
 * @return a raw pointer to the created property.
 */
QtProperty *FitOptionsBrowser::addDoubleProperty(const QString &propertyName) {
  if (m_propertyNameMap.contains(propertyName)) {
    throw std::runtime_error("Property " + propertyName.toStdString() + " already added.");
  }
  QtProperty *property = m_doubleManager->addProperty(propertyName);
  m_doubleManager->setDecimals(property, m_decimals);
  m_doubleManager->setRange(property, std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());
  this->addProperty(propertyName, property, &FitOptionsBrowser::getDoubleProperty,
                    &FitOptionsBrowser::setDoubleProperty);
  return property;
}

/**
 * @brief Show or hide in the browser a supported property
 * @param propertyName name of the existing property
 * @param show toggles the visibility of the property on/off
 * @pre if property is to be shown, property should not have been previously
 * added to the browser
 * @pre if property is to be hidden, property should not have been previously
 * removed from the browser
 */
void FitOptionsBrowser::displayProperty(const QString &propertyName, bool show) {
  if (!m_propertyNameMap.contains(propertyName)) {
    throw std::runtime_error("Property " + propertyName.toStdString() + " isn't supported by the browser.");
  }
  auto prop = m_propertyNameMap[propertyName];
  if (show) {
    m_browser->addProperty(prop);
  } else {
    m_browser->removeProperty(prop);
  }
}

/**
 * Adds the property with the given name to a blacklist of properties to hide
 */
bool FitOptionsBrowser::addPropertyToBlacklist(const QString &name) {
  if (!m_propertyNameMap.contains(name)) {
    return false;
  }
  auto prop = m_propertyNameMap[name];
  m_sequentialProperties.removeAll(prop);
  m_simultaneousProperties.removeAll(prop);
  m_browser->removeProperty(prop);
  return true;
}

/**
 * Show sequential fit (PlotPeakByLogValue) properties and hide the others.
 */
void FitOptionsBrowser::displaySequentialFitProperties() {
  foreach (QtProperty *prop, m_sequentialProperties) {
    m_browser->addProperty(prop);
  }
  foreach (QtProperty *prop, m_simultaneousProperties) {
    m_browser->removeProperty(prop);
  }
  emit changedToSequentialFitting();
}

} // namespace MantidQt::MantidWidgets
