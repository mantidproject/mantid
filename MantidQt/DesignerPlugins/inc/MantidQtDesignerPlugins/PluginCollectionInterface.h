#ifndef PLUGINCOLLECTIONINTERFACE_H_
#define PLUGINCOLLECTIONINTERFACE_H_

#include <QtDesigner>
#include <QtPlugin>
#include "MantidQtMantidWidgets/AlgorithmSelectorWidget.h"
#include "MantidQtMantidWidgets/DataSelector.h"
#include "MantidQtDesignerPlugins/DesignerPlugin.h"
#include "MantidQtMantidWidgets/ScriptEditor.h"
#include "MantidQtMantidWidgets/MWRunFiles.h"
#include "MantidQtMantidWidgets/FitPropertyBrowser.h"
#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"
#include "MantidQtMantidWidgets/InstrumentSelector.h"
#include "MantidQtMantidWidgets/WorkspaceSelector.h"
#include "MantidQtSliceViewer/ColorBarWidget.h"
#include "MantidQtSliceViewer/SliceViewer.h"
#include "MantidQtSliceViewer/LineViewer.h"
#include "MantidQtMantidWidgets/SafeQwtPlot.h"
#include "MantidQtAPI/AlgorithmPropertiesWidget.h"
#include "MantidQtMantidWidgets/ProcessingAlgoWidget.h"
#include "MantidQtMantidWidgets/MessageDisplay.h"

/** 
The PluginCollectionInterface implements the interface for the plugin library and holds a 
list of plugins defined by the library.

@author Martyn Gigg, Tessella plc
@date 10/08/2009

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
class PluginCollectionInterface : public QObject, public QDesignerCustomWidgetCollectionInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

public:
  /// Default constructor
  PluginCollectionInterface(QObject *parent = 0);
  /// Returns a list of the custom widgets within this library
  virtual QList<QDesignerCustomWidgetInterface*> customWidgets() const;

private:
  QList<QDesignerCustomWidgetInterface*> m_widgets;
};



//==============================================================================
/** Macro to REALLY quickly declare a plugin for
 * a widget in MantidWidgets
 *
 * @param PluginClass :: name to give your plugin
 * @param WidgetClass :: fully-qualified name of the widget class
 * @param ToolTip :: a string with the tooltip
 */
#define DECLARE_WIDGET_PLUGIN(PluginClass, WidgetClass, ToolTip) \
class PluginClass : public DesignerPlugin { \
public: \
  PluginClass(QObject * parent) : DesignerPlugin(parent) {} \
  QWidget *createWidget(QWidget *parent) \
  { return new WidgetClass(parent); } \
  QString name() const \
  { return #WidgetClass; } \
  QString toolTip() const \
  { return ToolTip; } \
};


//==============================================================================
// Declare plugins for several types of widgets in MantidWidgets
// REMEMBER TO ADD THESE TO PluginCollectionInterface.cpp -> customWidgets
//==============================================================================
DECLARE_WIDGET_PLUGIN(AlgorithmSelectorWidgetPlugin,
    MantidQt::MantidWidgets::AlgorithmSelectorWidget,
    "Widget for picking algorithms");

DECLARE_WIDGET_PLUGIN(ScriptEditorPlugin,
    ScriptEditor,
    "Widget for editing python script");

DECLARE_WIDGET_PLUGIN(FileFinderPlugin,
    MantidQt::MantidWidgets::MWRunFiles,
    "Searches for the given files within the paths defined by\nMantid's datasearch.directories property");

DECLARE_WIDGET_PLUGIN(InstrumentSelectorPlugin,
    MantidQt::MantidWidgets::InstrumentSelector,
    "Sets the current instrument within Mantid");

DECLARE_WIDGET_PLUGIN(MuonFitBrowserPlugin,
    MantidQt::MantidWidgets::MuonFitPropertyBrowser,
    "The menu for fitting functions within Muon Analysis");

DECLARE_WIDGET_PLUGIN(FitBrowserPlugin,
    MantidQt::MantidWidgets::FitPropertyBrowser,
    "The menu for fitting functions");

DECLARE_WIDGET_PLUGIN(WorkspaceSelectorPlugin,
    MantidQt::MantidWidgets::WorkspaceSelector,
    "Select a workspace for use in this operation");

DECLARE_WIDGET_PLUGIN(ColorBarWidgetPlugin,
    MantidQt::SliceViewer::ColorBarWidget,
    "Shows a color scale and allow user to change it");

DECLARE_WIDGET_PLUGIN(SliceViewerPlugin,
    MantidQt::SliceViewer::SliceViewer,
    "Shows 2D slices of MDWorkspaces");

DECLARE_WIDGET_PLUGIN(LineViewerPlugin,
    MantidQt::SliceViewer::LineViewer,
    "Shows 1D lines selected in a LineViewer");

DECLARE_WIDGET_PLUGIN(SafeQwtPlotPlugin,
    MantidQt::MantidWidgets::SafeQwtPlot,
    "Version of QwtPlot with workspace-level thread safety");

DECLARE_WIDGET_PLUGIN(AlgorithmPropertiesWidgetPlugin,
    MantidQt::API::AlgorithmPropertiesWidget,
    "List of algorithm properties");

DECLARE_WIDGET_PLUGIN(ProcessingAlgoWidgetPlugin,
    MantidQt::MantidWidgets::ProcessingAlgoWidget,
    "Choose an algorithm or write a script as a processing step");

DECLARE_WIDGET_PLUGIN(MessageDisplayPlugin,
    MantidQt::MantidWidgets::MessageDisplay,
    "Display messages with various priorities, optionally connecting to the logging framework");

DECLARE_WIDGET_PLUGIN(DataSelectorPlugin,
    MantidQt::MantidWidgets::DataSelector,
    "Choose a file path or workspace to work with");

#endif
