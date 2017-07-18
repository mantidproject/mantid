/***************************************************************************/
/**
 This file is a SIP file for binding C++ methods of the
 MantidQt widgets (SliceViewer in particular)
 to Python.

*/
/***************************************************************************/

// Define the module name. This has to match the library filename
%Module mantidqtpython

%UnitCode
  #if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
    #pragma GCC system_header
  #endif
%End

/******************************** SIP Imports ****************/
%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Include qwttypes.sip

%ModuleHeaderCode
#include "MantidQtAPI/PythonSystemHeader.h"
#include "MantidQtAPI/WorkspaceObserver.h"
%End

%InitialisationCode
qRegisterMetaType<Mantid::API::Workspace_sptr>();
qRegisterMetaType<std::string>();
%End
/***************************************************************************/
/**************** Exceptions ***********************************************/
/***************************************************************************/
%Exception std::exception(SIP_Exception) /PyName=StdException/
{
%TypeHeaderCode
#include <stdexcept>
%End
%RaiseCode
    const char *detail = sipExceptionRef.what();

    SIP_BLOCK_THREADS
    PyErr_SetString(sipException_std_exception, detail);
    SIP_UNBLOCK_THREADS
%End
};

%Exception std::invalid_argument(SIP_Exception) /PyName=StdInvalidArgument/
{
%TypeHeaderCode
#include <stdexcept>
%End
%RaiseCode
    const char *detail = sipExceptionRef.what();

    SIP_BLOCK_THREADS
    PyErr_SetString(sipException_std_invalid_argument, detail);
    SIP_UNBLOCK_THREADS
%End
};

%Exception std::runtime_error(SIP_Exception) /PyName=StdRuntimeError/
{
%TypeHeaderCode
#include <stdexcept>
%End
%RaiseCode
    const char *detail = sipExceptionRef.what();

    SIP_BLOCK_THREADS
    PyErr_SetString(sipException_std_runtime_error, detail);
    SIP_UNBLOCK_THREADS
%End
};

namespace GraphOptions
{
%TypeHeaderCode
#include "MantidQtAPI/GraphOptions.h"
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

namespace Mantid
{
namespace API
{

enum MDNormalization
{
//%TypeHeaderCode
//#include "../Framework/API/inc/MantidAPI/IMDWorkspace.h"
//%End
  NoNormalization,
  VolumeNormalization,
  NumEventsNormalization
};

}; // end namespace
}; // end namespace


namespace MantidQt
{
namespace API
{

class AlgorithmDialog : QDialog
{
%TypeHeaderCode
#include "MantidQtAPI/AlgorithmDialog.h"
%End
private:
  AlgorithmDialog();
};

class UserSubWindow : QMainWindow
{
%TypeHeaderCode
#include "MantidQtAPI/UserSubWindow.h"
%End
private:
  UserSubWindow();
};

class InterfaceManager
{
%TypeHeaderCode
#include "MantidQtAPI/InterfaceManager.h"
%End
public:
  MantidQt::API::AlgorithmDialog* createDialogFromName(const QString&, const int = -1,
                                                       QWidget* = 0, bool = false);
  MantidQt::API::UserSubWindow* createSubWindow(const QString& interface_name, QWidget* parent = 0);

  void showHelpPage(const QString & url=QString());

  void showWikiPage(const QString &page=QString());

  void showAlgorithmHelp(const QString &name, const int version=-1);

  void showConceptHelp(const QString &name);

  void showFitFunctionHelp(const QString &name=QString());

