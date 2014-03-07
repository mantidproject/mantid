#include "ParameterPropertyManager.h"
#include "qtpropertymanager.h"

ParameterPropertyManager::ParameterPropertyManager(QObject *parent)
  : QtDoublePropertyManager(parent)
{}

/**
 * Adds error parameter value to property display
 * @param property :: Property we want to display
 * @return Text representation of the property
 * @see QtAbstractPropertyManager
 */
QString ParameterPropertyManager::valueText(const QtProperty* property) const
{
  return QtDoublePropertyManager::valueText(property) + " (Error here)";
}
