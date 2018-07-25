%ModuleCode
#include "MantidQtWidgets/Common/Message.h"
// Allows suppression of namespaces within the module
using namespace MantidQt::MantidWidgets;
%End

%InitialisationCode
qRegisterMetaType<Message>("Message");
%End

// ----------------------------------------------------------------------------
// Exceptions
// ----------------------------------------------------------------------------

%Exception std::invalid_argument(SIP_Exception) /PyName=ValueError/
{
%TypeHeaderCode
#include <stdexcept>
%End
%RaiseCode
    const char *detail = sipExceptionRef.what();

    SIP_BLOCK_THREADS
    PyErr_SetString(PyExc_ValueError, detail);
    SIP_UNBLOCK_THREADS
%End
};

// ----------------------------------------------------------------------------
// Classes
// ----------------------------------------------------------------------------

class Configurable {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Configurable.h"
%End

public:
  void readSettings(const QSettings &storage);
  void writeSettings(QSettings *storage);

private:
  // Not constructible or copyable
  Configurable();
  Configurable(const Configurable&);
};

class MessageDisplay : QWidget, Configurable {
%TypeHeaderCode
#include "MantidQtWidgets/Common/MessageDisplay.h"
%End

public:
  MessageDisplay(QWidget *parent = 0);
  void attachLoggingChannel(int logLevel = -1);

  void appendFatal(const QString &text);
  void appendError(const QString &text);
  void appendWarning(const QString &text);
  void appendNotice(const QString &text);
  void appendInformation(const QString &text);
  void appendDebug(const QString &text);
};

class ScriptEditor : QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/ScriptEditor.h"
%End

public:
  enum AutoCompletionSource {
      AcsNone,
      AcsAll,
      AcsDocument,
      AcsAPIs
  };

public:
  ScriptEditor(const QString & language,
               QWidget *parent /TransferThis/ = 0) throw(std::invalid_argument);

  QString fileName() const;
  SIP_PYTUPLE getSelection() const;
%MethodCode
  int lineFrom(-1), indexFrom(-1), lineTo(-1), indexTo(-1);
  sipCpp->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
  sipRes = sipBuildResult(nullptr, "(iiii)", lineFrom, indexFrom,
                          lineTo, indexTo);
%End
  bool hasSelectedText() const;
  bool isModified() const;
  bool isReadOnly() const;
  QString selectedText() const;
  QString text() const;

  void enableAutoCompletion(AutoCompletionSource source);
  void disableAutoCompletion();
  void setCaretLineBackgroundColor (const QColor & col);
  void setCaretLineVisible(bool enable);
  void setIndentationsUseTabs(bool tabs);
  void setFileName(const QString &filename);
  void setMarginWidth(int margin, int width);
  void setModified(bool m);
  void setReadOnly(bool ro);
  void setSelection(int lineFrom, int indexFrom, int lineTo, int indexTo);
  void setTabWidth(int width);
  void setText(const QString &text);
  void updateCompletionAPI(const QStringList & completions);

public slots:
  void updateProgressMarker(int lineno, bool error);

signals:
  void fileNameChanged(const QString &fileName);
  void modificationChanged(bool m);

private:
  ScriptEditor(const ScriptEditor&);
};


class AlgorithmDialog: QDialog {
%TypeHeaderCode
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidAPI/AlgorithmObserver.h"
#include <boost/python/extract.hpp>
using namespace MantidQt::API;
%End
public:
  void setShowKeepOpen(const bool showOption);
  bool isShowKeepOpen() const;
  void initializeLayout();
  bool isInitialized() const;
  void setAlgorithm(SIP_PYOBJECT);
%MethodCode
  sipCpp->setAlgorithm(boost::python::extract<Mantid::API::IAlgorithm_sptr>(a0));
%End
  void setPresetValues(const QHash<QString, QString> &presetValues);
  void isForScript(bool forScript);
  void executeOnAccept(bool on);
  void setOptionalMessage(const QString &message);
  void addEnabledAndDisableLists(const QStringList &enabled,
                                 const QStringList &disabled);
  void addAlgorithmObserver(SIP_PYOBJECT);
%MethodCode
  sipCpp->addAlgorithmObserver(boost::python::extract<Mantid::API::AlgorithmObserver*>(a0));
%End
private:
  AlgorithmDialog();
  AlgorithmDialog(const AlgorithmDialog&);
};