  void showCustomInterfaceHelp(const QString &name);
};

class MantidDesktopServices
{
%TypeHeaderCode
#include "MantidQtAPI/MantidDesktopServices.h"
%End
public:
  static bool openUrl(const QUrl &url);
  static void setUrlHandler(const QString &scheme, QObject *receiver,
                            const char *method);
  static void unsetUrlHandler(const QString &scheme);
  static QString storageLocation(QDesktopServices::StandardLocation type);
  static QString displayName(QDesktopServices::StandardLocation type);
};

};

namespace API
{

class MWRunFiles : QWidget
{
%TypeHeaderCode
#include "MantidQtAPI/MWRunFiles.h"
%End
public:
void setLabelMinWidth(const int);
void liveButtonSetChecked(bool);
bool liveButtonIsChecked();
QString getText() /PyName=text/;
bool isValid();
void setUserInput(const QVariant &);
void setText(const QString&);
signals:
void fileFindingFinished();
void liveButtonPressed(bool);
};

};

namespace SliceViewer
{

//*WIKI* == Classes ==
//*WIKI* Here follows a list of the classes exposed to python and the methods you can execute on them.


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class SliceViewerWindow : QMainWindow
{
%TypeHeaderCode
#include "MantidQtSliceViewer/SliceViewerWindow.h"
%End
%Docstring

SliceViewerWindow
=================

    The SliceViewerWindow is a window containing a SliceViewer widget
    and a LineViewer Widget.

    It allows you to look at 2D slices in a workspace such as a
    MDEventWorkspace or a MDHistoWorkspace, and then perform 1D line
    sections.

    You can access the two contained widgets using:
        getSlicer() (for the SliceViewer)
        getLiner() (for the LineViewer)

    However, the methods of the SliceViewer are exposed to the
    SliceViewerWindow, so you do not need to get the SliceViewer
    widget directly; you can call the methods on the SliceViewerWindow
    directly. For example:

       svw.setSlicePoint(2, 10.5)

    See the methods for SliceViewer, below.

%End

public:
  SliceViewerWindow(const QString& wsName, const QString& label, Qt::WFlags f);
  MantidQt::SliceViewer::SliceViewer* getSlicer();
%Docstring
MantidQt::SliceViewer::SliceViewer* SliceViewerWindow::getSlicer()
------------------------------------------------------------------
    Get the SliceViewer widget inside the SliceViewerWindow.
    This is the main widget for controlling the 2D views
    and slice points.

    Returns:
        a pointer to the SliceViewer widget.

%End

  MantidQt::SliceViewer::LineViewer* getLiner();
%Docstring
MantidQt::SliceViewer::LineViewer* SliceViewerWindow::getLiner()
----------------------------------------------------------------
    Get the LineViewer widget inside the SliceViewerWindow.
    This is the widget for controlling the 1D line integration
    settings.

    Returns:
        a pointer to the LineViewer widget.

%End

  const QString& getLabel() const;

};

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class LineViewer : QWidget
{
%TypeHeaderCode
#include "MantidQtSliceViewer/LineViewer.h"
%End

%Docstring

LineViewer
==========

    The LineViewer is a widget to select 1D line sections out of
    a workspace such as the MDEventWorkspace or a MDHistoWorkspace.

    It is connected to the SliceViewer.

%End

public:

void apply()   throw (std::runtime_error);
%Docstring
void LineViewer::apply()
------------------------
    Perform the 1D integration using the current parameters.

    Raises:
        std::runtime_error if an error occurs.

%End

void showPreview();
%Docstring
void LineViewer::showPreview()
------------------------------
    Calculate and show the preview (non-integrated) line,
    using the current parameters.
%End

void showFull();
%Docstring
void LineViewer::showFull()
---------------------------
    Calculate and show the full (integrated) line, using the latest
    integrated workspace. The apply() method must have been called
    before calling this.
%End


//*WIKI* ==== Start/End Points ====
void setStartXY(double x, double y)   throw (std::runtime_error);
%Docstring
void LineViewer::setStartXY(double x, double y)
-----------------------------------------------
    Set the start point of the line to integrate

    Args:
        x :: position of the start in the "X" dimension
           (as shown in the SliceViewer).
        y :: position of the start in the "Y" dimension
           (as shown in the SliceViewer).

%End

void setEndXY(double x, double y)   throw (std::runtime_error);
%Docstring
void LineViewer::setEndXY(double x, double y)
---------------------------------------------
    Set the start point of the line to integrate

    Args:
        x :: position of the start in the "X" dimension
           (as shown in the SliceViewer).
        y :: position of the start in the "Y" dimension
           (as shown in the SliceViewer).

%End


//*WIKI* ==== Width ====
void setThickness(double width);
%Docstring
void LineViewer::setThickness(double width)
-------------------------------------------
    Set the thickness to integrate to be the same in all dimensions

    This sets the planar width and all the other dimensions' thicknesses
    to the same value.

    Args:
        width :: width of integration, in the units of all dimensions

%End

void setThickness(int dim, double width)   throw (std::invalid_argument, std::runtime_error);
%Docstring
void LineViewer::setThickness(int dim, double width)
----------------------------------------------------
    Set the thickness to integrate in a particular dimension.

    Integration is performed perpendicular to the XY plane,
    from -0.5 * thickness below to +0.5 * thickness above the center.

    Use setPlanarWidth() to set the width along the XY plane.

    Args:
        dim :: index of the dimension to change
        width :: width of integration, in the units of the dimension.

    Raises:
        std::invalid_argument if the index is invalid

%End

void setThickness(const QString & dim, double width)   throw (std::invalid_argument, std::runtime_error);
%Docstring
void LineViewer::setThickness(const QString & dim, double width)
----------------------------------------------------------------
    Set the thickness to integrate in a particular dimension.

    Integration is performed perpendicular to the XY plane,
    from -0.5 * thickness below to +0.5 * thickness above the center.

    Use setPlanarWidth() to set the width along the XY plane.

    Args:
        dim :: name of the dimension to change
        width :: thickness of integration, in the units of the dimension.

    Raises:
        std::runtime_error if the name is not found in the workspace

%End

void setPlanarWidth(double width);
%Docstring
void LineViewer::setPlanarWidth(double width)
---------------------------------------------
    Set the width of the line in the planar dimension only.
    Other dimensions' widths will follow unless they were manually changed

    Args:
        width :: width in the plane.
%End

double getPlanarWidth() const;
%Docstring
double LineViewer::getPlanarWidth()
-----------------------------------

    Returns:
        the width in the plane, or the width in dimension 0 if not restricted to a plane
%End


//*WIKI* ==== Binning ====
void setNumBins(int numBins)   throw (std::invalid_argument);
%Docstring
void LineViewer::setNumBins(int numBins)
----------------------------------------
    Set the number of bins in the line.

    Args:
        numBins :: # of bins

    Raises:
        std::invalid_argument if numBins < 1

%End

void setFixedBinWidthMode(bool fixedWidth, double binWidth)   throw (std::invalid_argument);
%Docstring
void LineViewer::setFixedBinWidthMode(bool fixedWidth, double binWidth)
-----------------------------------------------------------------------
    Sets the fixed bin width mode on or off.

    In fixed bin width mode, the width of each bin along the line length
    is constant, and the number of bins is adjusted to as the line
    gets longer.
    If off, then you use a fixed number of bins, and the bin width is
    then simply: width = length / number_of_bins.

    Args:
        fixedWidth :: if True, then keep the bin width fixed.
        binWidth :: for fixed bin width mode, this specified the desired
           bin width. Must be > 0. Ignored for non-fixed-bin-width mode.

    Raises:
        std::invalid_argument if binWidth <= 0

%End

double getFixedBinWidth() const;
%Docstring
double LineViewer::getFixedBinWidth()
-------------------------------------
    For fixed-bin-width mode, get the desired fixed bin width.

    Returns:
        the desired fixed bin width

%End

bool getFixedBinWidthMode() const;
%Docstring
bool LineViewer::getFixedBinWidthMode()
---------------------------------------
    Is the LineViewer in fixed-bin-width mode?

    Returns:
        True if in fixed bin width mode.

%End

int getNumBins() const;
%Docstring
int LineViewer::getNumBins()
----------------------------
    Get the number of bins

    Returns:
        the number of bins in the line to integrate (int)

%End

double getBinWidth() const;
%Docstring
double LineViewer::getBinWidth()
--------------------------------
    Get the width of each bin

    Returns:
        the width of each bin (double)

%End

int getXAxisDimensionIndex() const;
%Docstring
int LineViewer::getXAxisDimensionIndex()
--------------------------------
    Get the index of the dimension used for the x axis

    Returns:
        the index of the dimension used for the x axis (int)

%End


////*WIKI* ==== Plotting ====
void setPlotAxis(int choice);
%Docstring
void LineViewer::setPlotAxis(int choice)
----------------------------------------
    Choose which coordinates to use as the X axis to plot in the line view.

    Args:
        choice :: PlotAxisChoice, either Auto, X, Y or Distance.

%End

int getPlotAxis() const;
%Docstring
int LineViewer::getPlotAxis()
-----------------------------
    Return which coordinates to use as the X axis to plot in the line view.

    Returns:
        PlotAxisChoice, either Auto, X, Y or Distance.

%End


};

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class PeaksPresenter : QObject
{
%TypeHeaderCode
#include "MantidQtSliceViewer/PeaksPresenter.h"
%End

%Docstring

PeaksPresenter
===========
    This is a controlling unit for PeaksWorkspace displayed in the PeaksOverlay of the SliceViewer.
%End

private:
    PeaksPresenter();
    PeaksPresenter(const MantidQt::SliceViewer::PeaksPresenter&);
public:
%Docstring
void PeaksPresenter::setForegroundColor(const QColor color)
------------------------------------------------------
    Sets the Foreground color of the peaks workspace as displayed in the peaks overlay.

    Args:
        color :: color to change foreground to
%End
    virtual void setForegroundColor(const QColor) = 0;
%Docstring
void PeaksPresenter::setBackgroundColor(const QColor color)
------------------------------------------------------
    Sets the Background color of the peaks workspace as displayed in the peaks overlay.

    Args:
        color :: color to change background to
%End
    virtual void setBackgroundColor(const QColor) = 0;
%Docstring
void PeaksPresenter::showBackgroundRadius(const bool shown)
------------------------------------------------------
    Toggle the background radius on and off.

    Args:
        shown :: True to show the background radius, otherwise false.
%End
    virtual void showBackgroundRadius(const bool shown) = 0;
%Docstring
void PeaksPresenter::setShown(const bool shown)
------------------------------------------------------
    Toggle the overplotted state of this PeaksWorkspace in the PeaksOverlay

    Args:
        shown :: True to show. False to hide.
%End
    virtual void setShown(const bool shown) = 0;
%Docstring
void PeaksPresenter::zoomToPeak(const int index)
------------------------------------------------------
    Zoom in on a peak of the PeaksWorkspace. Provide an index (0 based) to go to the peak.

    Args:
        index :: Index into the peaks workspace (0 based) to zoom to.
%End
    virtual void zoomToPeak(const int index) = 0;
};

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class ProxyCompositePeaksPresenter :  QObject
{
%TypeHeaderCode
#include "MantidQtSliceViewer/ProxyCompositePeaksPresenter.h"
#include "MantidQtSliceViewer/PeaksPresenter.h"
%End
private:
    ProxyCompositePeaksPresenter();
    ProxyCompositePeaksPresenter(const MantidQt::SliceViewer::ProxyCompositePeaksPresenter&);
public:
  MantidQt::SliceViewer::PeaksPresenter* getPeaksPresenter(const QString& name);
};


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class SliceViewer : QWidget
{
%TypeHeaderCode
#include "MantidQtSliceViewer/SliceViewer.h"
%End

%Docstring

SliceViewer
===========

    The SliceViewer is a widget showing a 2D slice of a multi-dimensional
    workspace such as the MDEventWorkspace or a MDHistoWorkspace.

%End

public:
  //*WIKI* ==== Basics ====
  void setWorkspace(const QString & wsName)   throw (std::runtime_error);
%Docstring
void SliceViewer::setWorkspace(const QString & wsName)
------------------------------------------------------
    Set the workspace to view using its name.
    The workspace should be a MDHistoWorkspace or a MDEventWorkspace,
    with at least 2 dimensions.

    Args:
        wsName :: name of the MDWorkspace to look for

    Raises:
        std::runtime_error if the workspace is not found or is a MatrixWorkspace

%End

  QString getWorkspaceName() const;
%Docstring
QString SliceViewer::getWorkspaceName()
---------------------------------------

    Returns:
        the name of the workspace selected, or a blank string
    if no workspace is set.

%End

  void showControls(bool visible);
%Docstring
void SliceViewer::showControls(bool visible)
--------------------------------------------
    Programmatically show/hide the controls (sliders etc)

    Args:
        visible :: true if you want to show the controls.

%End

  void openFromXML(const QString & xml)   throw (std::invalid_argument, std::runtime_error);
%Docstring
void SliceViewer::openFromXML(const QString & xml)
--------------------------------------------------
    Opens a workspace and sets the view and slice points
    given the XML from the MultiSlice view in XML format.

    Args:
        xml :: string describing workspace, slice point, etc.

    Raises:
        std::runtime_error if error in parsing XML

%End

  QPixmap getImage();
%Docstring
QPixmap SliceViewer::getImage()
-------------------------------
    Grab the 2D view as an image. The image is rendered at the current window
    size, with the color scale but without the text boxes for changing them.

    See also saveImage() and copyImageToClipboard()

    Returns:
        QPixmap containing the image.

%End

  void saveImage(const QString & filename);
%Docstring
void SliceViewer::saveImage(const QString & filename)
-----------------------------------------------------
    Save the rendered 2D slice to an image file.

    Args:
        filename :: full path to the file to save, including extension
           (e.g. .png). If not specified or empty, then a dialog will prompt
           the user to pick a file.

%End

  void copyImageToClipboard();
%Docstring
void SliceViewer::copyImageToClipboard()
----------------------------------------
    Copy the rendered 2D image to the clipboard

%End

  void setFastRender(bool fast);
%Docstring
void SliceViewer::setFastRender(bool fast)
------------------------------------------
    Sets whether the image should be rendered in "fast" mode, where
    the workspace's resolution is used to guess how many pixels to render.

    If false, each pixel on screen will be rendered. This is the most
    accurate view but the slowest.

    This redraws the screen.

    Args:
        fast :: true to use "fast" rendering mode.

%End

  bool getFastRender() const;
%Docstring
bool SliceViewer::getFastRender()
---------------------------------
    Return true if the image is in "fast" rendering mode.

    In "fast" mode, the workspace's resolution is used to guess how many
    pixels to render. If false, each pixel on screen will be rendered.
    This is the most accurate view but the slowest.

    Returns:
        True if the image is in "fast" rendering mode.

%End

  void toggleLineMode(bool lineMode);
%Docstring
void SliceViewer::toggleLineMode(bool lineMode)
-----------------------------------------------
    Toggle "line-drawing" mode (to draw 1D lines using the mouse)

    Args:
        lineMode :: True to go into line mode, False to exit it.

%End


  //*WIKI* ==== X/Y Dimension ====
  void setXYDim(int indexX, int indexY)     throw (std::invalid_argument);
%Docstring
void SliceViewer::setXYDim(int indexX, int indexY)
--------------------------------------------------
    Set the index of the dimensions that will be shown as
    the X and Y axis of the plot.
    You cannot set both axes to be the same.

    To be called from Python, primarily.

    Args:
        indexX :: index of the X dimension, from 0 to NDims-1.
        indexY :: index of the Y dimension, from 0 to NDims-1.

    Raises:
        std::invalid_argument if an index is invalid or repeated.

%End

  void setXYDim(const QString & dimX, const QString & dimY)     throw (std::invalid_argument, std::runtime_error);
%Docstring
void SliceViewer::setXYDim(const QString & dimX, const QString & dimY)
----------------------------------------------------------------------
    Set the dimensions that will be shown as the X and Y axes

    Args:
        dimX :: name of the X dimension. Must match the workspace dimension names.
        dimY :: name of the Y dimension. Must match the workspace dimension names.

    Raises:
        std::runtime_error if the dimension name is not found.

%End

  int getDimX() const;
%Docstring
int SliceViewer::getDimX()
--------------------------

    Returns:
        the index of the dimension that is currently
    being shown as the X axis of the plot.

%End

  int getDimY() const;
%Docstring
int SliceViewer::getDimY()
--------------------------

    Returns:
        the index of the dimension that is currently
    being shown as the Y axis of the plot.

%End


  //*WIKI* ==== Slice Point ====
  void setSlicePoint(int dim, double value)     throw (std::invalid_argument);
%Docstring
void SliceViewer::setSlicePoint(int dim, double value)
------------------------------------------------------
    Sets the slice point in the given dimension:
    that is, what is the position of the plane in that dimension

    Args:
        dim :: index of the dimension to change
        value :: value of the slice point, in the units of the given dimension.
           This should be within the range of min/max for that dimension.

    Raises:
        std::invalid_argument if the index is invalid

%End

  void setSlicePoint(const QString & dim, double value)   throw (std::invalid_argument, std::runtime_error);
%Docstring
void SliceViewer::setSlicePoint(const QString & dim, double value)
------------------------------------------------------------------
    Sets the slice point in the given dimension:
    that is, what is the position of the plane in that dimension

    Args:
        dim :: name of the dimension to change
        value :: value of the slice point, in the units of the given dimension.
           This should be within the range of min/max for that dimension.

    Raises:
        std::runtime_error if the name is not found in the workspace

%End

  double getSlicePoint(int dim) const     throw (std::invalid_argument);
%Docstring
double SliceViewer::getSlicePoint(int dim)
------------------------------------------
    Returns the slice point in the given dimension

    Args:
        dim :: index of the dimension

    Returns:
        slice point for that dimension. Value has no significance for the
            X or Y display dimensions.

    Raises:
        std::invalid_argument if the index is invalid

%End

  double getSlicePoint(const QString & dim) const   throw (std::invalid_argument, std::runtime_error);
%Docstring
double SliceViewer::getSlicePoint(const QString & dim)
------------------------------------------------------
    Returns the slice point in the given dimension

    Args:
        dim :: name of the dimension

    Returns:
        slice point for that dimension. Value has no significance for the
            X or Y display dimensions.

    Raises:
        std::runtime_error if the name is not found in the workspace

%End


  //*WIKI* ==== View Limits ====
  void setXYLimits(double xleft, double xright, double ybottom, double ytop);
%Docstring
void SliceViewer::setXYLimits(double xleft, double xright, double ybottom, double ytop)
---------------------------------------------------------------------------------------
    Set the limits in X and Y to be shown in the plot.
    The X and Y values are in the units of their respective dimensions.
    You can change the mapping from X/Y in the plot to specific
    dimensions in the displayed workspace using setXYDim().

    You can flip the direction of the scale if you specify,
    e.g., xleft > xright.

    Args:
        xleft   :: x-value on the left side of the graph
        xright  :: x-value on the right side of the graph
        ybottom :: y-value on the bottom of the graph
        ytop    :: y-value on the top of the graph

%End

  QwtDoubleInterval getXLimits() const;
%Docstring
QwtDoubleInterval SliceViewer::getXLimits()
-------------------------------------------

    Returns:
        Returns the [left, right] limits of the view in the X axis.
%End

  QwtDoubleInterval getYLimits() const;
%Docstring
QwtDoubleInterval SliceViewer::getYLimits()
-------------------------------------------

    Returns:
        Returns the [bottom, top] limits of the view in the Y axis.
%End

  void zoomBy(double factor);
%Docstring
void SliceViewer::zoomBy(double factor)
---------------------------------------
    Zoom in or out, keeping the center of the plot in the same position.

    Args:
        factor :: double, if > 1 : zoom in by this factor.
                     if < 1 : it will zoom out.

%End

  void setXYCenter(double x, double y);
%Docstring
void SliceViewer::setXYCenter(double x, double y)
-------------------------------------------------
    Manually set the center of the plot, in X Y coordinates.
    This keeps the plot the same size as previously.
    Use setXYLimits() to modify the size of the plot by setting the X/Y edges,
    or you can use zoomBy() to zoom in/out

    Args:
        x :: new position of the center in X
        y :: new position of the center in Y

%End

  void resetZoom();
%Docstring
void SliceViewer::resetZoom()
-----------------------------
    Automatically resets the zoom view to full axes.
    This will reset the XY limits to the full range of the workspace.
    Use zoomBy() or setXYLimits() to modify the view range.
    This corresponds to the "View Extents" button.

%End


  //*WIKI* ==== Color Map and Scale ====
  void loadColorMap(QString filename);
%Docstring
void SliceViewer::loadColorMap(QString filename)
------------------------------------------------
    Load a color map from a file

    Args:
        filename :: file to open; empty to ask via a dialog box.

%End

  void setColorScale(double min, double max, int type)      throw (std::invalid_argument);
%Docstring
void SliceViewer::setColorScale(double min, double max, int type)
-----------------------------------------------------------------
    Set the color scale limits and type via a method call.

    Args:
        min :: minimum value corresponding to the lowest color on the map
        max :: maximum value corresponding to the highest color on the map
        type :: 0 for linear, 1 for log, 2 for power

    Raises:
        std::invalid_argument if max < min or if the values are
           inconsistent with a log color scale

%End

  void setColorScaleMin(double min)      throw (std::invalid_argument);
%Docstring
void SliceViewer::setColorScaleMin(double min)
----------------------------------------------
    Set the minimum value corresponding to the lowest color on the map

    Args:
        min :: minimum value corresponding to the lowest color on the map

    Raises:
        std::invalid_argument if max < min or if the values are
           inconsistent with a log color scale

%End

  void setColorScaleMax(double max)      throw (std::invalid_argument);
%Docstring
void SliceViewer::setColorScaleMax(double max)
----------------------------------------------
    Set the maximum value corresponding to the lowest color on the map

    Args:
        max :: maximum value corresponding to the lowest color on the map

    Raises:
        std::invalid_argument if max < min or if the values are
           inconsistent with a log color scale

%End

  void setColorScaleLog(bool log);
%Docstring
void SliceViewer::setColorScaleLog(bool log)
--------------------------------------------
    Set whether the color scale is logarithmic

    Args:
        log :: true for a log color scale, false for linear

    Raises:
        std::invalid_argument if the min/max values are inconsistent
           with a log color scale

%End

  int getColorScaleType();
%Docstring
int SliceViewer::getColorScaleType()
--------------------------------------------
    Get scale type as integer value

    Returns:
        0 for linear scale
        1 for log scale
        2 for power scale

%End

  double getColorScaleMin() const;
%Docstring
double SliceViewer::getColorScaleMin()
--------------------------------------

    Returns:
        the value that corresponds to the lowest color on the color map
%End

  double getColorScaleMax() const;
%Docstring
double SliceViewer::getColorScaleMax()
--------------------------------------

    Returns:
        the value that corresponds to the highest color on the color map
%End

  bool getColorScaleLog() const;
%Docstring
bool SliceViewer::getColorScaleLog()
------------------------------------

    Returns:
        True if the color scale is in logarithmic mode
%End

  void setColorScaleAutoFull();
%Docstring
void SliceViewer::setColorScaleAutoFull()
-----------------------------------------
    Automatically sets the min/max of the color scale,
    using the limits in the entire data set of the workspace
    (every bin, even those not currently visible).

%End

  void setColorScaleAutoSlice() ;
%Docstring
void SliceViewer::setColorScaleAutoSlice()
------------------------------------------
    Automatically sets the min/max of the color scale,
    using the limits in the data that is currently visible
    in the plot (only the bins in this slice and within the
    view limits)

%End

  void setColorMapBackground(int r, int g, int b);
%Docstring
void SliceViewer::setColorMapBackground(int r, int g, int b)
------------------------------------------------------------
    Set the "background" color to use in the color map. Default is white.

    This is the color that is shown when:

     - The coordinate is out of bounds of the workspace.
     - When a signal is NAN (not-a-number).
     - When the signal is Zero, if that option is selected using setTransparentZeros()

    Args:
        r :: red component, from 0-255
        g :: green component, from 0-255
        b :: blue component, from 0-255

%End

  void setTransparentZeros(bool transparent);
%Docstring
void SliceViewer::setTransparentZeros(bool transparent)
-------------------------------------------------------
    Set whether to display 0 signal as "transparent" color.

    Args:
        transparent :: true if you want zeros to be transparent.

%End

  void setNormalization(Mantid::API::MDNormalization norm);
%Docstring
void SliceViewer::setNormalization(Mantid::API::MDNormalization norm)
---------------------------------------------------------------------
    Set the normalization mode for viewing the data

    Args:
        norm :: MDNormalization enum. 0=none; 1=volume; 2=# of events

%End

  Mantid::API::MDNormalization getNormalization() const;
%Docstring
Mantid::API::MDNormalization SliceViewer::getNormalization()
------------------------------------------------------------

    Returns:
        the current normalization
%End

  void setColorBarAutoScale(bool autoscale);
%Docstring
void SliceViewer::setColorBarAutoScale(bool autoscale)
------------------------------------------------------
    Set autoscaling for the color scale on or off

	Args:
	    autoscale :: on/off status for autoscaling

%End

  //*WIKI* ==== Dynamic Rebinning ====
  void setRebinThickness(int dim, double thickness)   throw (std::runtime_error);
%Docstring
void SliceViewer::setRebinThickness(int dim, double thickness)
--------------------------------------------------------------
    Set the thickness (above and below the plane) for dynamic rebinning.

    Args:
        dim :: index of the dimension to adjust
        thickness :: thickness to set, in units of the dimension.

    Raises:
        runtime_error if the dimension index is invalid or the thickness is <= 0.0.

%End

  void setRebinNumBins(int xBins, int yBins)   throw (std::runtime_error);
%Docstring
void SliceViewer::setRebinNumBins(int xBins, int yBins)
-------------------------------------------------------
    Set the number of bins for dynamic rebinning.

    Args:
        xBins :: number of bins in the viewed X direction
        yBins :: number of bins in the viewed Y direction

    Raises:
        runtime_error if the number of bins is < 1

%End

  void setRebinMode(bool mode)   throw (std::runtime_error);
%Docstring
void SliceViewer::setRebinMode(bool mode)
------------------------------------------------------
    Sets the SliceViewer in dynamic rebin mode.
    In this mode, the current view area (see setXYLimits()) is used as the
    limits to rebin.
    See setRebinNumBins() to adjust the number of bins in the X/Y dimensions.
    See setRebinThickness() to adjust the thickness in other dimensions.

    Args:
        mode :: true for rebinning mode

%End

  void refreshRebin()   throw (std::runtime_error);
%Docstring
void SliceViewer::refreshRebin()
--------------------------------
    When in dynamic rebinning mode, this refreshes the rebinned area to be the
    currently viewed area. See setXYLimits(), setRebinNumBins(), setRebinThickness()

%End

  MantidQt::SliceViewer::ProxyCompositePeaksPresenter* setPeaksWorkspaces(const QStringList&);
%Docstring
void SliceViewer::setPeaksWorkspaces(const QStringList&)
--------------------------------
    Enable overplotting of one or more peaks workspaces.
    See clearPeaksWorkspaces() for removal.

%End

  void clearPeaksWorkspaces();
%Docstring
void SliceViewer::clearPeaksWorkspaces()
--------------------------------
    Clear overplotting of all overplotted peaks workspaces.
    See setPeaksWorkspaces() for addition.

%End
};


}; // end namespace
}; // end namespace







namespace MantidQt
{
namespace Factory
{

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class WidgetFactory
{
%TypeHeaderCode
#include "MantidQtFactory/WidgetFactory.h"
#include <QVector>
%End

%Docstring
WidgetFactory
=============

    The WidgetFactory is a class to create custom widgets.
    It currently supports creating SliceViewerWindows using:
        WidgetFactory.Instance().createSliceViewerWindow()

    You can also retrieve a previously-created window with:
        WidgetFactory.Instance().getSliceViewerWindow()
    This applies to windows created by Python or via the GUI.

%End


public:
  static MantidQt::Factory::WidgetFactory* Instance();
  MantidQt::SliceViewer::SliceViewerWindow* createSliceViewerWindow(const QString& wsName, const QString& label);
  MantidQt::SliceViewer::SliceViewerWindow* getSliceViewerWindow(const QString& wsName, const QString& label)   throw (std::runtime_error);
  void closeAllSliceViewerWindows();
%Docstring
void WidgetFactory::closeAllSliceViewerWindows()
------------------------------------------------
    Closes every previously-open instance of a SliceViewerWindow.

%End


  MantidQt::SliceViewer::SliceViewer* createSliceViewer(const QString& wsName);
%Docstring
MantidQt::SliceViewer::SliceViewer* WidgetFactory::createSliceViewer(const QString& wsName)
-------------------------------------------------------------------------------------------
    Create an instance of a bare SliceViewer Widget.
    This is only capable of doing 2D views, and cannot do line plots
    since it does not have a LineViewer.

    Use WidgetFactory::createSliceViewerWindow to create a window combining both.

    Args:
        wsName :: name of the workspace to show. Optional, blank for no workspace.

    Returns:
        the created SliceViewer *

%End


private:
  WidgetFactory(const MantidQt::Factory::WidgetFactory&);
  WidgetFactory();
%Docstring
WidgetFactory::WidgetFactory()
------------------------------
    Private constructor. This is not accessible,
    use the Instance() method to access the singleton instance
    instead.

%End

};

}; // end namespace
}; // end namespace

namespace MantidQt
{
namespace RefDetectorViewer
{


//*WIKI* == Classes ==
//*WIKI* Here follows a list of the classes exposed to python and the methods you can execute on them.

/***************************************************************************/

class RefIVConnections: QWidget
{
%TypeHeaderCode
#include "MantidQtRefDetectorViewer/RefIVConnections.h"
%End

%Docstring

RefIVConnections
================
    A RefIVConnections object is responsible for emitting signals about
    changes in the state of the image view.
%End

private:
  RefIVConnections();
};

/***************************************************************************/

class RefMatrixWSImageView
{
%TypeHeaderCode
#include "MantidQtRefDetectorViewer/RefMatrixWSImageView.h"
%End

%Docstring

RefMatrixWSImageView
====================

