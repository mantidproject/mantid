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
namespace SliceViewer
{


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class SliceViewer : QWidget
{
%TypeHeaderCode
#include "SliceViewer/inc/MantidQtSliceViewer/SliceViewer.h"
%End

public:
  void zoomBy(double factor);
  void setWorkspace(const QString & wsName) throw (std::runtime_error);
  void showControls(bool visible);
  
  void setXYDim(int indexX, int indexY) 	throw (std::invalid_argument);
  int getDimX() const;
  int getDimY() const;

  void setSlicePoint(int dim, double value) 	throw (std::invalid_argument);
  double getSlicePoint(int dim) const 	throw (std::invalid_argument);
  void setXYLimits(double xleft, double xright, double ybottom, double ytop);
  QwtDoubleInterval getXLimits() const;
  QwtDoubleInterval getYLimits() const;

  void loadColorMap(QString filename);
  void setColorScale(double min, double max, bool log) 	throw (std::invalid_argument);
  double getColorScaleMin() const;
  double getColorScaleMax() const;
  bool getColorScaleLog() const;
    
};



}; // end namespace
}; // end namespace
