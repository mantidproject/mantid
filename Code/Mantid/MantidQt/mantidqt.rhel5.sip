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


//  //*WIKI* ==== Dynamic Rebinning ====
//  void setRebinThickness(int dim, double thickness)   throw (std::runtime_error);
//  void setRebinNumBins(int xBins, int yBins)   throw (std::runtime_error);
//  void setRebinMode(bool mode, bool locked)   throw (std::runtime_error);
//  void refreshRebin()   throw (std::runtime_error);
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