class GenericDialog: AlgorithmDialog {
%TypeHeaderCode
#include "MantidQtWidgets/Common/GenericDialog.h"
using namespace MantidQt::API;
%End
public:
  GenericDialog(QWidget *parent = nullptr);
};


class InterfaceManager {
%TypeHeaderCode
#include "MantidQtWidgets/Common/InterfaceManager.h"
using namespace MantidQt::API;
%End
public:
  InterfaceManager();
  AlgorithmDialog *createDialogFromName(
      const QString &algorithmName, const int version = -1,
      QWidget *parent = nullptr, bool forScript = false,
      const QHash<QString, QString> &presetValues = QHash<QString, QString>(),
      const QString &optionalMsg = QString(),
      const QStringList &enabled = QStringList(),
      const QStringList &disabled = QStringList());
};

// ---------------------------------
// WorkspaceWidget and dependencies
// ---------------------------------

class MantidDisplayBase /Abstract/ {
%TypeHeaderCode
#include "MantidQtWidgets/Common/MantidDisplayBase.h"
%End
private:
  MantidDisplayBase();
};

class MantidTreeModel : MantidDisplayBase {
%TypeHeaderCode
#include "MantidQtWidgets/Common/MantidTreeModel.h"
%End
public:
  MantidTreeModel();
private:
  MantidTreeModel(const MantidTreeModel &);
};

class WorkspaceTreeWidget : QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidget.h"
%End
public:
  WorkspaceTreeWidget(MantidDisplayBase *mdb /Transfer/,
                      QWidget *parent /TransferThis/ = nullptr);
};

// Implementation to override context menu
class WorkspaceTreeWidgetSimple : WorkspaceTreeWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidgetSimple.h"
%End
public:
  WorkspaceTreeWidgetSimple(QWidget *parent /TransferThis/ = nullptr);

signals:
  void plotSpectrumClicked(const QStringList & workspaceNames);
  void overplotSpectrumClicked(const QStringList & workspaceNames);
  void plotSpectrumWithErrorsClicked(const QStringList & workspaceNames);
  void overplotSpectrumWithErrorsClicked(const QStringList & workspaceNames);
  void plotColorfillClicked(const QStringList &workspaceNames);
};

// ---------------------------------
// Manage User Directories
// ---------------------------------

class ManageUserDirectories : QDialog {
%TypeHeaderCode
#include "MantidQtWidgets/Common/ManageUserDirectories.h"
%End
public:
  ManageUserDirectories(QWidget *parent /TransferThis/ = nullptr);
};

// ---------------------------------
// Instrument View
// ---------------------------------

namespace GraphOptions
{
%TypeHeaderCode
#include "MantidQtWidgets/Common/GraphOptions.h"
%End

  enum ScaleType {Linear, Log10};

  enum Axis{Left, Right, Bottom, Top};

  enum CurveType {
  Line,
  Scatter,
  LineSymbols,
  VerticalBars,
  Area,
  Pie,
  VerticalDropLines,
  Spline,
  HorizontalSteps,
  Histogram,
  HorizontalBars,
  VectXYXY,
  ErrorBars,
  Box,
  VectXYAM,
  VerticalSteps,
  ColorMap,
  GrayScale,
  ColorMapContour,
  Contour,
  Function,
  ImagePlot,
  User
};

};

