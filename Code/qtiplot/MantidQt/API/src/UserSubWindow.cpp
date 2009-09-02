//----------------------------------
// Includes
//----------------------------------
// Disable Qt lowercase keywords as this includes a boost signal header
#define QT_NO_KEYWORDS
#include "MantidKernel/SignalChannel.h"
#undef QT_NO_KEYWORDS
#include "MantidQtAPI/UserSubWindow.h"

#include <QIcon>
#include <QMessageBox>
#include <QDir>
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
UserSubWindow::UserSubWindow(QWidget* parent) :  QWidget(parent), m_bIsInitialized(false), m_ifacename("")
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
 *  A boost 'slot' for the Mantid signal channel connection. This relays the message to
 * a Qt signal
 * @param msg The Poco message parameter
 */
void UserSubWindow::mantidLogReceiver(const Poco::Message & msg)
{
  emit logMessageReceived(QString::fromStdString(msg.getText()));
}

//--------------------------------------
// Protected member functions
//-------------------------------------
/**
 * Raise a dialog box with some information for the user
 * @param message The message to show
 */

void UserSubWindow::showInformationBox(const QString & message)
{
  if( !message.isEmpty() )
  {
    QMessageBox::information(this, this->windowTitle(), message);
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

