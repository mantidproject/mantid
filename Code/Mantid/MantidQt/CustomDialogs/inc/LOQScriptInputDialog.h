#ifndef MANTIDQT_CUSTOMDIALOGS_LOQSCRIPTINPUTDIALOG_H_
#define MANTIDQT_CUSTOMDIALOGS_LOQSCRIPTINPUTDIALOG_H_

//---------------------------
// Includes
//--------------------------
#include "MantidQtCustomDialogs/ui_LOQScriptInputDialog.h"
#include "MantidQtAPI/AlgorithmDialog.h"

namespace MantidQt
{
namespace CustomDialogs
{

/** 
    This class gives specialised dialog for the LOQ input algorithm.

    @author Martyn Gigg, Tessella Support Services plc
    @date 05/03/2009

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
class LOQScriptInputDialog : public MantidQt::API::AlgorithmDialog
{
  Q_OBJECT

  public:
  
  /// Default constructor
  LOQScriptInputDialog(QWidget *parent = 0);

  private:

  /// Initialize the layout
  virtual void initLayout();
  
  /// Get the input out of the dialog
  virtual void parseInput();

private slots:

  ///browse clicked method
  void browseClicked();

  private:
  // The form generated with Qt Designer
  Ui::LOQScriptInputDialog m_uiForm;
};

}
}

#endif //MANTIDQT_CUSTOMDIALOGS_LOQSCRIPTINPUTDIALOG_H_
