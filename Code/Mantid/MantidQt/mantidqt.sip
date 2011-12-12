/***************************************************************************/
/**
 This file is a SIP file for binding C++ methods of the 
 MantidQt widgets (SliceViewer in particular)
 to Python.
 
*/

// Define the module name. This has to match the library filename
%Module libmantidqtpython

/******************************** SIP Imports ****************/
%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip



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
  void setWorkspace(const QString & wsName);
};



}; // end namespace
}; // end namespace