    A RefMatrixWSImageView is the gateway to the image view display. To
    bring up an image display simply construct an object of this type, i.e.
        image_view = RefMatrixWSImageView("test_ws")

    See the methods for RefMatrixWSImageView, below.
%End


public:
  RefMatrixWSImageView(QString wsname, int peak_min, int peak_max, int back_min, int back_max, int tof_min, int tof_max);

  MantidQt::RefDetectorViewer::RefIVConnections * getConnections();
%Docstring
MantidQt::RefDetectorViewer::RefIVConnections* RefMatrixWSImageView::getConnections()
-------------------------------------------------------------------------------------
    Get the connections widget inside the image view. This object is responsible for
    emitting signals about changes in the state of the image view.

    Returns:
        a pointer to the RefIVConnections object.

%End


};

}; // end namespace
}; // end namespace

namespace MantidQt
{
namespace MantidWidgets
{

//*WIKI* == Classes ==
//*WIKI* Here follows a list of the classes exposed to python and the methods you can execute on them.

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

class FitPropertyBrowser : QDockWidget
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/FitPropertyBrowser.h"
%End
%Docstring

FitPropertyBrowser
=================

    The FitPropertyBrowser is a control for performing curve fitting.

%End

public:
	void fit();
	const QStringList& registeredFunctions()const;
	void setStartX(double);
	void setEndX(double);
	double centre()const;
	void setCentre(double value);
	double height()const;
	void setHeight(double value);
	double fwhm()const;
	void setFwhm(double value);
	QList<double> getParameterValues() const;
	QStringList getParameterNames() const;
};

}; // end namespace MantidWidgets
}; // end namespace MantidQt


