#ifndef DesignerPlugin_H
#define DesignerPlugin_H

#include <QDesignerCustomWidgetInterface>

/** 
The DesignerPlugin creates a Qt designer plugin of the AlgorithmSelectorWidget.

@author Martyn Gigg, Tessella plc
@date 03/08/2009

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>    
*/
class DesignerPlugin : public QObject, public QDesignerCustomWidgetInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)

public:
  // ==== Methods you must overridde ==========

  /// Returns a pointer to a newly constructed widget for this plugin wraps
  virtual QWidget *createWidget(QWidget *parent) = 0;
  /// Returns the fully-qualified class name
  virtual QString name() const = 0;

  // ==== Optionally overridden methods ==========

  /// Returns a tool tip for the widget
  virtual QString toolTip() const;
  /// Returns the include file that appears at the top of the generated .h file
  virtual QString includeFile() const;
  /// Returns the XML that defines the widget and its properties
  virtual QString domXml() const;


  /// Default constructor
  DesignerPlugin(QObject *parent = 0);
  /// Initialize the plugin
  void initialize(QDesignerFormEditorInterface *core);
  /// Returns if the plugin is initliaized
  bool isInitialized() const;
  /// Returns if this plugins is able to contain other widgets
  bool isContainer() const;
  /// Returns the group name within the designer
  QString group() const;
  /// Returns the icon to use
  QIcon icon() const;
  /// Returns a short description of the widget
  QString whatsThis() const;

private:
  std::string getShortName() const;

  /// Are we initialized? 
  bool m_initialized;
};

#endif
