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

  connectWidget(widget);
}

void WidgetAutoSaver::connectWidget(QWidget *widget)
{
  if ( auto w = qobject_cast<QLineEdit*>(widget) )
  {
    connect(w, SIGNAL(textChanged(QString)), this, SLOT(saveWidgetValue()));
  }
  else if ( auto w = qobject_cast<QCheckBox*>(widget) )
  {
    connect(w, SIGNAL(stateChanged(int)), this, SLOT(saveWidgetValue()));
  }
  else if ( auto w = qobject_cast<QComboBox*>(widget) )
  {
    connect(w, SIGNAL(currentIndexChanged(int)), this, SLOT(saveWidgetValue()));
  }
  // ... add more as neccessary
  else
  {
    throw std::runtime_error("Unsupported widget type");
  }
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
