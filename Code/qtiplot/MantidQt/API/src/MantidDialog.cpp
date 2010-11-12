//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/MantidDialog.h"

#include <QMessageBox>

using namespace MantidQt::API;

//------------------------------------------------------
// Public member functions
//------------------------------------------------------
/**
 * Default Constructor
 */
MantidDialog::MantidDialog(QWidget* parent):QDialog(parent)
{
  m_pyRunner = boost::shared_ptr<PythonRunner>(new PythonRunner());
  // re-emit the run Python code from m_pyRunner, to work this signal must reach the slot in QtiPlot
  connect(m_pyRunner.get(), SIGNAL(runAsPythonScript(const QString&)),
    this, SIGNAL(runAsPythonScript(const QString&)));
}

/**
 * Destructor
 */
MantidDialog::~MantidDialog()
{
}

/**
 *   Checks if receiver derives from MantidDialog. If it does calls the virtual handleException method.
 *   @param receiver The Qt event receiver
 *   @param e The exception
 *   @return True if the exception was handled, false otherwise.
 */
bool MantidDialog::handle( QObject* receiver, const std::exception& e )
{
    QObject* obj = receiver;
    while(obj)
    {
        if (obj->inherits("MantidQt::API::MantidDialog"))
        {
            qobject_cast<MantidDialog*>(obj)->handleException(e);
            return true;
        }
        obj = obj->parent();
    };
    return false;
}

/** Override this method to handle an exception in a derived class.
 *  @param e exception to handle
 */
void MantidDialog::handleException( const std::exception& e )
{
    QMessageBox::critical(qobject_cast<QWidget*>(parent()),"Mantid - Error",
        "Exception is caught in dialog:\n\n"+QString::fromStdString(e.what()));
    close();
}

/** Run a piece of python code and return any output that it writes to stdout
*  @param code the Python commands to execute
*  @param no_output if set to true this method returns an empty string, if false it returns the output from any Python print statements
*  @return output from Python print statements unless no_output is false
*/
QString MantidDialog::runPythonCode(const QString & code, bool no_output)
{
  return m_pyRunner->runPythonCode(code, no_output);
}
