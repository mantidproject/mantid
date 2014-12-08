#ifndef MANTIDQT_CUSTOM_DIALOGS_LOADDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGS_LOADDIALOG_H

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtAPI/AlgorithmDialog.h"
#include "ui_LoadDialog.h"

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
      This class gives specialised dialog for the Load algorithm. It requires that the specific 
      load algorithm has at least 2 properties with these names:

      <UL>
      <LI>Filename - A text property containing the filename </LI>
      <LI>OutputWorkspace - A text property containing the name of the OutputWorkspace </LI>
      </UL>

      There is no UI form as the most of the thing is dynamic.

      @author Martyn Gigg, Tessella plc
      @date 31/01/2011
      
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
    class LoadDialog : public API::AlgorithmDialog
    {
      Q_OBJECT
      
    public:
      /// Default constructor
      LoadDialog(QWidget *parent = NULL);

    private slots:
      /// Create the widgets and layouts that are dynamic, i.e they depend on 
      /// the specific load algorithm
      void createDynamicWidgets();
      /// Override the help button clicked method
      void helpClicked();
      /// Suggest a workspace name from the file
      void suggestWSName();
      /// Connect/Disconnect the signal that updates the workspace name with a suggested value
      void enableNameSuggestion(const bool on = false);
      /// Override accept() slot
      void accept();

    private:
      /// Initialize the layout
      void initLayout();
      /// Save the input history
      void saveInput();
      /// Tie static widgets to their properties
      void tieStaticWidgets(const bool readHistory);
      /// Clears all of the widgets from the old layout
      void removeOldInputWidgets(QVBoxLayout *layout);
      /// Create
      void createDynamicLayout();
      /// Create the widgets for a given property
      int createWidgetsForProperty(const Mantid::Kernel::Property* prop,
					                         QVBoxLayout *propertyLayout, QWidget *parent);

    private:
      /// Form
      Ui::LoadDialog m_form;
      /// The current file
      QString m_currentFiles;
      /// The initial height
      int m_initialHeight;
      /// Flag to indicating if we are populating the dialog
      bool m_populating;
    };

  }
}

#endif //MANTIDQT_CUSTOM_DIALOGS_LOADDIALOG_H