class InstrumentWidgetTab: QFrame
{
%TypeHeaderCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetTab.h"
%End
public:
	// This is a duplicate from the same enum in InstrumentWidget, just so you
	// can do like InstrumentWidgetRenderTab.FULL3D
	enum SurfaceType { FULL3D, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z,
                     SPHERICAL_X, SPHERICAL_Y, SPHERICAL_Z,
                     RENDERMODE_SIZE };
private:
  InstrumentWidgetTab(const InstrumentWidgetTab &);
};

class InstrumentWidget: QWidget
{
%TypeHeaderCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
%End

%Docstring
    The InstrumentWidget contains functionality for exploring
    a 3D model of an instrument. It is a tabbed widget offering
    rendering, picking, masking and grouping functions. There is
    also a tree view of the instrument hierarchy.

    For further help see the functions defined on each of the following
        - InstrumentWidgetRenderTab
        - InstrumentWidgetPickTab
        - InstrumentWidgetMaskTab
%End

public:
  // Note this enum is also defined in InstrumentWidgetTab
  enum SurfaceType { FULL3D, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z,
                     SPHERICAL_X, SPHERICAL_Y, SPHERICAL_Z,
                     RENDERMODE_SIZE };
  enum Tab { RENDER, PICK, MASK, TREE };

  InstrumentWidget(const QString &wsName, QWidget *parent = nullptr,
				bool resetGeometry = true, bool autoscaling = true,
				double scaleMin = 0.0, double scaleMax = 0.0,
				bool setDefaultView = true);

  InstrumentWidgetTab * getTab(const QString & title) const;
%Docstring
    Returns a handler to the requested tab

    Args:

        title The full title of a tab in the window

    Returns:
        a pointer to the requested tab widget

%End

  InstrumentWidgetTab * getTab(const Tab tab) const;
%Docstring
    Returns a handler to the requested tab

    Args:
        tab One of the Tab enumeration types:
	    InstrumentWidget.RENDER,InstrumentWidget.PICK,
	    InstrumentWidget.MASK,InstrumentWidget.TREE

    Returns:
        a pointer to the requested tab widget

%End

  void setBinRange(double min_value, double max_value);
%Docstring
    Updates the integration range over which the colours
    are calculated

    Args:
        min_value The minimum value over which the data is integrated
        max_value The maximum value over which the data is integrated

%End

  bool overlay(const QString & wsName);
%Docstring
    Overlays a workspace onto an unwrapped surface on the instrument view.
    Raises a dialog box if the current surface is not unwrapped.

    Args:
        wsName The name of the workspace

    Returns:
        A boolean indicating if the overlay was successful or not

%End

  // -- Deprecated in favour of specific tab functions --
  // Deprecation is done in mantidplot.py module rather than using
  // sip /Deprecated/annotation so that the message can be customised
  void changeColormap();
  void changeColormap(const QString & file);
  void setColorMapMinValue(double);
  void setColorMapMaxValue(double);
  void setColorMapRange(double, double);
  void selectComponent(const QString &);
  void setScaleType(GraphOptions::ScaleType);
  void setViewType(const QString &);

private:
InstrumentWidget(const InstrumentWidget &);
};

class InstrumentWidgetRenderTab: InstrumentWidgetTab
{
%TypeHeaderCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h"
%End

public:

  GraphOptions::ScaleType getScaleType() const;
%Docstring
    Returns the current scale type. 0=Linear, 1=Log10
%End

  void setScaleType(GraphOptions::ScaleType type);
%Docstring
    Set the scale type for the colour bar.

    Args:
        type The new type Options are: GraphOptions.Linear, GraphOptions.Log10
%End

  void setAxis(const QString& axisName);
%Docstring
    Set the current viewing axis by name.

    Args:
       axisName The name of a view in a string: X+,X-,Y+,Y-,Z+,Z-
%End

  void setMinValue(double value, bool apply = true);
%Docstring
    Set the minimum value for the colour scale

    Args:
       value The new value for the minimum
%End

  void setMaxValue(double value, bool apply = true);
%Docstring
    Set the maximum value for the colour scale

    Args:
       value The new value for the maximum
%End

