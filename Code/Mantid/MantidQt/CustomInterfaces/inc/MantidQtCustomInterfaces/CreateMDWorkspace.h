#ifndef MANTIDQTCUSTOMINTERFACES_CREATEMDWORKSPACE_H_
#define MANTIDQTCUSTOMINTERFACES_CREATEMDWORKSPACE_H_

//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/QtWorkspaceMementoModel.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "ui_CreateMDWorkspace.h"
#include "MantidQtAPI/UserSubWindow.h"

#include <Poco/NObserver.h>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "MantidKernel/ConfigService.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    //Forward declaration
    class CreateMDWorkspace : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

    public:
      /// The name of the interface as registered into the factory
      static std::string name() { return "Create MD Workspace"; }
      // This interface's categories.
      static QString categoryInfo() { return "Indirect"; }
      /// Default Constructor
      CreateMDWorkspace(QWidget *parent = 0);
      /// Destructor
      ~CreateMDWorkspace();

    private:
      /// Initialize the layout
      virtual void initLayout();
      /// init python-dependent sections
      virtual void initLocalPython();
      /// Run a confirmation dialog.
      int runConfirmation(const std::string& message);
      /// Checks the candidate is unique, then adds it to the existing data.
      void addUniqueMemento(WorkspaceMemento_sptr candidate);
      /// Get the first selected memento.
      WorkspaceMemento_sptr getFirstSelected();
      /// Find files of a certain type
      QStringList findFiles(const std::string fileType) const;

    private slots:

      void addWorkspaceClicked();

      void addNexusFileClicked();

      void addEventNexusFileClicked();

      void setUBMatrixClicked();

      void removeSelectedClicked();

      void findUBMatrixClicked();

      void createMDWorkspaceClicked();

      void setGoniometerClicked();

      void mergeClicked(bool);

      void setLogValueClicked();

      void helpClicked();

      void setLocationClicked();

    private:
      /// UI form
      Ui::CreateMDWorkspace  m_uiForm;
      /// Collection of all mementos
      WorkspaceMementoCollection m_data;
      /// Current memento
      WorkspaceMemento_sptr m_current;
      /// QT model for MVC table view
      QtWorkspaceMementoModel* m_model;
    };
  }
}
#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTANALYSIS_H_ */