namespace MantidQt
{
namespace MantidWidgets
{
class DataProcessorWhiteList
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWhiteList.h"
%End
public:
DataProcessorWhiteList();
void addElement(const QString &, const QString &, const QString &, bool showValue = false, const QString &prefix = "");
};

class DataProcessorPreprocessMap
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPreprocessMap.h"
%End
public:
DataProcessorPreprocessMap();
void addElement(const QString &, const QString &, const QString &prefix = "", const QString &blacklist = "");
};

class DataProcessorProcessingAlgorithm
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessingAlgorithm.h"
%End
public:
DataProcessorProcessingAlgorithm(const QString &, const QString &, const QString &blacklist = "");
};

class DataProcessorPostprocessingAlgorithm
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPostprocessingAlgorithm.h"
%End
public:
DataProcessorPostprocessingAlgorithm(const QString &, const QString &, const QString &);
};

class QDataProcessorWidget : QWidget
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorWidget.h"
%End
public:
QDataProcessorWidget(const MantidQt::MantidWidgets::DataProcessorWhiteList &,
                     const MantidQt::MantidWidgets::DataProcessorProcessingAlgorithm &,
                     QWidget *parent );
QDataProcessorWidget(const MantidQt::MantidWidgets::DataProcessorWhiteList &,
                     const MantidQt::MantidWidgets::DataProcessorPreprocessMap &,
                     const MantidQt::MantidWidgets::DataProcessorProcessingAlgorithm &,
                     QWidget *parent );
