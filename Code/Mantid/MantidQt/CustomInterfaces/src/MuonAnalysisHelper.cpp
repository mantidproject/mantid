#include "MantidQtCustomInterfaces/MuonAnalysisHelper.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

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

using namespace Mantid::API;

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
 * Return a first period MatrixWorkspace in a run workspace. If the run workspace has one period
 * only - it is returned.
 * @param ws :: Run workspace
 */
MatrixWorkspace_sptr firstPeriod(Workspace_sptr ws)
{
  if ( auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(ws) )
  {
    return boost::dynamic_pointer_cast<MatrixWorkspace>( group->getItem(0) );
  }
  else
  {
    return boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
  }
}

/**
 * Returns a number of periods in a run workspace
 * @param ws :: Run wokspace
 * @return Number of periods
 */
size_t numPeriods(Workspace_sptr ws)
{
  if ( auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(ws) )
  {
    return group->size();
  }
  else
  {
    return 1;
  }
}

/**
 * Print various informaion about the run
 * @param runWs :: Run workspace to retrieve information from
 * @param out :: Stream to print to
 */
void printRunInfo(MatrixWorkspace_sptr runWs, std::ostringstream& out)
{
  // Set display style for floating point values
  out << std::fixed << std::setprecision(12);

  out << "\nTitle: " << runWs->getTitle();
  out << "\nComment: " << runWs->getComment();

  const Run& run = runWs->run();

  Mantid::Kernel::DateAndTime start, end;

  // Add the start time for the run
  out << "\nStart: ";
  if ( run.hasProperty("run_start") )
  {
    start = run.getProperty("run_start")->value();
    out << start.toSimpleString();
  }

  // Add the end time for the run
  out << "\nEnd: ";
  if ( run.hasProperty("run_end") )
  {
    end = run.getProperty("run_end")->value();
    out << end.toSimpleString();
  }

  // Add counts to run information
  out << "\nCounts: ";
  double counts(0.0);
  for (size_t i=0; i<runWs->getNumberHistograms(); ++i)
  {
    for (size_t j=0; j<runWs->blocksize(); ++j)
    {
      counts += runWs->dataY(i)[j];
    }
  }
  out << counts/1000000 << " MEv";

  // Add average temperature.
  out << "\nAverage Temperature: ";
  if ( run.hasProperty("Temp_Sample") )
  {
    // Filter the temperatures by the start and end times for the run.
    run.getProperty("Temp_Sample")->filterByTime(start, end);

    // Get average of the values
    double average = run.getPropertyAsSingleValue("Temp_Sample");

    if (average != 0.0)
    {
      out << average;
    }
    else
    {
      out << "Not set";
    }
  }
  else
  {
    out << "Not found";
  }

  // Add sample temperature
  out << "\nSample Temperature: ";
  if ( run.hasProperty("sample_temp") )
  {
    out << run.getPropertyValueAsType<double>("sample_temp");
  }
  else
  {
    out << "Not found";
  }

  // Add sample magnetic field
  out << "\nSample Magnetic Field: ";
  if ( run.hasProperty("sample_magn_field") )
  {
    out << run.getPropertyValueAsType<double>("sample_magn_field");
  }
  else
  {
    out << "Not found";
  }
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
