//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidQtCustomInterfaces/MuonAnalysisFitDataTab.h"
#include "MantidKernel/ConfigService.h"

#include "MantidQtAPI/UserSubWindow.h"

#include <boost/shared_ptr.hpp>
#include <fstream>  

#include <QLineEdit>
#include <QFileDialog>
#include <QHash>
#include <QTextStream>
#include <QTreeWidgetItem>
#include <QSettings>
#include <QMessageBox>
#include <QInputDialog>
#include <QSignalMapper>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QTemporaryFile>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QtBoolPropertyManager>

//-----------------------------------------------------------------------------

namespace MantidQt
{
namespace CustomInterfaces
{
namespace Muon
{

/**
*  Do stuff before executing fit - e.g. handle fitting against 
*   raw while the data are plotted as bunch
*  @param p contain parameters set by the user in the fit property browser
*/
void MuonAnalysisFitDataTab::beforeDoFit(const QtBoolPropertyManager* p)
{

  bool wantToFitAgainstRawData = p->property("Fit To Raw Data").isValid();
  if (wantToFitAgainstRawData)
  {

  }
  else // Fit against bunch
  {
    
  } 
}

}
}
}