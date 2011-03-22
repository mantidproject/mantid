//-----------------------------------------------------------
// Includes
//-----------------------------------------------------------
#include "MantidQtDesignerPlugins/WorkspaceSelectorPlugin.h"
#include "MantidQtMantidWidgets/WorkspaceSelector.h"

#include <QDesignerFormEditorInterface>
#include <QtPlugin>

using namespace MantidQt::MantidWidgets;

/**
 * Default constructor
 * @param parent :: The parent of the plugin (default: NULL)
 */
WorkspaceSelectorPlugin::WorkspaceSelectorPlugin(QObject *parent) : QObject(parent), m_initialized(false)
{
}

/**
 * Initialize the plugin
 * @param formEditor :: A pointer to the interface that will control this plugin
 */
void WorkspaceSelectorPlugin::initialize(QDesignerFormEditorInterface * formEditor)
{
  (void) formEditor;
  if (m_initialized)
  {
    return;
  }
  m_initialized = true;
}

/**
 * Create a widget of the type wrapped by the plugin
 * @param parent :: The parent widget
 * @returns A newly constructed widget for the wrapped type.
 */
QWidget *WorkspaceSelectorPlugin::createWidget(QWidget *parent)
{
  return new WorkspaceSelector(parent, false);
}

/**
* Returns whether the plugin initialized or not
* @returns True if initialize() has been called, false otherwise
*/
bool WorkspaceSelectorPlugin::isInitialized() const
{
  return m_initialized;
}

/**
 * Returns whether this widget can contain other widgets
 * @returns True if other widgets can be placed within this widget, false otherwise
 */
bool WorkspaceSelectorPlugin::isContainer() const
{
  return false;
}

/**
 * Returns the class name of the widget that this plugin manages
 * @returns A string containing the fully qualified class name of the widget
 */
QString WorkspaceSelectorPlugin::name() const
{
  return "MantidQt::MantidWidgets::WorkspaceSelector";
}

/**
 * Returns the group within the designer that this plugin should be placed
 * @returns The name of the group of widgets in the designer 
 */
QString WorkspaceSelectorPlugin::group() const
{
  return "MantidWidgets";
}

/**
 * Returns the icon to display in the designer
 * @returns An icon that is used within the designer
 */
QIcon WorkspaceSelectorPlugin::icon() const
{
  return QIcon();
}

/**
 * The tooltip for the widget
 * @returns A string containing the tooltip for this widget
 */
QString WorkspaceSelectorPlugin::toolTip() const
{
  return "Select a workspace for use in this operation";
}

/** A short description of the widget
 * @returns A string containing a short description of the widget
 */
QString WorkspaceSelectorPlugin::whatsThis() const
{  
  return "A workspace selector widget.";
}

/**
 * The include file to use when generating the header file
 * @returns A string containing the path to the widget's header file
 */
QString WorkspaceSelectorPlugin::includeFile() const
{
  return "MantidQtMantidWidgets/WorkspaceSelector.h";
}

/**
 * Returns the XML used to define the widget in the designer
 * @returns A string containing the XML for the widget
 */
QString WorkspaceSelectorPlugin::domXml() const
{
  return 
    "<widget class=\"MantidQt::MantidWidgets::WorkspaceSelector\" name=\"workspaceSelector\">\n"
    "</widget>\n";
}