  void setRange(double minValue, double maxValue, bool apply = true);
%Docstring
    Set the range of the colour bar

    Args:
        minValue The new value for the minimum
        maxValue The new value for the maximum
%End

  void showAxes(bool on);
%Docstring
    Set the axes on/off

    Args:
        on True/False indicating whether the axes are visible
%End

  void setColorMapAutoscaling(bool on);
%Docstring
    Set whether the colour scale should auto scale when the data is updated

    Args:
        on True/False indicating whether auto scaling is active
%End

  void displayDetectorsOnly(bool yes);
%Docstring
    Set whether only detectors should be shown.

    Args:
        yes True/False indicating whether only detectors are shown
%End

  void enableGL(bool on);
%Docstring
    Toggle the use of OpenGL. This can only be used for the unwrapped views.

    Args:
        on True/False indicating whether OpenGL should be used.
%End

  void changeColorMap(const QString & filename = "");
%Docstring
    Change the color map for the given one. If blank then a dialog is raised

    Args:
        filename A filename giving the full path to a color map file
%End

  void setSurfaceType(int type);
%Docstring
    Set the surface type of the current window.

    Args:
        type A known suface type: FULL3D, CYLINDRICAL_X, CYLINDRICAL_Y,
                                  CYLINDRICAL_Z,SPHERICAL_X, SPHERICAL_Y,
                                  SPHERICAL_Z
%End

  void flipUnwrappedView(bool on);
%Docstring
    Set whether an unwrapped view is flipped

    Args:
        on True/False indicatingif to flip the view
%End

  void saveImage(QString filename = "");
%Docstring
    Save the current display buffer to an image file. The format is chosen by the filename extension and must
    be one of .bmp,.jpeg,.jpeg,.png,.ppm,.tiff,.xbm,.xpm

    Args:
        filename The filename for the saved image. An empty string raises a dialog box.
%End

private:
  InstrumentWidgetRenderTab();
  InstrumentWidgetRenderTab(const InstrumentWidgetRenderTab &);
};

class InstrumentWidgetPickTab: InstrumentWidgetTab
{
%TypeHeaderCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
%End

public:
  enum ToolType {Zoom, PixelSelect, TubeSelect, PeakSelect, PeakErase};

  void selectTool(const ToolType tool);
%Docstring
    Select the active tool.

    Args:
        tool One of InstrumentWidgetPickTab.{Zoom, PixelSelect, TubeSelect, PeakSelect, PeakErase}
%End

private:
  InstrumentWidgetPickTab();
  InstrumentWidgetPickTab(const InstrumentWidgetPickTab &);
};

class InstrumentWidgetMaskTab: InstrumentWidgetTab
{
%TypeHeaderCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h"
%End

public:
  enum Mode {Mask, Group};
  enum Activity {Move,Select,DrawEllipse,DrawRectangle,DrawEllipticalRing,DrawRectangularRing};

  void setMode(Mode mode);
%Docstring
  Set the mode to either group/mask

  Args:
      mode One of the Mode enumerations InstrumentWidgetMaskTab.{Mask, Group}
%End

  void selectTool(Activity tool);
%Docstring
  Set the mode to either group/mask

  Args:
      tool One of the Activity enumerations InstrumentWidgetMaskTab.{Move,Select,DrawEllipse,DrawRectangle,DrawEllipticalRing,DrawRectangularRing}
%End

private:
  InstrumentWidgetMaskTab();
  InstrumentWidgetMaskTab(const InstrumentWidgetMaskTab &);
};

class InstrumentWidgetTreeTab: InstrumentWidgetTab
{
%TypeHeaderCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h"
%End

public:
  void selectComponentByName(const QString& name);
%Docstring
    Sets the tree tab to center the view on the named component

    Args:
        name The name of a component in the instrument
%End

private:
  InstrumentWidgetTreeTab();
  InstrumentWidgetTreeTab(const InstrumentWidgetTreeTab &);
};
