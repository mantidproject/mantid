// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plugins/Designer/DesignerPlugin.h"
#include "MantidQtWidgets/Common/AlgorithmSelectorWidget.h"
#include <QDesignerFormEditorInterface>
#include <QtPlugin>

using namespace MantidQt::MantidWidgets;

/**
 * Default constructor
 * @param parent :: The parent of the plugin (default: NULL)
 */
DesignerPlugin::DesignerPlugin(QObject *parent)
    : QObject(parent), m_initialized(false) {}

/**
 * Initialize the plugin
 * @param formEditor :: A pointer to the interface that will control this plugin
 */
void DesignerPlugin::initialize(QDesignerFormEditorInterface *formEditor) {
  (void)formEditor;
  if (m_initialized) {
    return;
  }
  m_initialized = true;
}

/**
 * Returns whether the plugin initialized or not
 * @returns True if initialize() has been called, false otherwise
 */
bool DesignerPlugin::isInitialized() const { return m_initialized; }

/**
 * Returns whether this widget can contain other widgets
 * @returns True if other widgets can be placed within this widget, false
 * otherwise
 */
bool DesignerPlugin::isContainer() const { return false; }

/**
 * Returns the group within the designer that this plugin should be placed
 * @returns The name of the group of widgets in the designer
 */
QString DesignerPlugin::group() const { return "MantidWidgets"; }

/**
 * Returns the icon to display in the designer
 * @returns An icon that is used within the designer
 */
QIcon DesignerPlugin::icon() const { return QIcon(); }

/**
 * The tooltip for the widget
 * @returns A string containing the tooltip for this widget
 */
QString DesignerPlugin::toolTip() const {
  return "Creates a widget of type " + this->name();
}

/** A short description of the widget
 * @returns A string containing a short description of the widget
 */
QString DesignerPlugin::whatsThis() const { return this->toolTip(); }

/** @return the name of the widget without the namespace */
std::string DesignerPlugin::getShortName() const {
  std::string name = this->name().toStdString();
  size_t n = name.rfind(':');
  if (n != std::string::npos) {
    name = name.substr(n + 1, name.size() - n);
  }
  return name;
}

/**
 * The include file to use when generating the header file
 * @returns A string containing the path to the widget's header file
 */
QString DesignerPlugin::includeFile() const {
  std::string thisNamespace = "MantidWidgets";
  std::string name = this->name().toStdString();
  size_t n = name.rfind("::");
  // Find the namespace
  if (n != std::string::npos) {
    name = name.substr(0, n);
    size_t n = name.rfind("::");
    if (n == std::string::npos)
      n = 0;
    else
      n = n + 2;
    thisNamespace = name.substr(n, name.size() - n);
  }
  std::string include =
      "MantidQt" + thisNamespace + "/" + this->getShortName() + ".h";
  return QString::fromStdString(include);
}

/**
 * Returns the XML used to define the widget in the designer
 * @returns A string containing the XML for the widget
 */
QString DesignerPlugin::domXml() const {
  // Default name of a widget = class name, with first letter lower case.
  std::string name = this->getShortName();
  name[0] = static_cast<char>(tolower(static_cast<int>(name[0])));

  return QString::fromStdString(
      "<widget class=\"" + this->name().toStdString() + "\" name=\"" + name +
      "\">\n"
      "</widget>\n");
}
