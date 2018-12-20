#ifndef MANTIDQT_CUSTOMDIALOGS_LOQSCRIPTINPUTDIALOG_H_
#define MANTIDQT_CUSTOMDIALOGS_LOQSCRIPTINPUTDIALOG_H_

//---------------------------
// Includes
//--------------------------
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "ui_LOQScriptInputDialog.h"

namespace MantidQt {
namespace CustomDialogs {

/**
    This class gives specialised dialog for the LOQ input algorithm.

    @author Martyn Gigg, Tessella Support Services plc
    @date 05/03/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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
class LOQScriptInputDialog : public MantidQt::API::AlgorithmDialog {
  Q_OBJECT

public:
  /// Default constructor
  LOQScriptInputDialog(QWidget *parent = nullptr);

private:
  /// Initialize the layout
  void initLayout() override;

  /// Get the input out of the dialog
  void parseInput() override;

private slots:

  /// browse clicked method
  void browseClicked();

private:
  // The form generated with Qt Designer
  Ui::LOQScriptInputDialog m_uiForm;
};
} // namespace CustomDialogs
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMDIALOGS_LOQSCRIPTINPUTDIALOG_H_
