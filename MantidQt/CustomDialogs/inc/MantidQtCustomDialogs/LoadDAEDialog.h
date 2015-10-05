#ifndef MANTIDQT_CUSTOM_DIALOGSLOADDAEDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGSLOADDAEDIALOG_H

//----------------------
// Includes
//----------------------
#include "MantidQtAPI/AlgorithmDialog.h"

//---------------------------
// Qt Forward declarations
//---------------------------
class QLabel;
class QLineEdit;
class QPushButton;
class QString;
class QVBoxLayout;
class QCheckBox;

namespace MantidQt
{
namespace CustomDialogs
{

/** 
    This class gives specialised dialog for the LoadDAE algorithm.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 14/07/2010

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class LoadDAEDialog : public MantidQt::API::AlgorithmDialog
{
  Q_OBJECT

public:

  /// Constructor
  LoadDAEDialog(QWidget *parent = 0);
  /// Destruktor
  ~LoadDAEDialog();

protected:
  /// This does the work and must be overridden in each deriving class
  void initLayout();

private:
  /* GUI components */
  QLineEdit *lineHost;
  QLineEdit *lineName;
  QLineEdit *minSpLineEdit;
  QLineEdit *maxSpLineEdit;
  QLineEdit *listSpLineEdit;
  QLineEdit *updateLineEdit;

};

}
}

#endif /* MANTIDQT_CUSTOM_DIALOGSLOADDAEDIALOG_H */
