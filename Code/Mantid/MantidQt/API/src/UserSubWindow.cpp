//----------------------------------
// Includes
//----------------------------------
// Disable Qt lowercase keywords as this includes a boost signal header
#define QT_NO_KEYWORDS
#include "MantidKernel/SignalChannel.h"
#undef QT_NO_KEYWORDS
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include <QIcon>
#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include <QTemporaryFile>
#include <QTextStream>

// Boost
#include "boost/bind.hpp"

//Poco
#include "Poco/LoggingRegistry.h"

#include <iostream>

using namespace MantidQt::API;

//------------------------------------------------------
// Public member functions
//------------------------------------------------------
/**
 * Default Constructor
 */
UserSubWindow::UserSubWindow(QWidget* parent) :  
  QWidget(parent), m_bIsInitialized(false), m_isPyInitialized(false), m_ifacename("")
{
  setAttribute(Qt::WA_DeleteOnClose, false);
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

  // Make the logging connection
  connectToMantidSignal();

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
 *  A boost 'slot' for the Mantid signal channel connection. This relays the message to
 * a Qt signal
 * @param msg The Poco message parameter
 */
void UserSubWindow::mantidLogReceiver(const Poco::Message & msg)
{
  emit logMessageReceived(QString::fromStdString(msg.getText()));
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
 * @param message The message to show
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
 * @param code The code to execute
 * @param no_output An optional flag to specify that no output is needed. If running only small commands enable this
 * as it should be faster. The default value is false
 */
QString UserSubWindow::runPythonCode(const QString & code, bool no_output)
{
  if( no_output )
  {
    emit runAsPythonScript(code);
    return QString();
  }
  
  // Otherwise we need to gather the information from stdout. This is achieved by redirecting the stdout stream
  // to a temproary file and then reading its contents
  // A QTemporaryFile object is used since the file is automatically deleted when the object goes out of scope
  QTemporaryFile tmp_file;
  if( !tmp_file.open() )
  {
    showInformationBox("An error occurred opening a temporary file in " + QDir::tempPath());
    return QString();
  }
  //The file name is only valid when the file is open
   QString tmpstring = tmp_file.fileName();
   tmp_file.close();
   QString code_to_run = "import sys; sys.stdout = open('" + tmpstring + "', 'w')\n" + code;
   emit runAsPythonScript(code_to_run);

   //Now get the output
   tmp_file.open();
   QTextStream stream(&tmp_file);
   tmpstring.clear();
   while( !stream.atEnd() )
   {
     tmpstring.append(stream.readLine().trimmed() + "\n");
   }
   return tmpstring;
}

/**
 * Open a file selection box
 * @param save if true a save dialog box used (prompts for replace if file exists) otherwise a load file (file must then exist)
 * @param exts the dialog boxes will only show files that have extensions that match one of the QStrings in the list
 */
QString UserSubWindow::openFileDialog(const bool save, const QStringList &exts)
{
  QString filter;
  if ( !exts.empty() )
  {
    filter = "Files (";
    for ( int i = 0; i < exts.size(); i ++ )
    {
      filter.append("*." + exts[i] + " ");
    }
    filter.trimmed();
    filter.append(")");
  }
  filter.append(";;All Files (*.*)");

  QString filename;
  if( save )
  {
    filename = QFileDialog::getSaveFileName(this, "Save file", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
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
*  @param parent a pointer to an object that will look after it deleting it
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
 * @param iface_name The name of the interface
 */
void UserSubWindow::setInterfaceName(const QString & iface_name)
{
  m_ifacename = iface_name;
}

/**
 * Connect to Mantid's signal channel so that we can receive log messages
 */
bool UserSubWindow::connectToMantidSignal()
{
  try
  {
    Poco::SignalChannel *pChannel = 
      dynamic_cast<Poco::SignalChannel*>(Poco::LoggingRegistry::defaultRegistry().channelForName("signalChannel"));
    if( !pChannel )
    { 
      return false;
    }
    pChannel->sig().connect(boost::bind(&UserSubWindow::mantidLogReceiver, this, _1));
  }
  catch(...)
  {
    return false;
  }
  return true;
}