QDataProcessorWidget(const MantidQt::MantidWidgets::DataProcessorWhiteList &,
                     const MantidQt::MantidWidgets::DataProcessorProcessingAlgorithm &,
                     const MantidQt::MantidWidgets::DataProcessorPostprocessingAlgorithm &,
                     QWidget *parent );
QDataProcessorWidget(const MantidQt::MantidWidgets::DataProcessorWhiteList &,
                     const MantidQt::MantidWidgets::DataProcessorPreprocessMap &,
                     const MantidQt::MantidWidgets::DataProcessorProcessingAlgorithm &,
                     const MantidQt::MantidWidgets::DataProcessorPostprocessingAlgorithm &,
                     QWidget *parent );
void accept(MantidQt::MantidWidgets::DataProcessorMainPresenter *);
void setInstrumentList(const QString &instruments, const QString &defaultInstrument);
void transfer(const QList<QString> &);

signals:
void runPythonCode(const QString &);

private:
QDataProcessorWidget(const MantidQt::MantidWidgets::QDataProcessorWidget &);
};

class DataProcessorMainPresenter
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMainPresenter.h"
%End
public:
DataProcessorMainPresenter();

virtual QString getPreprocessingOptionsAsString() const;
virtual QString getProcessingOptions() const;
virtual QString getPostprocessingOptions() const;

