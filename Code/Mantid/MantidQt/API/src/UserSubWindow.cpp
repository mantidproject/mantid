//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidQtAPI/UserSubWindow.h"

#include <QIcon>
#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include <QTemporaryFile>
#include <QTextStream>

#include <iostream>

using namespace MantidQt::API;

//------------------------------------------------------
// Public member functions
//------------------------------------------------------
/**
 * Default Constructor
 */
UserSubWindow::UserSubWindow(QWidget* parent) :  
  QMainWindow(parent), m_bIsInitialized(false), m_isPyInitialized(false), m_ifacename(""), m_pythonRunner()
{
  setAttribute(Qt::WA_DeleteOnClose, false);

  // re-emit the run Python code from m_pyRunner, to work this signal must reach the slot in QtiPlot
  connect(&m_pythonRunner, SIGNAL(runAsPythonScript(const QString&, bool)),
    this, SIGNAL(runAsPythonScript(const QString&, bool)));

}

/**
 * Destructor
 */
UserSubWindow::~UserSubWindow()
{
}

/**
 * Create the layout for this dialog.
 */
void UserSubWindow::initializeLayout()
{
  if( isInitialized() ) return;

  //Calls the derived class function
  this->initLayout();

  //Se the object name to the interface name
  setObjectName(m_ifacename);

  //Set the icon
  setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));

  m_bIsInitialized = true;
}

/**
 * Has this window been initialized yet
 *  @returns Whether initialzedLayout has been called yet
 */
bool UserSubWindow::isInitialized() const
{ 
  return m_bIsInitialized; 
}

/**
 * Has the Python initialization function been called yet?
 * @returns Whether initializeLocalPython  has been called yet
 */
bool UserSubWindow::isPyInitialized() const
{ 
  return m_isPyInitialized; 
}

/**
 * Initialize local Python environment. This is called once when the interface is created and
 * is meant to be used to run one off code, i.e. importing modules.
 */
void UserSubWindow::initializeLocalPython()
{
  if( isPyInitialized() ) return;

  //Call overridable function
  this->initLocalPython();
  m_isPyInitialized = true;
}

//--------------------------------------
// Protected member functions
//-------------------------------------
/**
 * Raise a dialog box with some information for the user
 * @param message :: The message to show
 */

void UserSubWindow::showInformationBox(const QString & message) const
{
  if( !message.isEmpty() )
  {
    QMessageBox::information(const_cast<UserSubWindow*>(this), this->windowTitle(), message);
  }
}

/**
 * Execute a piece of Python code and the output that was written to stdout, i.e. the output from print
 * statements
 * @param code :: The code to execute
 * @param no_output :: An optional flag to specify that no output is needed. If running only small commands enable this
 * as it should be faster. The default value is false
 */
QString UserSubWindow::runPythonCode(const QString & code, bool no_output)
{
  return m_pythonRunner.runPythonCode(code,no_output);
}

/**
 * Open a file selection box
 * @param save :: if true a save dialog box used (prompts for replace if file exists) otherwise a load file (file must then exist)
 * @param exts :: the dialog boxes will only show files that have extensions that match one of the QStrings in the list
 */
QString UserSubWindow::openFileDialog(const bool save, const QStringList &exts)
{
  QString filter;
  if ( !exts.empty() )
  {
    filter = "";
    for ( int i = 0; i < exts.size(); i ++ )
    {
      filter.append("*." + exts[i] + " ");
    }
    filter = filter.trimmed();
  }
  filter.append(";;All Files (*.*)");

  QString filename;
  if( save )
  {
    filename = MantidQt::API::FileDialogHandler::getSaveFileName(this, "Save file", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
  }
  else
  {
    filename = QFileDialog::getOpenFileName(this, "Open file", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
  }

  if( !filename.isEmpty() ) 
  {
    AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(filename).absoluteDir().path());
  }
  return filename;
}
/** Returns a pointer to a new validator QLabel. The code is copied from
*  AlgorithmDialog.cpp and wont know if the validator label changes there
*  @param parent :: a pointer to an object that will look after it deleting it
*/
QLabel* UserSubWindow::newValidator(QWidget *parent)
{
  QLabel *validLbl = new QLabel("*", parent);
  QPalette pal = validLbl->palette();
  pal.setColor(QPalette::WindowText, Qt::darkRed);
  validLbl->setPalette(pal);
  return validLbl;
}

//--------------------------------------
// Private member functions
//-------------------------------------
/**
 * Set the interface name
 * @param iface_name :: The name of the interface
 */
void UserSubWindow::setInterfaceName(const QString & iface_name)
{
  m_ifacename = iface_name;
}
