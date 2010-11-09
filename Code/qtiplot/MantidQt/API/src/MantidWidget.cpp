#include "MantidQtAPI/MantidWidget.h"
#include <QTemporaryFile>
#include <QTextStream>
#include <QDir>

using namespace MantidQt::API;

/**
* Default constructor
* @param parent The parent widget
*/
MantidWidget::MantidWidget(QWidget *parent) : QWidget(parent)
{
}

/** Run a piece of python code and return any output that it writes to stdout
*  @param code the Python commands to execute
*  @param no_output if set to true this method returns an empty string, if false it returns the output from any Python print statements
*  @return output from Python print statements unless no_output is false
*/
QString MantidWidget::runPythonCode(const QString & code, bool no_output)
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
    throw std::runtime_error("An error occurred opening a temporary file in " + QDir::tempPath().toStdString());
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
