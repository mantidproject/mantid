#ifndef MANTIDQTAPI_MANTIDWIDGET_H_
#define MANTIDQTAPI_MANTIDWIDGET_H_

#include "DllOption.h"
#include "MantidQtAPI/PythonRunner.h"
#include <QWidget>
#include <QVariant>

namespace MantidQt
{
  namespace API
  {
    /** 
    This is the base class all customised widgets that do not wish to be tied
    to a specific Mantid algorithm but rather customised for user's requirements.
    
    The virtual function getUserInput() must be implemented to return what the widget considers
    as user input.

    @author Martyn Gigg, Tessella Support Services plc
    @date 18/03/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
    */
    class EXPORT_OPT_MANTIDQT_API MantidWidget : public QWidget
    {
    Q_OBJECT

    public:

    /// Returns a QVariant containing what the widget classes as user input so that
    /// input can be returned through a common interface.
    virtual QVariant getUserInput() const { return QVariant(); }
    /**
     * Sets a value on a mantid widget through a common interface
     * @param :: value The value as a QVariant
     */
    virtual void setUserInput(const QVariant & value)  { Q_UNUSED(value); }

    signals:
      void runAsPythonScript(const QString& code);

    protected:
      /// Default constructor
      MantidWidget(QWidget *parent = NULL);
      /// Run python code that is passed to it and, optionally, return 
      /// anything it wrote to standard output as a string
      QString runPythonCode(const QString & code, bool no_output = false);

    private:
      /// This object implements the runPythonCode() function, by emitting the code as a runAsPythonScript signal
      PythonRunner m_pyRunner;
    };
  }
}

#endif //MANTIDQTAPI_MANTIDWIDGET_H_
