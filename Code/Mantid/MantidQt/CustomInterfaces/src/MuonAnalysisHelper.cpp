#include "MantidQtCustomInterfaces/MuonAnalysisHelper.h"

#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>

#include <stdexcept>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace MuonAnalysisHelper
{

/**
 * Sets double validator for specified field.
 * @param field :: Field to set validator for
 */
void setDoubleValidator(QLineEdit* field)
{
  QDoubleValidator* newValidator = new QDoubleValidator(field);
  newValidator->setNotation(QDoubleValidator::StandardNotation);
  field->setValidator(newValidator);
}

/**
 * Constructor
 * @param groupName :: The top-level group to use for all the widgets
 */
WidgetAutoSaver::WidgetAutoSaver(const QString& groupName)
{
  m_settings.beginGroup(groupName);
}

/**
 * Register new widget for auto-saving.
 * @param widget :: A pointer to the widget
 * @param name :: A name to use when saving/loading
 * @param defaultValue :: A value to load when the widget has not been saved yet
 */
void WidgetAutoSaver::registerWidget(QWidget *widget, const QString& name, QVariant defaultValue)
{
  m_registeredWidgets.push_back(widget);
  m_widgetNames[widget] = name;
  m_widgetDefaultValues[widget] = defaultValue;
  m_widgetGroups[widget] = m_settings.group(); // Current group set up using beginGroup and endGroup
}

/**
 * Return a signal (which can be used instead of SIGNAL()) which is emmited when given widget is
 * changed.
 * @param widget
 * @return A signal you can use instead of SIGNAL() to determine when widget value was changed
 */
const char* WidgetAutoSaver::changedSignal(QWidget *widget)
{
  if ( qobject_cast<QLineEdit*>(widget) )
  {
    return SIGNAL(textChanged(QString));
  }
  else if ( qobject_cast<QCheckBox*>(widget) )
  {
    return SIGNAL(stateChanged(int));
  }
  else if ( qobject_cast<QComboBox*>(widget) )
  {
    return SIGNAL(currentIndexChanged(int));
  }
  // ... add more as neccessary
  else
  {
    throw std::runtime_error("Unsupported widget type");
  }
}

/**
 * Enable/disable auto-saving of all the registered widgets.
 * @param enabled :: Whether auto-saving should be enabled or disabled
 */
void WidgetAutoSaver::setAutoSaveEnabled(bool enabled)
{
  foreach (QWidget* w, m_registeredWidgets)
  {
    setAutoSaveEnabled(w, enabled);
  }
}

/**
 * Enable/disable auto-saving of all the registered widgets.
 * @param widget :: Registered widget for which to enable/disable auto-saving
 * @param enabled :: Whether auto-saving should be enabled or disabled
 */
void WidgetAutoSaver::setAutoSaveEnabled(QWidget* widget, bool enabled)
{
  if (enabled)
    connect(widget, changedSignal(widget), this, SLOT(saveWidgetValue()));
  else
    disconnect(widget, changedSignal(widget), this, SLOT(saveWidgetValue()));
}

/**
 * Saves the value of the registered widget which signalled the slot
 */
void WidgetAutoSaver::saveWidgetValue()
{
  // Get the widget which called the slot
  QWidget* sender = qobject_cast<QWidget*>(QObject::sender());

  if(!sender)
    throw std::runtime_error("Unable to save value of non-widget QObject");

  const QString& senderName = m_widgetNames[sender];
  const QString& senderGroup = m_widgetGroups[sender];

  QSettings settings;
  settings.beginGroup(senderGroup);

  if ( auto w = qobject_cast<QLineEdit*>(sender) )
  {
    settings.setValue(senderName, w->text());
  }
  else if ( auto w = qobject_cast<QCheckBox*>(sender) )
  {
    settings.setValue(senderName, w->isChecked());
  }
  else if ( auto w = qobject_cast<QComboBox*>(sender) )
  {
    settings.setValue(senderName, w->currentIndex());
  }
  // ... add more as neccessary
}

/**
 * Load the auto-saved (or default) value of the given widget.
 * @param widget :: Widget to load saved value for
 */
void WidgetAutoSaver::loadWidgetValue(QWidget *widget)
{
  const QString& name = m_widgetNames[widget];
  const QString& group = m_widgetGroups[widget];
  QVariant defaultValue = m_widgetDefaultValues[widget];

  QSettings settings;
  settings.beginGroup(group);

  QVariant value = settings.value(name, defaultValue);

  if ( auto w = qobject_cast<QLineEdit*>(widget) )
  {
    w->setText(value.toString());
  }
  else if ( auto w = qobject_cast<QCheckBox*>(widget) )
  {
    w->setChecked(value.toBool());
  }
  else if ( auto w = qobject_cast<QComboBox*>(widget) )
  {
    w->setCurrentIndex(value.toInt());
  }
  // ... add more as neccessary
}

/**
 * Load the auto-saved (or default) value of all the registered widgets.
 */
void WidgetAutoSaver::loadWidgetValues()
{
  foreach (QWidget* w, m_registeredWidgets)
  {
    loadWidgetValue(w);
  }
}

/**
 * Begin new-auto save group. All the registerWidget calls between this and next beginGroup will be
 * put in the given group.
 * @param name :: The name of the group
 */
void WidgetAutoSaver::beginGroup(const QString &name)
{
  m_settings.beginGroup(name);
}

/**
 * Ends the scope of the previous begin group.
 */
void WidgetAutoSaver::endGroup()
{
  m_settings.endGroup();
}

} // namespace MuonAnalysisHelper
} // namespace CustomInterfaces
} // namespace Mantid
