#ifndef MANTIDQT_CUSTOM_DIALOGS_FITDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGS_FITDIALOG_H

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtAPI/AlgorithmDialog.h"
#include "ui_FitDialog.h"

//------------------------------------------------------------------------------
// Qt Forward declarations
//------------------------------------------------------------------------------
class QVBoxLayout;
class QSpinBox;

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

    //------------------------------------------------------------------------------
    // Local Forward declarations
    //------------------------------------------------------------------------------
    class InputWorkspaceWidget;
    class DynamicPropertiesWidget;

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
      
      File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
      Code Documentation is available at: <http://doxygen.mantidproject.org>    
    */
    class FitDialog : public API::AlgorithmDialog
    {
      Q_OBJECT
      
    public:
      /// Default constructor
      FitDialog(QWidget *parent = NULL);

    private slots:
      /// Override the help button clicked method
      //void helpClicked();
      void workspaceChanged(const QString&);
      void functionChanged();

    private:
      /// Initialize the layout
      void initLayout();
      /// Save the input history
      void saveInput();
      virtual void parseInput();
      /// Tie static widgets to their properties
      void tieStaticWidgets(const bool readHistory);
      /// Create InputWorkspaceWidgets and populate the tabs of the tab widget
      void createInputWorkspaceWidgets();


      /// Clears all of the widgets from the old layout
      void removeOldInputWidgets();
      /// Create
      void createDynamicLayout();
      /// Return property value stored in history
      QString getStoredPropertyValue(const QString& propName) const;
      /// Get allowed values for a property
      QStringList getAllowedPropertyValues(const QString& propName) const;

      /// Is the function MD?
      bool isMD() const;

    private:
      /// Form
      Ui::FitDialog m_form;
      QList<QWidget*> m_tabs;

      friend class InputWorkspaceWidget;
    };

    /**
     * Widget for inputting workspace information. 
     */
    class InputWorkspaceWidget: public QWidget
    {
      Q_OBJECT
    public:
      /// Constructor
      InputWorkspaceWidget(FitDialog* parent, int domainIndex = 0);
      /// Return property value stored in history
      QString getStoredPropertyValue(const QString& propName) const
      {return m_fitDialog->getStoredPropertyValue(propName);}
      /// Get allowed values for a property
      QStringList getAllowedPropertyValues(const QString& propName) const
      {return m_fitDialog->getAllowedPropertyValues(propName);}
      /// Get workspace name
      QString getWorkspaceName() const ;
      /// Return the domain index
      int getDomainIndex() const {return m_domainIndex;} 
      /// Set a property
      void setPropertyValue(const QString& propName, const QString& propValue);
      /// Set all workspace properties
      void setProperties();
    protected slots:
      /// Set the dynamic properties
      void setDynamicProperties();
    protected:
      /// Is ws name set?
      bool isWSNameSet() const;
      /// Is the workspace MW?
      bool isMatrixWorkspace() const;
      /// Is the workspace MD?
      bool isMDWorkspace() const;
      /// is current workspace supported by Fit?
      bool isWorkspaceSupported() const;

      /// Parent FitDialog
      FitDialog *m_fitDialog;
      /// In multidomain fitting it is index of domain created from this workspace
      /// In single domain case == 0
      int m_domainIndex;
      /// Name of the property for the input workspace
      QString m_wsPropName;
      /// Workspace name widget
      QComboBox *m_workspaceName;
      /// Dynamic propeties widget
      DynamicPropertiesWidget *m_dynamicProperties;

      /// The main layout
      QVBoxLayout *m_layout;
    };

    /**
     * Base class for input workspace's dynamic properties widget
     */
    class DynamicPropertiesWidget: public QWidget
    {
    public:
      /// Constructor
      DynamicPropertiesWidget(InputWorkspaceWidget* parent):QWidget(parent),m_wsWidget(parent){}
      /// Initialize the child widgets with stored and allowed values
      virtual void init() = 0;
      /// Set all workspace properties
      virtual void setProperties() = 0;
    protected:
      /// Parent InputWorkspaceWidget
      InputWorkspaceWidget *m_wsWidget;
    };

    /**
     * Widgets to set properties for a MatrixWorkspace: WorkspaceIndex, StartX, EndX
     */
    class MWPropertiesWidget: public DynamicPropertiesWidget
    {
    public:
      MWPropertiesWidget(InputWorkspaceWidget* parent);
      /// Initialize the child widgets with stored and allowed values
      virtual void init();
      /// Set all workspace properties
      void setProperties();
    protected:
      QSpinBox *m_workspaceIndex;
      QLineEdit *m_startX;
      QLineEdit *m_endX;
    };

  }
}

#endif //MANTIDQT_CUSTOM_DIALOGS_FITDIALOG_H
