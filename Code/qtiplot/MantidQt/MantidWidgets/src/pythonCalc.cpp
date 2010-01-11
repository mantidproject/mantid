#include "MantidQtMantidWidgets/pythonCalc.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/AlgorithmManager.h"
#include <QTextStream>
#include <QDir>
#include <cmath>
#include <QMessageBox>
#include <QTemporaryFile>

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
/** Sets m_pyScript to the contents of the named file
* @param pythonFile the name of the file to read
*/
void pythonCalc::readFile(const QString &pythonFile)
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
	  // strip some Python comments, might speed things up when there are multiple input files and these lines are repeated many times
	  if ( line.indexOf("#") == 0 ) continue;
	  // separate out the header because we might want to create a file where the body is repeated many times but there is one header
	  if ( line.indexOf("import ") == 0 || line.indexOf(" import ") != -1 )
	  {
		m_templateH.append(line + "\n");
	  }
	  else m_templateB.append(line + "\n");
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
/** Sends interpretes the Python code through runPythonCode() and returns
*  a results summary
*  @return some information about the results of the Python execution
*/
QString pythonCalc::run()
{
  return runPythonCode(m_pyScript, false);
}
//??STEVES?? this is a copy of what is in the interfaces class
QString pythonCalc::runPythonCode(const QString & code, bool no_output)
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
    QMessageBox::information(this, "", "An error occurred opening a temporary file in " + QDir::tempPath());
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