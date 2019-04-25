// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PLUGINCOLLECTIONINTERFACE_H_
#define PLUGINCOLLECTIONINTERFACE_H_

#include "MantidQtWidgets/Common/AlgorithmPropertiesWidget.h"
#include "MantidQtWidgets/Common/AlgorithmSelectorWidget.h"
#include "MantidQtWidgets/Common/DataSelector.h"
#include "MantidQtWidgets/Common/FitPropertyBrowser.h"
#include "MantidQtWidgets/Common/InstrumentSelector.h"
#include "MantidQtWidgets/Common/LogValueSelector.h"
#include "MantidQtWidgets/Common/MWRunFiles.h"
#include "MantidQtWidgets/Common/MessageDisplay.h"
#include "MantidQtWidgets/Common/MuonFitDataSelector.h"
#include "MantidQtWidgets/Common/MuonFitPropertyBrowser.h"
#include "MantidQtWidgets/Common/ProcessingAlgoWidget.h"

#include "MantidQtWidgets/Common/ScriptEditor.h"
#include "MantidQtWidgets/Common/WorkspaceSelector.h"
#include "MantidQtWidgets/Plotting/Qwt/ColorBarWidget.h"
#include "MantidQtWidgets/Plotting/Qwt/DisplayCurveFit.h"
#include "MantidQtWidgets/Plotting/Qwt/MWView.h"
#include "MantidQtWidgets/Plotting/Qwt/PreviewPlot.h"
#include "MantidQtWidgets/Plotting/Qwt/SafeQwtPlot.h"
#include "MantidQtWidgets/Plugins/Designer/DesignerPlugin.h"
#include "MantidQtWidgets/SliceViewer/LineViewer.h"
#include "MantidQtWidgets/SliceViewer/SliceViewer.h"
#include <QtDesigner>
#include <QtPlugin>

/**
The PluginCollectionInterface implements the interface for the plugin library
and holds a
list of plugins defined by the library.

@author Martyn Gigg, Tessella plc
@date 10/08/2009
*/
class PluginCollectionInterface
    : public QObject,
      public QDesignerCustomWidgetCollectionInterface {
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

public:
  /// Default constructor
  PluginCollectionInterface(QObject *parent = nullptr);
  /// Returns a list of the custom widgets within this library
  virtual QList<QDesignerCustomWidgetInterface *>
  customWidgets() const override;

private:
  QList<QDesignerCustomWidgetInterface *> m_widgets;
};

//==============================================================================
/** Macro to REALLY quickly declare a plugin for
 * a widget in MantidWidgets
 *
 * @param PluginClass :: name to give your plugin
 * @param WidgetClass :: fully-qualified name of the widget class
 * @param ToolTip :: a string with the tooltip
 */
#define DECLARE_WIDGET_PLUGIN(PluginClass, WidgetClass, ToolTip)               \
  class PluginClass : public DesignerPlugin {                                  \
  public:                                                                      \
    PluginClass(QObject *parent) : DesignerPlugin(parent) {}                   \
    QWidget *createWidget(QWidget *parent) override {                          \
      return new WidgetClass(parent);                                          \
    }                                                                          \
    QString name() const override { return #WidgetClass; }                     \
    QString toolTip() const override { return ToolTip; }                       \
  }

//==============================================================================
// Declare plugins for several types of widgets in MantidWidgets
// REMEMBER TO ADD THESE TO PluginCollectionInterface.cpp -> customWidgets
//==============================================================================
DECLARE_WIDGET_PLUGIN(AlgorithmSelectorWidgetPlugin,
                      MantidQt::MantidWidgets::AlgorithmSelectorWidget,
                      "Widget for picking algorithms");

DECLARE_WIDGET_PLUGIN(ScriptEditorPlugin, ScriptEditor,
                      "Widget for editing python script");

DECLARE_WIDGET_PLUGIN(FileFinderPlugin, MantidQt::API::MWRunFiles,
                      "Searches for the given files within the paths defined "
                      "by\nMantid's datasearch.directories property");

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
                      MantidQt::MantidWidgets::ColorBarWidget,
                      "Shows a color scale and allow user to change it");

DECLARE_WIDGET_PLUGIN(SliceViewerPlugin, MantidQt::SliceViewer::SliceViewer,
                      "Shows 2D slices of MDWorkspaces");

DECLARE_WIDGET_PLUGIN(LineViewerPlugin, MantidQt::SliceViewer::LineViewer,
                      "Shows 1D lines selected in a LineViewer");

DECLARE_WIDGET_PLUGIN(SafeQwtPlotPlugin, MantidQt::MantidWidgets::SafeQwtPlot,
                      "Version of QwtPlot with workspace-level thread safety");

DECLARE_WIDGET_PLUGIN(MWViewPlugin, MantidQt::MantidWidgets::MWView,
                      "2D view of a MatrixWorkspace");

DECLARE_WIDGET_PLUGIN(AlgorithmPropertiesWidgetPlugin,
                      MantidQt::API::AlgorithmPropertiesWidget,
                      "List of algorithm properties");

DECLARE_WIDGET_PLUGIN(
    ProcessingAlgoWidgetPlugin, MantidQt::MantidWidgets::ProcessingAlgoWidget,
    "Choose an algorithm or write a script as a processing step");

DECLARE_WIDGET_PLUGIN(MessageDisplayPlugin,
                      MantidQt::MantidWidgets::MessageDisplay,
                      "Display messages with various priorities, optionally "
                      "connecting to the logging framework");

DECLARE_WIDGET_PLUGIN(DataSelectorPlugin, MantidQt::MantidWidgets::DataSelector,
                      "Choose a file path or workspace to work with");

DECLARE_WIDGET_PLUGIN(PreviewPlotPlugin, MantidQt::MantidWidgets::PreviewPlot,
                      "Curve plots for workspace spectra");

DECLARE_WIDGET_PLUGIN(DisplayCurveFitPlugin,
                      MantidQt::MantidWidgets::DisplayCurveFit,
                      "Curve plots for workspace spectra");

DECLARE_WIDGET_PLUGIN(MuonFitDataSelectorPlugin,
                      MantidQt::MantidWidgets::MuonFitDataSelector,
                      "Data selection for muon fits");

DECLARE_WIDGET_PLUGIN(LogValueSelectorPlugin,
                      MantidQt::MantidWidgets::LogValueSelector,
                      "Select a log name and function");

#endif
