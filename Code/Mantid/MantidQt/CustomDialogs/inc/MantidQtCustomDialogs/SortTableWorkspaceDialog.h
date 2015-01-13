#ifndef MANTIDQT_CUSTOM_DIALOGS_SORTTABLEWORKSPACEDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGS_SORTTABLEWORKSPACEDIALOG_H

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtAPI/AlgorithmDialog.h"
#include "ui_SortTableWorkspaceDialog.h"

#include <QMap>

namespace MantidQt
{

  namespace CustomDialogs
  {

    /** 
      This class gives specialised dialog for the SortTableWorkspace algorithm.

      @date 1/12/2014
      
      Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
      
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
    class SortTableWorkspaceDialog : public API::AlgorithmDialog
    {
      Q_OBJECT
      
    public:
      /// Default constructor
      SortTableWorkspaceDialog(QWidget *parent = NULL);

    private:
      /// Initialize the layout
      void initLayout();
      /// Pass input from non-standard GUI elements to the algorithm
      void parseInput();
      /// Tie static widgets to their properties
      void tieStaticWidgets(const bool readHistory);
    private slots:
      /// Update GUI after workspace changes
      void workspaceChanged(const QString& wsName);
      /// Add GUI elements to set a new column as a sorting key
      void addColumn();
      /// Sync the GUI after a sorting column name changes
      void changedColumnName(int);
      /// Remove a column to sort by.
      void removeColumn();
      /// Clear the GUI form the workspace specific data/elements
      void clearGUI();
    private:
      /// Form
      Ui::SortTableWorkspaceDialog m_form;
      /// Names of the columns in the workspace
      QStringList m_columnNames;
      /// Names of columns used to sort the table
      QStringList m_sortColumns;
    };

  } // CustomDialogs
} // MantidQt

#endif //MANTIDQT_CUSTOM_DIALOGS_SORTTABLEWORKSPACEDIALOG_H
