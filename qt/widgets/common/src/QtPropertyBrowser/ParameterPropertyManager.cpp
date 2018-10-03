// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/QtPropertyBrowser/ParameterPropertyManager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"

#include <cmath>
#include <stdexcept>

const QString ParameterPropertyManager::ERROR_TOOLTIP(" (Error)");

ParameterPropertyManager::ParameterPropertyManager(QObject *parent)
    : QtDoublePropertyManager(parent), m_errors(), m_errorsEnabled(false) {}

/**
 * Throws if property error is not set
 * @param property :: Property to get error for
 * @return Error value of the property
 */
double ParameterPropertyManager::error(const QtProperty *property) const {
  // Cast for searching purposes
  auto prop = const_cast<QtProperty *>(property);

  if (!m_errors.contains(prop))
    throw std::runtime_error("Parameter doesn't have error value set");

  return m_errors[prop];
}

/**
 * @param property :: Parameter property
 * @return Parameter description
 */
std::string
ParameterPropertyManager::description(const QtProperty *property) const {
  // Cast for searching purposes
  auto prop = const_cast<QtProperty *>(property);

  if (!m_descriptions.contains(prop))
    throw std::runtime_error("Parameter doesn't have description set");

  return m_descriptions[prop];
}

/**
 * @param property :: Property to check
 * @return True if error was set for the property, false otherwise
 */
bool ParameterPropertyManager::isErrorSet(const QtProperty *property) const {
  // Cast for searching purposes
  auto prop = const_cast<QtProperty *>(property);

  return m_errors.contains(prop);
}

/**
 * @param property :: Property to set error for
 * @param error :: Error value to set
 */
void ParameterPropertyManager::setError(QtProperty *property,
                                        const double &error) {
  m_errors[property] = error;
  emit propertyChanged(property);
  updateTooltip(property);
}

/**
 * @param property :: Parameter property to set error for
 * @param description :: Description of the parameter
 */
void ParameterPropertyManager::setDescription(QtProperty *property,
                                              const std::string &description) {
  m_descriptions[property] = description;
  updateTooltip(property);
}

/**
 * Clears error of the property, if one was set. If error was not set, the
 * function does nothing.
 * @param property :: Property to clear error for
 */
void ParameterPropertyManager::clearError(QtProperty *property) {
  m_errors.remove(property);
  if (hasProperty(property)) {
    emit propertyChanged(property);
    updateTooltip(property);
  }
}

/**
 * Clears errors of all the properties, if they are set.
 */
void ParameterPropertyManager::clearErrors() {
  const auto properties = m_errors.keys();
  for (const auto prop : properties) {
    clearError(prop);
  }
}

/**
 * Sets errors enabled state. Updates all the properties as well to show/hide
 * errors.
 * @param enabled :: New errors enabled state
 */
void ParameterPropertyManager::setErrorsEnabled(bool enabled) {
  m_errorsEnabled = enabled;

  foreach (QtProperty *prop, m_errors.keys()) {
    emit propertyChanged(prop);
    updateTooltip(prop);
  }
}

/**
 * Adds error parameter value to property display
 * @param property :: Property we want to display
 * @return Text representation of the property
 * @see QtAbstractPropertyManager
 */
QString ParameterPropertyManager::valueText(const QtProperty *property) const {
  QString originalValueText = QtDoublePropertyManager::valueText(property);

  if (areErrorsEnabled() && isErrorSet(property)) {
    double propError = error(property);
    int precision = decimals(property);

    // Format logic taken from QtDoublePropertyManager::valueText
    double absVal = fabs(value(property));
    char format = absVal > 1e5 || (absVal != 0 && absVal < 1e-5) ? 'e' : 'f';

    return originalValueText +
           QString(" (%1)").arg(propError, 0, format, precision);
  } else {
    // No error set or errors disabled, so don't append error value
    return originalValueText;
  }
}

/**
 * @param property :: Property to update tooltip for
 */
void ParameterPropertyManager::updateTooltip(QtProperty *property) const {
  // Description only initially
  QString tooltip = QString::fromStdString(description(property));

  if (areErrorsEnabled() && isErrorSet(property)) {
    // If error is displayed - add description for it
    tooltip += ERROR_TOOLTIP;
  }

  property->setToolTip(tooltip);
}
