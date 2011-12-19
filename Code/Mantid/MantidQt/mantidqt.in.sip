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
class WidgetFactoryImpl
{
%TypeHeaderCode
#include "Factory/inc/MantidQtFactory/WidgetFactory.h"
%End

public:
  WidgetFactoryImpl();
  MantidQt::SliceViewer::SliceViewerWindow* createSliceViewerWindow(const QString& wsName, const QString& label);
  MantidQt::SliceViewer::SliceViewer* createSliceViewer(const QString& wsName);

};

}; // end namespace
}; // end namespace


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class WidgetFactory
{
%TypeHeaderCode
#include "Factory/inc/MantidQtFactory/WidgetFactory.h"
%End

public:
  static MantidQt::Factory::WidgetFactoryImpl& Instance();
private:
  WidgetFactory();
};

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
  MantidQt::SliceViewer::LineViewer* getLiner();

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
  void showControls(bool visible);

  void setXYDim(int indexX, int indexY)     throw (std::invalid_argument);
  void setXYDim(const QString & dimX, const QString & dimY)     throw (std::invalid_argument, std::runtime_error);
  int getDimX() const;
  int getDimY() const;

  void setSlicePoint(int dim, double value)     throw (std::invalid_argument);
  void setSlicePoint(const QString & dim, double value)   throw (std::invalid_argument, std::runtime_error);
  double getSlicePoint(int dim) const     throw (std::invalid_argument);
  double getSlicePoint(const QString & dim) const   throw (std::invalid_argument, std::runtime_error);
  void setXYLimits(double xleft, double xright, double ybottom, double ytop);
  QwtDoubleInterval getXLimits() const;
  QwtDoubleInterval getYLimits() const;
  void zoomBy(double factor);
  void setXYCenter(double x, double y);
  void resetZoom();

  void loadColorMap(QString filename);
  void setColorScale(double min, double max, bool log)      throw (std::invalid_argument);
  void setColorScaleMin(double min)      throw (std::invalid_argument);
  void setColorScaleMax(double max)      throw (std::invalid_argument);
  void setColorScaleLog(bool log);
  double getColorScaleMin() const;
  double getColorScaleMax() const;
  bool getColorScaleLog() const;
  void setColorScaleAutoFull();
  void setColorScaleAutoSlice();

};



}; // end namespace
}; // end namespace
