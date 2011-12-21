/***************************************************************************/
/**
 This file is a SIP file for binding C++ methods of the 
 MantidQt widgets (SliceViewer in particular)
 to Python.
 
*/
/***************************************************************************/

// Define the module name. This has to match the library filename
%Module mantidqtpython

/******************************** SIP Imports ****************/
%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Include qwttypes.sip

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
#include "Factory/inc/MantidQtFactory/WidgetFactory.h"
%End

public:
  static MantidQt::Factory::WidgetFactory* Instance();
  MantidQt::SliceViewer::SliceViewerWindow* createSliceViewerWindow(const QString& wsName, const QString& label);
  MantidQt::SliceViewer::SliceViewerWindow* getSliceViewerWindow(const QString& wsName, const QString& label)   throw (std::runtime_error);
  MantidQt::SliceViewer::SliceViewer* createSliceViewer(const QString& wsName);
%Docstring
MantidQt::SliceViewer::SliceViewer* WidgetFactory::createSliceViewer(const QString& wsName)
-------------------------------------------------------------------------------------------
    Create an instance of a bare SliceViewer Widget.
    This is only capable of doing 2D views, and cannot do line plots
    since it does not have a LineViewer.
   
    Use WidgetFactory::createSliceViewerWindow to create a window combining both.
   
    @param wsName :: name of the workspace to show. Optional, blank for no workspace.
    @return the created SliceViewer *
   
%End


private:
  WidgetFactory();
%Docstring
WidgetFactory::WidgetFactory()
------------------------------
    Constructor
   
%End


};

}; // end namespace
}; // end namespace



