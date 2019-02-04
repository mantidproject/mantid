// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef DesignerPlugin_H
#define DesignerPlugin_H

#include <QDesignerCustomWidgetInterface>

/**
The DesignerPlugin creates a Qt designer plugin of the AlgorithmSelectorWidget.

@author Martyn Gigg, Tessella plc
@date 03/08/2009
*/
class DesignerPlugin : public QObject, public QDesignerCustomWidgetInterface {
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetInterface)

public:
  // ==== Methods you must overridde ==========

  /// Returns a pointer to a newly constructed widget for this plugin wraps
  QWidget *createWidget(QWidget *parent) override = 0;
  /// Returns the fully-qualified class name
  QString name() const override = 0;

  // ==== Optionally overridden methods ==========

  /// Returns a tool tip for the widget
  QString toolTip() const override;
  /// Returns the include file that appears at the top of the generated .h file
  QString includeFile() const override;
  /// Returns the XML that defines the widget and its properties
  QString domXml() const override;

  /// Default constructor
  DesignerPlugin(QObject *parent = nullptr);
  /// Initialize the plugin
  void initialize(QDesignerFormEditorInterface *core) override;
  /// Returns if the plugin is initliaized
  bool isInitialized() const override;
  /// Returns if this plugins is able to contain other widgets
  bool isContainer() const override;
  /// Returns the group name within the designer
  QString group() const override;
  /// Returns the icon to use
  QIcon icon() const override;
  /// Returns a short description of the widget
  QString whatsThis() const override;

private:
  std::string getShortName() const;

  /// Are we initialized?
  bool m_initialized;
};

#endif
