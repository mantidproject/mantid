#include "ParameterPropertyManager.h"

#include "qtpropertymanager.h"

#include <stdexcept>

ParameterPropertyManager::ParameterPropertyManager(QObject *parent)
  : QtDoublePropertyManager(parent)
{}

/**
 * Throws if property error is not set
 * @param property :: Property to get error for
 * @return Error value of the property
 */
double ParameterPropertyManager::error(const QtProperty* property) const
{
  if (!m_errors.contains(property))
    throw std::runtime_error("Parameter doesn't have error value set");

  return m_errors[property];
}

/**
 * @param property :: Property to check
 * @return True if error was set for the property, false otherwise
 */
bool ParameterPropertyManager::isErrorSet(const QtProperty* property) const
{
  return m_errors.contains(property);
}

/**
 * @param property :: Property to set error for
 * @param error :: Error value to set
 */
void ParameterPropertyManager::setError(const QtProperty* property, double error)
{
  m_errors[property] = error;
}

/**
 * Adds error parameter value to property display
 * @param property :: Property we want to display
 * @return Text representation of the property
 * @see QtAbstractPropertyManager
 */
QString ParameterPropertyManager::valueText(const QtProperty* property) const
{
  QString originalValueText = QtDoublePropertyManager::valueText(property);

  if (isErrorSet(property))
  {
    double propError = error(property);
    int precision = decimals(property);

    return originalValueText + QString(" (%1)").arg(propError, 0, 'g', precision);
  }
  else
  {
    // No error set, so don't append error value
    return originalValueText;
  }
}
