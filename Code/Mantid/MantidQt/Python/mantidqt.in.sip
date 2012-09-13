/***************************************************************************/
/**
 This file is a SIP file for binding C++ methods of the 
 MantidQt widgets (SliceViewer and RefDetectorViewer in particular)
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
#include "SliceViewer/inc/MantidQtSliceViewer/SliceViewerWindow.h"
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
  MantidQt::SliceViewer::LineViewer* getLiner();
  const QString& getLabel() const;

};




/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class LineViewer : QWidget
{
%TypeHeaderCode
#include "SliceViewer/inc/MantidQtSliceViewer/LineViewer.h"
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
void showPreview();
void showFull();

//*WIKI* ==== Start/End Points ====
void setStartXY(double x, double y)   throw (std::runtime_error);
void setEndXY(double x, double y)   throw (std::runtime_error);

//*WIKI* ==== Width ====
void setThickness(double width);
void setThickness(int dim, double width)   throw (std::invalid_argument, std::runtime_error);
void setThickness(const QString & dim, double width)   throw (std::invalid_argument, std::runtime_error);
void setPlanarWidth(double width);
double getPlanarWidth() const;

//*WIKI* ==== Binning ====
void setNumBins(int numBins)   throw (std::invalid_argument);
void setFixedBinWidthMode(bool fixedWidth, double binWidth)   throw (std::invalid_argument);
double getFixedBinWidth() const;
bool getFixedBinWidthMode() const;
int getNumBins() const;
double getBinWidth() const;

////*WIKI* ==== Plotting ====
void setPlotAxis(int choice);
int getPlotAxis() const;

};


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class SliceViewer : QWidget
{
%TypeHeaderCode
#include "SliceViewer/inc/MantidQtSliceViewer/SliceViewer.h"
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
  QString getWorkspaceName() const;
  void showControls(bool visible);
  void openFromXML(const QString & xml)   throw (std::invalid_argument, std::runtime_error);
  QPixmap getImage();
  void saveImage(const QString & filename);
  void copyImageToClipboard();
  void setFastRender(bool fast);
  bool getFastRender() const;
  void toggleLineMode(bool lineMode);

  //*WIKI* ==== X/Y Dimension ====
  void setXYDim(int indexX, int indexY)     throw (std::invalid_argument);
  void setXYDim(const QString & dimX, const QString & dimY)     throw (std::invalid_argument, std::runtime_error);
  int getDimX() const;
  int getDimY() const;

  //*WIKI* ==== Slice Point ====
  void setSlicePoint(int dim, double value)     throw (std::invalid_argument);
  void setSlicePoint(const QString & dim, double value)   throw (std::invalid_argument, std::runtime_error);
  double getSlicePoint(int dim) const     throw (std::invalid_argument);
  double getSlicePoint(const QString & dim) const   throw (std::invalid_argument, std::runtime_error);

  //*WIKI* ==== View Limits ====
  void setXYLimits(double xleft, double xright, double ybottom, double ytop);
  QwtDoubleInterval getXLimits() const;
  QwtDoubleInterval getYLimits() const;
  void zoomBy(double factor);
  void setXYCenter(double x, double y);
  void resetZoom();

  //*WIKI* ==== Color Map and Scale ====
  void loadColorMap(QString filename);
  void setColorScale(double min, double max, bool log)      throw (std::invalid_argument);
  void setColorScaleMin(double min)      throw (std::invalid_argument);
  void setColorScaleMax(double max)      throw (std::invalid_argument);
  void setColorScaleLog(bool log);
  double getColorScaleMin() const;
  double getColorScaleMax() const;
  bool getColorScaleLog() const;
  void setColorScaleAutoFull();
  void setColorScaleAutoSlice() ;
  void setColorMapBackground(int r, int g, int b);
  void setTransparentZeros(bool transparent);
  void setNormalization(Mantid::API::MDNormalization norm);
  Mantid::API::MDNormalization getNormalization() const;

  //*WIKI* ==== Dynamic Rebinning ====
  void setRebinThickness(int dim, double thickness)   throw (std::runtime_error);
  void setRebinNumBins(int xBins, int yBins)   throw (std::runtime_error);
  void setRebinMode(bool mode, bool locked)   throw (std::runtime_error);
  void refreshRebin()   throw (std::runtime_error);
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
#include "Factory/inc/MantidQtFactory/WidgetFactory.h"
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

  MantidQt::SliceViewer::SliceViewer* createSliceViewer(const QString& wsName);

private:
  WidgetFactory();
};

}; // end namespace
}; // end namespace



/****************************************************************************
/*** RefDetectorViewer ******************************************************
/****************************************************************************
namespace MantidQt
{
namespace RefDetectorViewer
{
	
//*WIKI* == Classes ==
//*WIKI* Here follows a list of the classes exposed to python and the methods you can execute on them.

/**************************************
/**************************************
/**************************************

class ArrayDataSource : ImageDataSource
{
%TypeHeaderCode
#include "RefDetectorViewer/inc/MantidQtRefDetectorViewer/ArrayDataSource.h"
%End
%Docstring

ArrayDataSource
===============

	This class provides a wrapper around a simple 2D array of doubles stored
	in row-major order in a 1D array, so that the array can be viewed using
	the RefDetector data viewer.

%End

public:
	ArrayDataSource(double total_xmin, double total_xmax,
				    double total_ymin, double total_ymax,
				    size_t total_rows, size_t total_cols, 
				    float* data);
	MantidQt::RefDetectorViewer::DataArray* getDataArray( double xmin,
														  double xmax,
														  double ymin,
														  double ymax,
														  size_t n_rows,
														  size_t n_cols,
														  bool is_log_x);
														  
};

/**************************************
/**************************************
/**************************************

class DataArray
{
%TypeHeaderCode
#include "RefDetectorViewer/inc/MantidQtRefDetectorViewer/DataArray.h"
%End
%Docstring

DataArray
=========
	
	This class provides a simple immutable wrapper around a block of data
	returned from an ImageDataSource
	
%End

public:
	DataArray(double xmin, double xmax,
			  double ymin, double ymax,
			  bool is_log_x,
			  size_t n_rows, size_t n_cols,
			  float *data);

//*WIKI* == get the smallest or largest x or y values actually covered by the DataArray
	double GetXMin() const;
	double GetXMax() const;
	double GetYMin() const;
	double GetYMax() const;
	
//*WIKI* == check if the returned array is binned logarithmically in 'x'
	bool isLogX() const;
	
//*WIKI* == get the smallest or largest value recorded in the DataArray
	double getDataMin() const;
	double getDataMax() const;
	
//*WIKI* == get the number of rows or columns in the DataArray
	size_t GetNRows() const;
	size_t GetNCols() const;
	
//*WIKI* == get simple array containing all values, packed in a 1D array	
	float *GetData() const;
	
//*WIKI* == get the value at the specified row and column
	double GetValue(int row, int col) const;

//*WIKI* == get the value from the row and column containing the specified point
	double GetValue(double x, double y) const;
	
};	

/**************************************
/**************************************
/**************************************

class ImageView : QMainWindow
{
%TypeHeaderCode
#include "RefDetectorViewer/inc/MantidQtRefDetectorViewer/ImageView.h"
%End
%Docstring

ImageView
=========

	This is the QMainWindow for the RefDetectorViewer. Data is
	displayed in an ImageView by constructing the ImageView object
	and specifying a particular Data Source
	
%End

public:
	ImageView(ImageDataSource* dataSource);
	
};

}; // end namespace
}; // end namespace

