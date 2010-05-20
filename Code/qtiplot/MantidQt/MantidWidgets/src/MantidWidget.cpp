#include "MantidQtMantidWidgets/MantidWidget.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/PropertyWithValue.h"
#include <QPalette>
#include <QTemporaryFile>
#include <QTextStream>
#include <QDir>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

bool MantidWidget::g_pythonRunning = false;

/**Creates a widget whose runAsPythonScript() signal is connects MantidPlot via a GUI
*  @param parent a GUI object whose runAsPythonScript() is connected to MantidPlot
*/
MantidWidget::MantidWidget(QWidget *parent) : QWidget(parent)
{
  if (parent)
  {
    // interface widgets have their runAsPythonScript signal connected to Qtiplot, this widget will only run Python if this signal is connected to an interface
    connect(this, SIGNAL(runAsPythonScript(const QString&)),
            parent, SIGNAL(runAsPythonScript(const QString&)));
  }
}

void MantidWidget::renameWorkspace(const QString &oldName, const QString &newName)
{
  IAlgorithm_sptr rename =
    AlgorithmManager::Instance().createUnmanaged("RenameWorkspace");
  rename->initialize();
  rename->setPropertyValue("InputWorkspace", oldName.toStdString());
  rename->setPropertyValue("OutputWorkspace", newName.toStdString());

  rename->execute();
}
void MantidWidget::setupValidator(QLabel *star)
{
  QPalette pal = star->palette();
  pal.setColor(QPalette::WindowText, Qt::darkRed);
  star->setPalette(pal);
}
QLabel* MantidWidget::newStar(const QGroupBox * const UI, int valRow, int valCol)
{// use new to create the QLabel the layout will take ownership and delete it later
  QLabel *validLbl = new QLabel("*");
  setupValidator(validLbl);
  // link the validator into the location specified by the user
  QGridLayout *grid = qobject_cast<QGridLayout*>(UI->layout());
  grid->addWidget(validLbl, valRow, valCol);
  return validLbl;
}
QLabel* MantidWidget::newStar(QGridLayout * const lay, int valRow, int valCol)
{// use new to create the QLabel the layout will take ownership and delete it later
  QLabel *validLbl = new QLabel("*");
  setupValidator(validLbl);
  // link the validator into the location specified by the user
  lay->addWidget(validLbl, valRow, valCol);
  return validLbl;
}
void MantidWidget::hideValidators()
{// loop through all the validators in the map
  QHash<const QWidget * const, QLabel *>::iterator
	  vali = m_validators.begin();
  for ( ; vali != m_validators.end(); ++vali)
  {
    vali.value()->hide();    
  }
}
//??STEVES?? this is a copy of what is in the interfaces class
/** Run a piece of python code and return any output that it writes to stdout
*  @param code the Python commands to execute
*  @param no_output if set to true this method returns an empty string, if false it returns the output from any Python print statements
*  @return output from Python print statements unless no_output is false
*/
QString MantidWidget::runPythonCode(const QString & code, bool no_output)
{
  if( no_output )
  {
    runPython(code);
    return QString();
  }
  
  // Otherwise we need to gather the information from stdout. This is achieved by redirecting the stdout stream
  // to a temproary file and then reading its contents
  // A QTemporaryFile object is used since the file is automatically deleted when the object goes out of scope
  QTemporaryFile tmp_file;
  if( !tmp_file.open() )
  {
    throw std::runtime_error("An error occurred opening a temporary file in " + QDir::tempPath().toStdString());
  }
  //The file name is only valid when the file is open
   QString tmpstring = tmp_file.fileName();
   tmp_file.close();
   QString code_to_run = "import sys; sys.stdout = open('" + tmpstring + "', 'w')\n" + code;

   runPython(code_to_run);

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
/** Checks if g_pythonRunning is set before running a script. If it is not set this
*  method sets it before sending a runAsPythonScript(code) signal and unsets it on completion
*  @param code Python commands to run
*/
void MantidWidget::runPython(const QString &code)
{
  if ( ! g_pythonRunning )
  {
    g_pythonRunning = true;
    emit runAsPythonScript(code);
    g_pythonRunning = false;
  }
}