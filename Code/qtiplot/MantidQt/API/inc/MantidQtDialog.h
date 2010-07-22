#ifndef MANTIDQT_API_MANTIDQTDIALOG_H_
#define MANTIDQT_API_MANTIDQTDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"

#include <QDialog>

//----------------------------------
// Qt Forward declarations
//----------------------------------

//Top-level namespace for this library
namespace MantidQt
{

namespace API 
{

/** 
    Dialog derived from this class can capture and handle exceptions raised in 
    its event handlers. To be able to do this override QAplication::notify method:

        bool MyApplication::notify( QObject * receiver, QEvent * event )
        {
            bool res = false;
            try
            {
                res = QApplication::notify(receiver,event);
            }
            catch(std::exception& e)
            {
                if (MantidQt::API::MantidQtDialog::handle(receiver,e))
                    return true; // stops event propagation
                else
                   // do somethig else ... 
            }

            return res;
        }



    @author Roman Tolchenov, Tessella plc
    @date 24/04/2009

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
class EXPORT_OPT_MANTIDQT_API MantidQtDialog : public QDialog
{
  
  Q_OBJECT

public:

  /// DefaultConstructor
  MantidQtDialog(QWidget* parent = 0);
  /// Destructor
  virtual ~MantidQtDialog();

  /// Handles the exception caught in an event handler.
  static bool handle( QObject* receiver, const std::exception& e );

  // Override this method to handle an exception in a derived class.
  virtual void handleException( const std::exception& e );

};

}
}

#endif //MANTIDQT_API_MANTIDQTDIALOG_H_
