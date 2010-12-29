#include "MantidQtMantidWidgets/MWRunFiles.h"
#include "MantidQtDesignerPlugins/FileFinderPlugin.h"
#include <QDesignerFormEditorInterface>
#include <QtPlugin>

using namespace MantidQt::MantidWidgets;

/**
 * Default constructor
 * @param parent The parent of the plugin (default: NULL)
 */
FileFinderPlugin::FileFinderPlugin(QObject *parent) : QObject(parent), m_initialized(false)
{
}

/**
 * Initialize the plugin
 * @param formEditor A pointer to the interface that will control this plugin
 */
void FileFinderPlugin::initialize(QDesignerFormEditorInterface * formEditor)
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
 * @param parent The parent widget
 * @returns A newly constructed widget for the wrapped type.
 */
QWidget *FileFinderPlugin::createWidget(QWidget *parent)
{
  return new MWRunFiles(parent);
}

/**
* Returns whether the plugin initialized or not
* @returns True if initialize() has been called, false otherwise
*/
bool FileFinderPlugin::isInitialized() const
{
  return m_initialized;
}

/**
 * Returns whether this widget can contain other widgets
 * @returns True if other widgets can be placed within this widget, false otherwise
 */
bool FileFinderPlugin::isContainer() const
{
  return false;
}

/**
 * Returns the class name of the widget that this plugin manages
 * @returns A string containing the fully qualified class name of the widget
 */
QString FileFinderPlugin::name() const
{
  return "MantidQt::MantidWidgets::MWRunFiles";
}

/**
 * Returns the group within the designer that this plugin should be placed
 * @returns The name of the group of widgets in the designer 
 */
QString FileFinderPlugin::group() const
{
  return "MantidWidgets";
}

/**
 * Returns the icon to display in the designer
 * @returns An icon that is used within the designer
 */
QIcon FileFinderPlugin::icon() const
{
  return QIcon();
}

/**
 * The tooltip for the widget
 * @returns A string containing the tooltip for this widget
 */
QString FileFinderPlugin::toolTip() const
{
  return "Searches for the given files within the paths defined by\n"
         "Mantid's datasearch.directories property.";
}

/** A short description of the widget
 * @returns A string containing a short description of the widget
 */
QString FileFinderPlugin::whatsThis() const
{  
  return "A file finder widget";
}

/**
 * The include file to use when generating the header file
 * @returns A string containing the path to the widget's header file
 */
QString FileFinderPlugin::includeFile() const
{
  return "MantidQtMantidWidgets/MWRunFiles.h";
}

/**
 * Returns the XML used to define the widget in the designer
 * @returns A string containing the XML for the widget
 */
QString FileFinderPlugin::domXml() const
{
  return "<widget class=\"MantidQt::MantidWidgets::MWRunFiles\" name=\"mwRunFiles\">\n"
    " <property name=\"label\">\n"
    "  <string>TextLabel</string>\n"
    " </property>\n"
    " <property name=\"geometry\">\n"
    "  <rect>\n"
    "   <x>0</x>\n"
    "   <y>0</y>\n"
    "   <width>300</width>\n"
    "   <height>20</height>\n"
    "  </rect>\n"
    " </property>\n"
    "</widget>\n";
}