virtual void notifyADSChanged(const QSet<QString> &);
};

class DataProcessorAppendRowCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorAppendRowCommand.h"
%End
public:
DataProcessorAppendRowCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorAppendRowCommand();
DataProcessorAppendRowCommand(const MantidQt::MantidWidgets::DataProcessorAppendRowCommand &);
};

class DataProcessorAppendGroupCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorAppendGroupCommand.h"
%End
public:
DataProcessorAppendGroupCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorAppendGroupCommand();
DataProcessorAppendGroupCommand(const MantidQt::MantidWidgets::DataProcessorAppendGroupCommand &);
};

class DataProcessorClearSelectedCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorClearSelectedCommand.h"
%End
public:
DataProcessorClearSelectedCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorClearSelectedCommand();
DataProcessorClearSelectedCommand(const MantidQt::MantidWidgets::DataProcessorClearSelectedCommand &);
};

class DataProcessorCopySelectedCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCopySelectedCommand.h"
%End
public:
DataProcessorCopySelectedCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorCopySelectedCommand();
DataProcessorCopySelectedCommand(const MantidQt::MantidWidgets::DataProcessorCopySelectedCommand &);
};

class DataProcessorCutSelectedCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCutSelectedCommand.h"
%End
public:
DataProcessorCutSelectedCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorCutSelectedCommand();
DataProcessorCutSelectedCommand(const MantidQt::MantidWidgets::DataProcessorCutSelectedCommand &);
};

class DataProcessorDeleteGroupCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorDeleteGroupCommand.h"
%End
public:
DataProcessorDeleteGroupCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorDeleteGroupCommand();
DataProcessorDeleteGroupCommand(const MantidQt::MantidWidgets::DataProcessorDeleteGroupCommand &);
};

class DataProcessorDeleteRowCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorDeleteRowCommand.h"
%End
public:
DataProcessorDeleteRowCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorDeleteRowCommand();
DataProcessorDeleteRowCommand(const MantidQt::MantidWidgets::DataProcessorDeleteRowCommand &);
};

class DataProcessorExpandCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorExpandCommand.h"
%End
public:
DataProcessorExpandCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorExpandCommand();
DataProcessorExpandCommand(const MantidQt::MantidWidgets::DataProcessorExpandCommand &);
};

class DataProcessorExportTableCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorExportTableCommand.h"
%End
public:
DataProcessorExportTableCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorExportTableCommand();
DataProcessorExportTableCommand(const MantidQt::MantidWidgets::DataProcessorExportTableCommand &);
};

class DataProcessorGroupRowsCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorGroupRowsCommand.h"
%End
public:
DataProcessorGroupRowsCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorGroupRowsCommand();
DataProcessorGroupRowsCommand(const MantidQt::MantidWidgets::DataProcessorGroupRowsCommand &);
};

class DataProcessorImportTableCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorImportTableCommand.h"
%End
public:
DataProcessorImportTableCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorImportTableCommand();
DataProcessorImportTableCommand(const MantidQt::MantidWidgets::DataProcessorImportTableCommand &);
};

class DataProcessorNewTableCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorNewTableCommand.h"
%End
public:
DataProcessorNewTableCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorNewTableCommand();
DataProcessorNewTableCommand(const MantidQt::MantidWidgets::DataProcessorNewTableCommand &);
};

class DataProcessorOpenTableCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorOpenTableCommand.h"
%End
public:
DataProcessorOpenTableCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorOpenTableCommand();
DataProcessorOpenTableCommand(const MantidQt::MantidWidgets::DataProcessorOpenTableCommand &);
};

class DataProcessorOptionsCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorOptionsCommand.h"
%End
public:
DataProcessorOptionsCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorOptionsCommand();
DataProcessorOptionsCommand(const MantidQt::MantidWidgets::DataProcessorOptionsCommand &);
};

class DataProcessorPasteSelectedCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPasteSelectedCommand.h"
%End
public:
DataProcessorPasteSelectedCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorPasteSelectedCommand();
DataProcessorPasteSelectedCommand(const MantidQt::MantidWidgets::DataProcessorPasteSelectedCommand &);
};

class DataProcessorPlotGroupCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPlotGroupCommand.h"
%End
public:
DataProcessorPlotGroupCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorPlotGroupCommand();
DataProcessorPlotGroupCommand(const MantidQt::MantidWidgets::DataProcessorPlotGroupCommand &);
};

class DataProcessorPlotRowCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPlotRowCommand.h"
%End
public:
DataProcessorPlotRowCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorPlotRowCommand();
DataProcessorPlotRowCommand(const MantidQt::MantidWidgets::DataProcessorPlotRowCommand &);
};

class DataProcessorProcessCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessCommand.h"
%End
public:
DataProcessorProcessCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorProcessCommand();
DataProcessorProcessCommand(const MantidQt::MantidWidgets::DataProcessorProcessCommand &);
};

class DataProcessorSaveTableCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableCommand.h"
%End
public:
DataProcessorSaveTableCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorSaveTableCommand();
DataProcessorSaveTableCommand(const MantidQt::MantidWidgets::DataProcessorSaveTableCommand &);
};

class DataProcessorSaveTableAsCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableAsCommand.h"
%End
public:
DataProcessorSaveTableAsCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorSaveTableAsCommand();
DataProcessorSaveTableAsCommand(const MantidQt::MantidWidgets::DataProcessorSaveTableAsCommand &);
};

class DataProcessorSeparatorCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSeparatorCommand.h"
%End
public:
DataProcessorSeparatorCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorSeparatorCommand();
DataProcessorSeparatorCommand(const MantidQt::MantidWidgets::DataProcessorSeparatorCommand &);
};

class DataProcessorWorkspaceCommand
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWorkspaceCommand.h"
%End
public:
DataProcessorWorkspaceCommand(const MantidQt::MantidWidgets::QDataProcessorWidget &, const QString &);
void execute();
QString name();
QString icon();
QString tooltip();
QString whatsthis();
QString shortcut();

private:
DataProcessorWorkspaceCommand();
DataProcessorWorkspaceCommand(const MantidQt::MantidWidgets::DataProcessorWorkspaceCommand &);
};

}; // end namespace MantidWidgets
}; // end namespace MantidQt

namespace MantidQt
{
namespace MantidWidgets
{

class SlitCalculator : QDialog
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/SlitCalculator.h"
%End
%Docstring

SlitCalculator
==============

    The SlitCalculator is a control for interacting with the CalculateSlits algorithm.

%End

public:
  SlitCalculator(QWidget* parent);
  virtual ~SlitCalculator();
  void setCurrentInstrumentName(char*);
  void processInstrumentHasBeenChanged();

};

}; // end namespace MantidWidgets
}; // end namespace MantidQt



/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

namespace MantidQt
{
namespace API
{

class WidgetScrollbarDecorator
{
%TypeHeaderCode
#include "MantidQtAPI/WidgetScrollbarDecorator.h"
%End
%Docstring

WidgetScrollbarDecorator
=================

    Adds scrollbar functionality to a QWidget

    Mainly intended to make QDialogs and QMainWindows scrollable since this
    feature is missing in Qt. This allows particularly large dialogs to be
    used on laptops and other devices with small screens.

    Usage:
        from mantidqtpython import MantidQt

        # Usually in __init__ or other setup function
        self._scrollbars = MantidQt.API.WidgetScrollbarDecorator(self)
        self._scrollbars.setEnabled(True) # Must come after setupUi(self)!

%End

public:
    explicit WidgetScrollbarDecorator(QWidget *target);
%Docstring
WidgetScrollbarDecorator::WidgetScrollbarDecorator(QWidget *target)
------------------------------
    Constructor. Decorates the target widget to add scrollbars to it.

    Scrollbars are disabled by default and must be enabled using:
    setEnabled(True)

%End

    virtual ~WidgetScrollbarDecorator();

    bool enabled() const;
    void setEnabled(bool enable);
%Docstring
WidgetScrollbarDecorator::setEnabled(bool enable)
------------------------------
    Enables or disables scrollbars on the target widget.

    Note: setupUi() MUST be called before scrollbars are enabled.

%End

    void setThresholdWidth(int width);
%Docstring
setThresholdWidth(int width)
------------------------------
    Set the width, in pixels, at which scrollbars should appear

%End

    void setThresholdHeight(int height);
%Docstring
setThresholdHeight(int height)
------------------------------
    Set the height, in pixels, at which scrollbars should appear

%End

    void setThresholdSize(int width, int height);
%Docstring
setThresholdSize(int width, int height)
------------------------------
    Set the size, in pixels, at which scrollbars should appear

%End

};

}; // end namespace API
}; // end namespace MantidQt

namespace MantidQt
{
namespace MantidWidgets
{
class InstrumentWidgetTab: QFrame
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/InstrumentView/InstrumentWidgetTab.h"
%End
public:
	// This is a duplicate from the same enum in InstrumentWidget, just so you
	// can do like InstrumentWidgetRenderTab.FULL3D
	enum SurfaceType { FULL3D, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z,
                     SPHERICAL_X, SPHERICAL_Y, SPHERICAL_Z,
                     RENDERMODE_SIZE };
private:
  InstrumentWidgetTab(const MantidQt::MantidWidgets::InstrumentWidgetTab &);
};

class InstrumentWidget: QWidget
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/InstrumentView/InstrumentWidget.h"
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

  MantidQt::MantidWidgets::InstrumentWidgetTab * getTab(const QString & title) const;
%Docstring
    Returns a handler to the requested tab

    Args:

        title The full title of a tab in the window

    Returns:
        a pointer to the requested tab widget

%End

  MantidQt::MantidWidgets::InstrumentWidgetTab * getTab(const Tab tab) const;
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
InstrumentWidget(const MantidQt::MantidWidgets::InstrumentWidget &);
};

class InstrumentWidgetRenderTab: MantidQt::MantidWidgets::InstrumentWidgetTab
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/InstrumentView/InstrumentWidgetRenderTab.h"
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
  InstrumentWidgetRenderTab(const MantidQt::MantidWidgets::InstrumentWidgetRenderTab &);
};

class InstrumentWidgetPickTab: MantidQt::MantidWidgets::InstrumentWidgetTab
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/InstrumentView/InstrumentWidgetPickTab.h"
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
  InstrumentWidgetPickTab(const MantidQt::MantidWidgets::InstrumentWidgetPickTab &);
};

class InstrumentWidgetMaskTab: MantidQt::MantidWidgets::InstrumentWidgetTab
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/InstrumentView/InstrumentWidgetMaskTab.h"
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
  InstrumentWidgetMaskTab(const MantidQt::MantidWidgets::InstrumentWidgetMaskTab &);
};

class InstrumentWidgetTreeTab: MantidQt::MantidWidgets::InstrumentWidgetTab
{
%TypeHeaderCode
#include "MantidQtMantidWidgets/InstrumentView/InstrumentWidgetTreeTab.h"
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
  InstrumentWidgetTreeTab(const MantidQt::MantidWidgets::InstrumentWidgetTreeTab &);
};

};//end namespace MantidWidgets
};//end namespace MantidQt
