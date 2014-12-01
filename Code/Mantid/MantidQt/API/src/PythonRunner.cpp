#include "MantidQtAPI/PythonRunner.h"
#include "MantidKernel/Logger.h"

#include <QTemporaryFile>
#include <QDir>
#include <QTextStream>
#include <stdexcept>
#include <sstream>

using namespace MantidQt::API;

namespace
{
  Mantid::Kernel::Logger g_log("PythonRunner");

  /// Format of python warning message. It will be removed from any output string
  QString PYTHON_V1_WARN = "Warning: Python API v1 call has been made.\n";
}


/** Run a piece of python code and return any output that it writes to stdout
*  @param code :: the Python commands to execute
*  @param no_output :: if set to true this method returns an empty string, if false it returns the output from any Python print statements
*  @return output from Python print statements unless no_output is false
*/
QString PythonRunner::runPythonCode(const QString & code, bool no_output)
{
  using Mantid::Kernel::Logger;

  if(g_log.is(Logger::Priority::PRIO_DEBUG)) g_log.debug() << "Running Python code:\n" << code.toAscii().data() << "\n";

  if( no_output )
  {
    emit runAsPythonScript(code, true);
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
  QString code_to_run = "import sys; sys.stdout = open(\"" + tmpstring + "\", 'w');\n" + code;
  emit runAsPythonScript(code_to_run, true);

  //Now get the output
  tmp_file.open();
  QTextStream stream(&tmp_file);
  tmpstring.clear();

  while( !stream.atEnd() )
  {
    tmpstring.append(stream.readLine().trimmed() + "\n");
  }
  //FIXME: Ticket-9217 - Commented out for the moment to try and get working with clang
  if(g_log.is(Logger::Priority::PRIO_DEBUG)) g_log.debug() << "Raw output from execution:\n" << tmpstring.toAscii().data() << "\n";
  return tmpstring;
}
/** This Python helper function converts a list of strings into one
* string that Python will recognise as a Python tuple
* @param list string entries
* @return the strings as a comma separated list in brakets
*/
const QString PythonRunner::stringList2Tuple(const QStringList & list)
{
  QString tuple("(");
  QStringList::const_iterator end = list.end();
  for(QStringList::const_iterator it = list.begin(); it != end; ++it)
  {
    tuple += "'"+*it+"',";
  }
  tuple += ")";
  return tuple;
}