namespace MantidQt
{
namespace SliceViewer
{


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class SliceViewerWindow : QMainWindow
{
%TypeHeaderCode
#include "SliceViewer/inc/MantidQtSliceViewer/SliceViewerWindow.h"
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
   
    @return a pointer to the SliceViewer widget.
   
%End

  MantidQt::SliceViewer::LineViewer* getLiner();
%Docstring
MantidQt::SliceViewer::LineViewer* SliceViewerWindow::getLiner()
----------------------------------------------------------------
    Get the LineViewer widget inside the SliceViewerWindow.
    This is the widget for controlling the 1D line integration
    settings.
   
    @return a pointer to the LineViewer widget.
   
%End

  const QString & getLabel();

};


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class LineViewer : QWidget
{
%TypeHeaderCode
#include "SliceViewer/inc/MantidQtSliceViewer/LineViewer.h"
%End

};


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class SliceViewer : QWidget
{
%TypeHeaderCode
#include "SliceViewer/inc/MantidQtSliceViewer/SliceViewer.h"
%End

public:
  void setWorkspace(const QString & wsName)   throw (std::runtime_error);
%Docstring
void SliceViewer::setWorkspace(const QString & wsName)
------------------------------------------------------
    Set the workspace to view using its name.
    The workspace should be a MDHistoWorkspace or a MDEventWorkspace,
    with at least 2 dimensions.
   
    @param wsName :: name of the MDWorkspace to look for
    @throw std::runtime_error if the workspace is not found or is a MatrixWorkspace
   
%End

  QString getWorkspaceName() const;
%Docstring
QString SliceViewer::getWorkspaceName()
---------------------------------------
    @return the name of the workspace selected, or a blank string
    if no workspace is set.
   
%End

  void showControls(bool visible);
%Docstring
void SliceViewer::showControls(bool visible)
--------------------------------------------
    Programmatically show/hide the controls (sliders etc)
   
    @param visible :: true if you want to show the controls.
   
%End

  void openFromXML(const QString & xml)   throw (std::invalid_argument, std::runtime_error);
%Docstring
void SliceViewer::openFromXML(const QString & xml)
--------------------------------------------------
    Opens a workspace and sets the view and slice points
    given the XML from the MultiSlice view in XML format.
   
    @param xml :: string describing workspace, slice point, etc.
    @throw std::runtime_error if error in parsing XML
   
%End


  void setXYDim(int indexX, int indexY)     throw (std::invalid_argument);
%Docstring
void SliceViewer::setXYDim(int indexX, int indexY)
--------------------------------------------------
    Set the index of the dimensions that will be shown as
    the X and Y axis of the plot.
    You cannot set both axes to be the same.
   
    To be called from Python, primarily.
   
    @param indexX :: index of the X dimension, from 0 to NDims-1.
    @param indexX :: index of the Y dimension, from 0 to NDims-1.
    @throw std::invalid_argument if an index is invalid or repeated.
   
%End

  void setXYDim(const QString & dimX, const QString & dimY)     throw (std::invalid_argument, std::runtime_error);
%Docstring
void SliceViewer::setXYDim(const QString & dimX, const QString & dimY)
----------------------------------------------------------------------
    Set the dimensions that will be shown as the X and Y axes
   
    @param dimX :: name of the X dimension. Must match the workspace dimension names.
    @param dimY :: name of the Y dimension. Must match the workspace dimension names.
    @throw std::runtime_error if the dimension name is not found.
   
%End

  int getDimX() const;
%Docstring
int SliceViewer::getDimX()
--------------------------
    @return the index of the dimension that is currently
    being shown as the X axis of the plot.
   
%End

  int getDimY() const;
%Docstring
int SliceViewer::getDimY()
--------------------------
    @return the index of the dimension that is currently
    being shown as the Y axis of the plot.
   
%End


  void setSlicePoint(int dim, double value)     throw (std::invalid_argument);
%Docstring
void SliceViewer::setSlicePoint(int dim, double value)
------------------------------------------------------
    Sets the slice point in the given dimension:
    that is, what is the position of the plane in that dimension
   
    @param dim :: index of the dimension to change
    @param value :: value of the slice point, in the units of the given dimension.
           This should be within the range of min/max for that dimension.
   
%End

  void setSlicePoint(const QString & dim, double value)   throw (std::invalid_argument, std::runtime_error);
%Docstring
void SliceViewer::setSlicePoint(const QString & dim, double value)
------------------------------------------------------------------
    Sets the slice point in the given dimension:
    that is, what is the position of the plane in that dimension
   
    @param dim :: name of the dimension to change
    @param value :: value of the slice point, in the units of the given dimension.
           This should be within the range of min/max for that dimension.
   
%End

  double getSlicePoint(int dim) const     throw (std::invalid_argument);
%Docstring
double SliceViewer::getSlicePoint(int dim)
------------------------------------------
    Returns the slice point in the given dimension
   
    @param dim :: index of the dimension
    @return slice point for that dimension. Value has not significance for the X or Y display dimensions.
   
%End

  double getSlicePoint(const QString & dim) const   throw (std::invalid_argument, std::runtime_error);
%Docstring
double SliceViewer::getSlicePoint(const QString & dim)
------------------------------------------------------
    Returns the slice point in the given dimension
   
    @param dim :: name of the dimension
    @return slice point for that dimension. Value has not significance for the X or Y display dimensions.
   
%End

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
   
    @param xleft   :: x-value on the left side of the graph
    @param xright  :: x-value on the right side of the graph
    @param ybottom :: y-value on the bottom of the graph
    @param ytop    :: y-value on the top of the graph
   
%End

  QwtDoubleInterval getXLimits() const;
%Docstring
QwtDoubleInterval SliceViewer::getXLimits()
-------------------------------------------
    @return Returns the [left, right] limits of the view in the X axis. 
%End

  QwtDoubleInterval getYLimits() const;
%Docstring
QwtDoubleInterval SliceViewer::getYLimits()
-------------------------------------------
    @return Returns the [bottom, top] limits of the view in the Y axis. 
%End

  void zoomBy(double factor);
%Docstring
void SliceViewer::zoomBy(double factor)
---------------------------------------
    Zoom in or out, keeping the center of the plot in the same position.
   
    @param factor :: double, if > 1 : zoom in by this factor.
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
   
    @param x :: new position of the center in X
    @param y :: new position of the center in Y
   
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


  void loadColorMap(QString filename);
%Docstring
void SliceViewer::loadColorMap(QString filename)
------------------------------------------------
    Load a color map from a file
   
    @param filename :: file to open; empty to ask via a dialog box.
   
%End

  void setColorScale(double min, double max, bool log)      throw (std::invalid_argument);
%Docstring
void SliceViewer::setColorScale(double min, double max, bool log)
-----------------------------------------------------------------
    Set the color scale limits and log mode via a method call.
   
    @param min :: minimum value corresponding to the lowest color on the map
    @param max :: maximum value corresponding to the highest color on the map
    @param log :: true for a log color scale, false for linear
    @throw std::invalid_argument if max < min or if the values are
           inconsistent with a log color scale
   
%End

  void setColorScaleMin(double min)      throw (std::invalid_argument);
%Docstring
void SliceViewer::setColorScaleMin(double min)
----------------------------------------------
    Set the minimum value corresponding to the lowest color on the map
   
    @param min :: minimum value corresponding to the lowest color on the map
    @throw std::invalid_argument if max < min or if the values are
           inconsistent with a log color scale
   
%End

  void setColorScaleMax(double max)      throw (std::invalid_argument);
%Docstring
void SliceViewer::setColorScaleMax(double max)
----------------------------------------------
    Set the maximum value corresponding to the lowest color on the map
   
    @param max :: maximum value corresponding to the lowest color on the map
    @throw std::invalid_argument if max < min or if the values are
           inconsistent with a log color scale
   
%End

  void setColorScaleLog(bool log);
%Docstring
void SliceViewer::setColorScaleLog(bool log)
--------------------------------------------
    Set whether the color scale is logarithmic
   
    @param log :: true for a log color scale, false for linear
    @throw if the min/max values are inconsistent with a log color scale
   
%End

  double getColorScaleMin() const;
%Docstring
double SliceViewer::getColorScaleMin()
--------------------------------------
    @return the value that corresponds to the lowest color on the color map 
%End

  double getColorScaleMax() const;
%Docstring
double SliceViewer::getColorScaleMax()
--------------------------------------
    @return the value that corresponds to the highest color on the color map 
%End

  bool getColorScaleLog() const;
%Docstring
bool SliceViewer::getColorScaleLog()
------------------------------------
    @return True if the color scale is in logarithmic mode 
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


};




}; // end namespace
}; // end namespace
