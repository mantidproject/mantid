#ifndef MANTIDQT_CUSTOM_DIALOGS_CONVERTTABLETOMATRIXWORKSPACEDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGS_CONVERTTABLETOMATRIXWORKSPACEDIALOG_H

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtAPI/AlgorithmDialog.h"
#include "ui_ConvertTableToMatrixWorkspaceDialog.h"

//------------------------------------------------------------------------------
// Qt Forward declarations
//------------------------------------------------------------------------------
class QVBoxLayout;

namespace MantidQt
{
  //------------------------------------------------------------------------------
  // Mantid Forward declarations
  //------------------------------------------------------------------------------
  namespace MantidWidgets
  {
    class MWRunFiles;
  }

  namespace CustomDialogs
  {

    /** 
      This class gives specialised dialog for the ConvertTableToMatrixWorkspace algorithm.

      @author Roman Tolchenov, Tessella plc
      @date 26/01/2012
      
      Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source
      
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
    class ConvertTableToMatrixWorkspaceDialog : public API::AlgorithmDialog
    {
      Q_OBJECT
      
    public:
      /// Default constructor
      ConvertTableToMatrixWorkspaceDialog(QWidget *parent = NULL);

    private slots:
      /// Update the column name widgets
      void fillColumnNames(const QString&);

    private:
      /// Initialize the layout
      void initLayout();

    private:
      /// Form
      Ui::ConvertTableToMatrixWorkspaceDialog m_form;
    };

  }
}

#endif //MANTIDQT_CUSTOM_DIALOGS_CONVERTTABLETOMATRIXWORKSPACEDIALOG_H
