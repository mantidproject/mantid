//-----------------------------------------------------------
// Includes
//-----------------------------------------------------------
#include "MantidQtDesignerPlugins/ScriptEditorPlugin.h"
#include "MantidQtMantidWidgets/ScriptEditor.h"
#include <QDesignerFormEditorInterface>
#include <QtPlugin>

//Q_EXPORT_PLUGIN2(ScriptEditorPluginName, ScriptEditorPlugin)

/**
 * Default constructor
 * @param parent :: The parent of the plugin (default: NULL)
 */
ScriptEditorPlugin::ScriptEditorPlugin(QObject *parent) : QObject(parent), m_initialized(false)
{
}

/**
 * Initialize the plugin
 * @param formEditor :: A pointer to the interface that will control this plugin
 */
void ScriptEditorPlugin::initialize(QDesignerFormEditorInterface * formEditor)
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
QWidget *ScriptEditorPlugin::createWidget(QWidget *parent)
{
  return new ScriptEditor(parent);
}

/**
* Returns whether the plugin initialized or not
* @returns True if initialize() has been called, false otherwise
*/
bool ScriptEditorPlugin::isInitialized() const
{
  return m_initialized;
}

/**
 * Returns whether this widget can contain other widgets
 * @returns True if other widgets can be placed within this widget, false otherwise
 */
bool ScriptEditorPlugin::isContainer() const
{
  return false;
}

/**
 * Returns the class name of the widget that this plugin manages
 * @returns A string containing the fully qualified class name of the widget
 */
QString ScriptEditorPlugin::name() const
{
  return "ScriptEditor";
}

/**
 * Returns the group within the designer that this plugin should be placed
 * @returns The name of the group of widgets in the designer 
 */
QString ScriptEditorPlugin::group() const
{
  return "MantidWidgets";
}

/**
 * Returns the icon to display in the designer
 * @returns An icon that is used within the designer
 */
QIcon ScriptEditorPlugin::icon() const
{
  return QIcon();
}

/**
 * The tooltip for the widget
 * @returns A string containing the tooltip for this widget
 */
QString ScriptEditorPlugin::toolTip() const
{
  return "A python script editor window with highlighting.";
}

/** A short description of the widget
 * @returns A string containing a short description of the widget
 */
QString ScriptEditorPlugin::whatsThis() const
{  
  return "A Python ScriptEditor window";
}

/**
 * The include file to use when generating the header file
 * @returns A string containing the path to the widget's header file
 */
QString ScriptEditorPlugin::includeFile() const
{
  return "MantidQtMantidWidgets/ScriptEditor.h";
}

/**
 * Returns the XML used to define the widget in the designer
 * @returns A string containing the XML for the widget
 */
QString ScriptEditorPlugin::domXml() const
{
  return 
    "<widget class=\"ScriptEditor\" name=\"ScriptEditor\">\n"
    //" <property name=\"geometry\">\n"
    //"  <rect>\n"
    //"   <x>0</x>\n"
    //"   <y>0</y>\n"
    //"   <width>300</width>\n"
    //"   <height>20</height>\n"
    //"  </rect>\n"
    //" </property>\n"
    "</widget>\n";
}
