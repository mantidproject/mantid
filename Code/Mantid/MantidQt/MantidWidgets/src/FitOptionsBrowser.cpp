#include "MantidQtMantidWidgets/FitOptionsBrowser.h"

#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/PropertyWithValue.h"

#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "qteditorfactory.h"
#include "DoubleEditorFactory.h"
#include "CompositeEditorFactory.h"
#include "ButtonEditorFactory.h"
#if defined(__INTEL_COMPILER)
  #pragma warning enable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic pop
  #endif
#endif

#include <QVBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <limits>

namespace MantidQt
{
namespace MantidWidgets
{

/**
 * Constructor
 * @param parent :: The parent widget.
 */
FitOptionsBrowser::FitOptionsBrowser(QWidget *parent)
  :QWidget(parent),
  m_decimals(6)
{
  // create m_browser
  createBrowser();
  createProperties();

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->addWidget(m_browser);
  layout->setContentsMargins(0,0,0,0);

}

/**
 * Create the Qt property browser and set up property managers.
 */
void FitOptionsBrowser::createBrowser()
{
  /* Create property managers: they create, own properties, get and set values  */
  m_stringManager = new QtStringPropertyManager(this);
  m_doubleManager = new QtDoublePropertyManager(this);
  m_intManager = new QtIntPropertyManager(this);
  m_boolManager = new QtBoolPropertyManager(this);
  m_enumManager = new QtEnumPropertyManager(this);
  m_groupManager = new QtGroupPropertyManager(this);

  // create editor factories
  QtSpinBoxFactory *spinBoxFactory = new QtSpinBoxFactory(this);
  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory(this);
  QtLineEditFactory *lineEditFactory = new QtLineEditFactory(this);
  QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(this);
  QtEnumEditorFactory *comboBoxFactory = new QtEnumEditorFactory(this);

  m_browser = new QtTreePropertyBrowser(NULL, QStringList(), false);
  // assign factories to property managers
  m_browser->setFactoryForManager(m_stringManager, lineEditFactory);
  m_browser->setFactoryForManager(m_doubleManager, doubleEditorFactory);
  m_browser->setFactoryForManager(m_intManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_boolManager, checkBoxFactory);
  m_browser->setFactoryForManager(m_enumManager, comboBoxFactory);

  //m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
  //connect(m_browser, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));

  connect(m_enumManager,SIGNAL(propertyChanged(QtProperty*)),this,SLOT(enumChanged(QtProperty*)));

  // Fill in getter and setter maps
}

/**
 * Create browser's QtProperties
 */
void FitOptionsBrowser::createProperties()
{
  // Create MaxIterations property
  m_maxIterations = m_intManager->addProperty("Max Iterations");
  {
    m_intManager->setValue(m_maxIterations,500);
    m_intManager->setMinimum(m_maxIterations,0);
    m_browser->addProperty(m_maxIterations);

    addProperty("MaxIterations", &FitOptionsBrowser::getMaxIterations, &FitOptionsBrowser::setMaxIterations);
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
    std::vector<std::string> minimizerOptions =
        Mantid::API::FuncMinimizerFactory::Instance().getKeys();
    QStringList minimizers;

    // Store them in the m_minimizer enum property
    for(auto it = minimizerOptions.begin(); it != minimizerOptions.end(); ++it)
    {
      minimizers << QString::fromStdString(*it);
    }
    m_enumManager->setEnumNames(m_minimizer,minimizers);
    int i = m_enumManager->enumNames(m_minimizer).indexOf("Levenberg-Marquardt");
    if ( i >= 0 )
    {
      m_enumManager->setValue(m_minimizer,i);
    }

    m_browser->addProperty(m_minimizerGroup);

    addProperty("Minimizer", &FitOptionsBrowser::getMinimizer, &FitOptionsBrowser::setMinimizer);
  }

  // Create cost function property
  m_costFunction = m_enumManager->addProperty("Cost Function");
  {
    // Get names of registered cost functions from the factory
    std::vector<std::string> costOptions =
      Mantid::API::CostFunctionFactory::Instance().getKeys();
    QStringList costFunctions;

    // Store them in the m_minimizer enum property
    for(auto it = costOptions.begin(); it != costOptions.end(); ++it)
    {
      costFunctions << QString::fromStdString(*it);
    }
    m_enumManager->setEnumNames(m_costFunction,costFunctions);

    m_browser->addProperty(m_costFunction);

    addProperty("CostFunction", &FitOptionsBrowser::getCostFunction, &FitOptionsBrowser::setCostFunction);
  }

  // Create Output property
  m_output = m_stringManager->addProperty("Output");
  {
    m_browser->addProperty(m_output);
    addProperty("Output", &FitOptionsBrowser::getOutput, &FitOptionsBrowser::setOutput);
  }

  // Create Ignore property
  m_ignoreInvalidData = m_boolManager->addProperty("Ignore Invalid Data");
  {
    m_browser->addProperty(m_ignoreInvalidData);
    addProperty("IgnoreInvalidData", &FitOptionsBrowser::getIgnoreInvalidData, &FitOptionsBrowser::setIgnoreInvalidData);
  }
}

void FitOptionsBrowser::addProperty(const QString& name, 
  QString (FitOptionsBrowser::*getter)()const, 
  void (FitOptionsBrowser::*setter)(const QString&))
{
  m_getters[name] = getter;
  m_setters[name] = setter;
}


/** Create a double property and set some settings
 * @param name :: The name of the new property
 * @param manager :: The current property manager
 * @return Pointer to the created property
 */
QtProperty* FitOptionsBrowser::addDoubleProperty(const QString& name)
{
  QtProperty* prop = m_doubleManager->addProperty(name);
  m_doubleManager->setDecimals(prop,m_decimals);
  m_doubleManager->setRange(prop,-std::numeric_limits<double>::max(),std::numeric_limits<double>::max());
  return prop;
}

/**
 * Update the browser when an enum property changes.
 * @param prop :: Property that changed its value.
 */
void FitOptionsBrowser::enumChanged(QtProperty* prop)
{
  if (prop == m_minimizer)
  {
    updateMinimizer();
  }
}

/**
 * Update the browser when minimizer changes.
 */
void FitOptionsBrowser::updateMinimizer()
{
  int i = m_enumManager->value(m_minimizer);
  QString minimizerName = m_enumManager->enumNames(m_minimizer)[i];
  m_minimizerGroup->setPropertyName("Minimizer " + minimizerName);

  // Remove properties of the old minimizer
  auto subProperties = m_minimizerGroup->subProperties();
  foreach(QtProperty* prop, subProperties)
  {
    if ( prop != m_minimizer )
    {
      m_minimizerGroup->removeSubProperty(prop);
    }
  }

  // Check if the new minimizer has its own properties
  auto minimizer = 
    Mantid::API::FuncMinimizerFactory::Instance().createMinimizer(minimizerName.toStdString());

  // Create and add properties to the minimizer group
  auto minimizerProperties = minimizer->getProperties();
  for(auto property = minimizerProperties.begin(); property != minimizerProperties.end(); ++property)
  {
    auto prop = createPropertyProperty( *property );
    if ( !*property ) continue;
    m_minimizerGroup->addSubProperty( prop );
  }
}

/**
 * Create a QtProperty for an Algorithm Property
 * and attach it to the correct manager.
 * @param property :: An algorithm property.
 */
QtProperty* FitOptionsBrowser::createPropertyProperty(Mantid::Kernel::Property* property)
{
    QString propName = QString::fromStdString( property->name() );
    QtProperty* prop = NULL;
    if ( auto prp = dynamic_cast<Mantid::Kernel::PropertyWithValue<bool>* >(property) )
    {
      prop = m_boolManager->addProperty( propName );
      bool val = *prp;
      m_boolManager->setValue( prop, val );
    }
    else if ( auto prp = dynamic_cast<Mantid::Kernel::PropertyWithValue<double>* >(property) )
    {
      prop = this->addDoubleProperty( propName );
      double val = *prp;
      m_doubleManager->setValue( prop, val );
    }
    else if ( auto prp = dynamic_cast<Mantid::Kernel::PropertyWithValue<int>* >(property) )
    {
      prop = m_intManager->addProperty( propName );
      int val = *prp;
      m_intManager->setValue( prop, val );
    }
    else if ( auto prp = dynamic_cast<Mantid::Kernel::PropertyWithValue<size_t>* >(property) )
    {
      prop = m_intManager->addProperty( propName );
      size_t val = *prp;
      m_intManager->setValue( prop, static_cast<int>(val) );
    }
    else if ( auto prp = dynamic_cast<Mantid::Kernel::PropertyWithValue<std::string>* >(property) )
    {
      prop = m_stringManager->addProperty( propName );
      QString val = QString::fromStdString( prp->value() );
      m_stringManager->setValue( prop, val );
    }
    else if ( dynamic_cast<Mantid::API::IWorkspaceProperty* >(property) )
    {
      prop = m_stringManager->addProperty( propName );
      m_stringManager->setValue( prop, QString::fromStdString( property->value() ) );
    }
    else
    {
        QMessageBox::warning(this,"MantidPlot - Error","Type of minimizer's property " + propName + " is not yet supported by the browser.");
        return NULL;
    }

    // Something bad happened in QtPropertyBrowser.
    if ( !prop )
    {
      throw std::runtime_error("Failed to create a QtProperty.");
    }

    // set the tooltip from property doc string
    QString toolTip = QString::fromStdString( property->documentation() );
    if ( !toolTip.isEmpty() )
    {
      prop->setToolTip( toolTip );
    }

    return prop;
}

/**
 * Copy values of the properties to an algorithm.
 * @param fit :: An instance of the Fit algorithm.
 */
void FitOptionsBrowser::copyPropertiesToAlgorithm(Mantid::API::IAlgorithm& fit) const
{
    for(auto p = m_getters.constBegin(); p != m_getters.constEnd(); ++p)
    {
      auto f = p.value();
      fit.setPropertyValue( p.key().toStdString(), (this->*f)().toStdString() );
    }
}

/**
 * Get a string representation of a Fit's property value.
 * @param name :: The name of a Fit's property.
 */
QString FitOptionsBrowser::getProperty(const QString& name) const
{
  if ( !m_getters.contains(name) )
  {
    throw std::runtime_error("Property " + name.toStdString() + " isn't supported by the browser.");
  }
  auto f = m_getters[name];
  return (this->*f)();
}

/**
 * Set a new value to a Fit's property.
 * @param name :: The name of a Fit's property.
 * @param value :: The new value as a string.
 */
void FitOptionsBrowser::setProperty(const QString& name, const QString& value)
{
  if ( !m_getters.contains(name) )
  {
    throw std::runtime_error("Property " + name.toStdString() + " isn't supported by the browser.");
  }
  auto f = m_setters[name];
  (this->*f)(value);
}

/**
 * Get the value of the Minimizer property.
 */
QString FitOptionsBrowser::getMinimizer() const
{
  int i = m_enumManager->value(m_minimizer);
  QString minimStr = m_enumManager->enumNames(m_minimizer)[i];

  auto subProperties = m_minimizerGroup->subProperties();
  if ( subProperties.size() > 1 )
  {
    foreach(QtProperty* prop, subProperties)
    {
      if ( prop == m_minimizer ) continue;
      if ( prop->propertyManager() == m_stringManager )
      {
        QString value = m_stringManager->value(prop);
        if ( !value.isEmpty() )
        {
          minimStr += "," + prop->propertyName() + "=" + value;
        }
      }
      else
      {
        minimStr += "," + prop->propertyName() + "=";
        if ( prop->propertyManager() == m_intManager )
        {
          minimStr += QString::number( m_intManager->value(prop) );
        }
        else if ( prop->propertyManager() == m_doubleManager )
        {
          minimStr += QString::number( m_doubleManager->value(prop) );
        }
        else if ( prop->propertyManager() == m_boolManager )
        {
          minimStr += QString::number( m_boolManager->value(prop) );
        }
        else
        {
          throw std::runtime_error("The fit browser doesn't support the type of minimizer's property " + prop->propertyName().toStdString() );
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
void FitOptionsBrowser::setMinimizer(const QString& value)
{
  QStringList terms = value.split(',');
  int i = m_enumManager->enumNames(m_minimizer).indexOf(terms[0]);
  m_enumManager->setValue(m_minimizer,i);
}

/**
 * Get the value of the CostFunction property.
 */
QString FitOptionsBrowser::getCostFunction() const
{
  int i = m_enumManager->value(m_costFunction);
  return m_enumManager->enumNames(m_costFunction)[i];
}

/**
 * Set new value to the CostFunction property.
 * @param value :: The new value.
 */
void FitOptionsBrowser::setCostFunction(const QString& value)
{
  int i = m_enumManager->enumNames(m_costFunction).indexOf(value);
  m_enumManager->setValue(m_costFunction,i);
}

/**
 * Get the value of the MaxIterations property.
 */
QString FitOptionsBrowser::getMaxIterations() const
{
  return QString::number(m_intManager->value(m_maxIterations));
}

/**
 * Set new value to the MaxIterations property.
 * @param value :: The new value.
 */
void FitOptionsBrowser::setMaxIterations(const QString& value)
{
  m_intManager->setValue(m_maxIterations,value.toInt());
}

/**
 * Get the value of the Output property.
 */
QString FitOptionsBrowser::getOutput() const
{
  return m_stringManager->value(m_output);
}

/**
 * Set new value to the Output property.
 * @param value :: The new value.
 */
void FitOptionsBrowser::setOutput(const QString& value)
{
  m_stringManager->setValue(m_output,value);
}

/**
 * Get the value of the IgnoreInvalidData property.
 */
QString FitOptionsBrowser::getIgnoreInvalidData() const
{
  return QString::number(m_boolManager->value(m_ignoreInvalidData));
}

/**
 * Set new value to the IgnoreInvalidData property.
 * @param value :: The new value.
 */
void FitOptionsBrowser::setIgnoreInvalidData(const QString& value)
{
  bool boolValue = (value == "1") || (value == "true");
  m_boolManager->setValue( m_ignoreInvalidData, boolValue );
}

/**
 * Save the last property values in settings.
 * @param settings :: A QSettings instance provided by the user of this class.
 */
void FitOptionsBrowser::saveSettings(QSettings& settings) const
{
  for(auto p = m_getters.constBegin(); p != m_getters.constEnd(); ++p)
  {
    auto f = p.value();
    settings.setValue( p.key(), (this->*f)() );
  }
}

/**
 * Load property values from settings.
 * @param settings :: A QSettings instance provided by the user of this class.
 */
void FitOptionsBrowser::loadSettings(const QSettings& settings)
{
  for(auto p = m_setters.constBegin(); p != m_setters.constEnd(); ++p)
  {
    QString value = settings.value( p.key() ).toString();
    if ( !value.isEmpty() )
    {
      auto f = p.value();
      (this->*f)( value );
    }
  }
}


} // MantidWidgets
} // MantidQt
