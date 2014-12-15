#ifndef MANTIDQTAPI_PYTHONRUNNER_H_
#define MANTIDQTAPI_PYTHONRUNNER_H_

#include "DllOption.h"
#include <QObject>
#include <QStringList>

/** The ase class from which mantid custom widgets are derived it contains
*  some useful functions
*/
namespace MantidQt
{
  namespace API
  {
    /** 
    This is the base class all customised user interfaces that do not wish to be tied
    to a specific Mantid algorithm but rather customised for user's requirements

    @author Steve Williams, Rutherford Appleton Laboratory
    @date 10/11/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
    */
    class EXPORT_OPT_MANTIDQT_API PythonRunner : public QObject
    {
    Q_OBJECT

    public:
      /// Default constructor
      PythonRunner() : QObject(){}

      /// Run python code
      QString runPythonCode(const QString & code, bool no_output = false);
      /// Converts a list of strings into a string recognised by Python as a tuple
      static const QString stringList2Tuple(const QStringList & list);
    signals:
      void runAsPythonScript(const QString& code, bool);
    };
  }
}

#endif //MANTIDQTAPI_PYTHONRUNNER_H_
