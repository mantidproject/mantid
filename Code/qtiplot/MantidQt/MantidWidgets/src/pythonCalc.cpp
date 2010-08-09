#include "MantidQtMantidWidgets/pythonCalc.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/AlgorithmManager.h"
#include <QMessageBox>
#include <QHash>
#include <QTextStream>
#include <QFile>
#include <stdexcept>
#include <cmath>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

/** returns a read-only reference to the Python script
* @return an executable python script
*/
const QString& pythonCalc::python() const
{
  return m_pyScript;
}
/** Protected constructor only desendents of this class can be construected
*  @param interface this parent widget needs to have its runAsPythonScript signal connected to MantidPlot
*/
pythonCalc::pythonCalc(QWidget *interface) : MantidWidget(interface),
  m_pyScript(""), m_fails()
{
}
/** Looks for error reports from the object passed to it.  If the map returned by the
*  the objects invalid() method isn't empty it places displays red stars and throws
*  @param pythonGenerat the object with an invalid() map that contains a list of bad controls and error messages
*  @return a description of any error
*/
QString pythonCalc::checkNoErrors(const QHash<const QWidget * const, QLabel *> &validLbls) const
{
  // any errors in reading user values and constructing the script were inserted into a map, go through it
  std::map<const QWidget * const, std::string>::const_iterator errs = m_fails.begin();
  for ( ; errs != m_fails.end(); ++errs)
  {// there are two maps, one links errors to the invalid control and one links controls to validators, put them together to load the errors into the validators
    if ( validLbls.find(errs->first) == validLbls.end() )
	  {// can't find a validator label for this control, throw here, it'll get caught below and a dialog box will be raised
	    return QString::fromStdString(errs->second);
	  }
	  validLbls[errs->first]->setToolTip(QString::fromStdString(errs->second));
	  validLbls[errs->first]->show();
  }
  if ( m_fails.size() > 0 )
  {// some errors were displayed in the loop above
	  return "One or more settings are invalid. The invalid settings are\nmarked with a *, hold your mouse over the * for more information";
  }
  return "";
}
/** Sets m_pyScript to the contents of the named file
* @param pythonFile the name of the file to read
*/
void pythonCalc::appendFile(const QString &pythonFile)
{
  QFile py_script(pythonFile);
  try
  {
    if ( !py_script.open(QIODevice::ReadOnly) )
    {
      throw Mantid::Kernel::Exception::FileError(std::string("Couldn't open python file "), pythonFile.toStdString());
    }
    QTextStream stream(&py_script);
    QString line;
    while( !stream.atEnd() )
    {
	    line = stream.readLine();
  	  m_pyScript.append(line + "\n");
    }
    py_script.close();
  }
  catch( ... )
  {
    py_script.close();
    throw;
  }
}
/** Sets m_pyScript to the contents of the named file
* @param pythonFile the name of the file to read
*/
void pythonCalc::loadFile(const QString &pythonFile)
{
  m_pyScript.clear();
  QFile py_script(pythonFile);
  try
  {
    if ( !py_script.open(QIODevice::ReadOnly) )
    {
      throw Mantid::Kernel::Exception::FileError(std::string("Couldn't open python file "), pythonFile.toStdString());
    }
    QTextStream stream(&py_script);
    QString line;
    while( !stream.atEnd() )
    {
	    line = stream.readLine();
  	  m_pyScript.append(line + "\n");
    }
    py_script.close();
  }
  catch( ... )
  {
    py_script.close();
    throw;
  }
}
/** Replaces the marker word in the m_pyScript with the text in the QLineEdit
*  checking if the value will fail validation, storing any error in m_fails
* @param pythonMark the word to search for and replace
* @param userVal points to the QLineEdit containing the user value
* @param check the property that will be used to for validity checking
*/
void pythonCalc::LEChkCp(QString pythonMark, const QLineEdit * const userVal, Property * const check)
{
  QString setting = userVal->text();
  std::string error = replaceErrsFind(pythonMark, setting, check);
  if ( ! error.empty() )
  {
    m_fails[userVal] = error;
  }
}
/** Replaces the marker word in the m_pyScript with the passed string checking
*  if the value will fail validation (errors are stored m_fails)
* @param pythonMark the word to search for and replace
* @param setting textual representation of the value
* @param check the property that will be used to for validity checking
*/
std::string pythonCalc::replaceErrsFind(QString pythonMark, const QString &setting, Property * const check)
{
  m_pyScript.replace(pythonMark, "'"+setting+"'");
  return check->setValue(setting.toStdString());
}
/** Appends the text in the QLineEdit to m_pyScript and stores any error
*   in m_fails
* @param userVal points to the QLineEdit containing the user value
* @param check the property that will be used to for validity checking
*/
void pythonCalc::appendChk(const QLineEdit * const userVal, Property * const check)
{
  m_pyScript.append("'"+userVal->text()+"'");
  std::string error = check->setValue(userVal->text().toStdString());
  if ( ! error.empty() )
  {
    m_fails[userVal] = error;
  }
}
/** Sends interpretes the Python code through runPythonCode() and returns
*  a results summary
*  @return any print statements executed in the Python script
*/
QString pythonCalc::run()
{
 QString tests = runPythonCode(m_pyScript, false);
 //std::cerr << "results\n";
 //std::cerr << tests.toStdString() << "\n";
 return tests;
}
