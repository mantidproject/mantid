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

WidgetAutoSaver::WidgetAutoSaver(const QString& groupName)
{
  m_settings.beginGroup(groupName);
}

void WidgetAutoSaver::registerWidget(QWidget *widget, const QString& name, QVariant defaultValue)
{
  registeredWidgets.push_back(widget);
  widgetNames[widget] = name;
  widgetDefaultValues[widget] = defaultValue;
  widgetGroups[widget] = m_settings.group(); // Current group set up using beginGroup and endGroup
}

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

void WidgetAutoSaver::setAutoSaveEnabled(bool enabled)
{
  foreach (QWidget* w, registeredWidgets)
  {
    setAutoSaveEnabled(w, enabled);
  }
}

void WidgetAutoSaver::setAutoSaveEnabled(QWidget &widget, bool enabled)
{
  if (enabled)
    connect(widget, changedSignal(widget), this, SLOT(saveWidgetValue()));
  else
    disconnect(widget, changedSignal(widget), this, SLOT(saveWidgetValue()));
}

void WidgetAutoSaver::saveWidgetValue()
{
  // Get the widget which called the slot
  QWidget* sender = qobject_cast<QWidget*>(QObject::sender());

  if(!sender)
    throw std::runtime_error("Unable to save value of non-widget QObject");

  const QString& senderName = widgetNames[sender];
  const QString& senderGroup = widgetGroups[sender];

  QSettings settings;
  settings.beginGroup(senderGroup);

  if ( auto w = qobject_cast<QLineEdit*>(sender) )
  {
    settings.setValue(senderName, w->text());
  }
  else if ( auto w = qobject_cast<QCheckBox*>(sender) )
  {
    settings.setValue(senderName, static_cast<int>(w->checkState()));
  }
  else if ( auto w = qobject_cast<QComboBox*>(sender) )
  {
    settings.setValue(senderName, w->currentIndex());
  }
  // ... add more as neccessary
}

void WidgetAutoSaver::loadWidgetValue(QWidget *widget)
{
  const QString& name = widgetNames[widget];
  const QString& group = widgetGroups[widget];
  QVariant defaultValue = widgetDefaultValues[widget];

  QSettings settings;
  settings.beginGroup(group);

  QVariant value = settings.value(name, defaultValue);

  if ( auto w = qobject_cast<QLineEdit*>(widget) )
  {
    w->setText(value.toString());
  }
  else if ( auto w = qobject_cast<QCheckBox*>(widget) )
  {
    w->setCheckState(static_cast<Qt::CheckState>(value.toInt()));
  }
  else if ( auto w = qobject_cast<QComboBox*>(widget) )
  {
    w->setCurrentIndex(value.toInt());
  }
  // ... add more as neccessary
}

void WidgetAutoSaver::loadWidgetValues()
{
  foreach (QWidget* w, registeredWidgets)
  {
    loadWidgetValue(w);
  }
}

void WidgetAutoSaver::beginGroup(const QString &name)
{
  m_settings.beginGroup(name);
}

void WidgetAutoSaver::endGroup()
{
  m_settings.endGroup();
}

} // namespace MuonAnalysisHelper
} // namespace CustomInterfaces
} // namespace Mantid
