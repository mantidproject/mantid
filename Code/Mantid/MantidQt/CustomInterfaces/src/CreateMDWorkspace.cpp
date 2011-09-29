//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/CreateMDWorkspace.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoCollection.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1125
#endif
#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"
#include "DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
  #pragma warning enable 1125
#endif

#include <QtCheckBoxFactory>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qmessagebox.h>

namespace MantidQt
{
namespace CustomInterfaces
{

//Add this class to the list of specialised dialogs in this namespace
//DECLARE_SUBWINDOW(CreateMDWorkspace); //TODO: Enable this to use it via mantid plot. Not ready for this yet!

/*
Constructor taking a WorkspaceMementoCollection, which acts as the model.
*/
CreateMDWorkspace::CreateMDWorkspace(QWidget *parent) : m_data(new WorkspaceMementoCollection)
{
  //Generate memento view model.
  m_model = new QtWorkspaceMementoModel(m_data->getWorkingData());
}

/*
Initalize the layout.
*/
void CreateMDWorkspace::initLayout()
{
  m_uiForm.setupUi(this);
  connect(m_uiForm.btn_revert, SIGNAL(clicked()), this, SLOT(revertClicked()));
  connect(m_uiForm.btn_create, SIGNAL(clicked()), this, SLOT(createMDWorkspaceClicked()));
  connect(m_uiForm.btn_apply_all, SIGNAL(clicked()), this, SLOT(applyToAllClicked()));
  connect(m_uiForm.btn_add_workspace, SIGNAL(clicked()), this, SLOT(addWorkspaceClicked()));
  connect(m_uiForm.btn_remove_workspace, SIGNAL(clicked()), this, SLOT(removeWorkspaceClicked()));
  //Set MVC Model
  m_uiForm.tableView->setModel(m_model);

}

void CreateMDWorkspace::initLocalPython()
{
}

/*
Run a generic confirmation dialog.
@param message : The message to display.
*/
int CreateMDWorkspace::runConfirmation(const std::string& message)
{
  QMessageBox msgBox;
  msgBox.setText(message.c_str());
  msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
  msgBox.setDefaultButton(QMessageBox::Cancel);
  return msgBox.exec();
}

/*
Handler for revert click event.
*/
void CreateMDWorkspace::revertClicked()
{
  int ret = runConfirmation("Are you sure that you wish to revert ALL Changes to ALL Workspaces?");
  if(QMessageBox::Ok == ret)
  {
    m_data->revertAll(m_model);
  }
}

/*
Handler for the apply to all clicked. Under these circumstances changes to all mementos will be overwritten by the selected.
*/
void CreateMDWorkspace::applyToAllClicked()
{
   int ret = runConfirmation("Are you sure that you wish to apply ALL changes to ALL Workspaces?");
   if(QMessageBox::Ok == ret)
   {
      m_data->applyAll(m_model);
   }

}

void CreateMDWorkspace::addWorkspaceClicked()
{
  std::string wsName = m_uiForm.workspaceSelector->currentText();
  if(!wsName.empty())
  {
    using namespace Mantid::API;
    Workspace_sptr ws = AnalysisDataService::Instance().retrieve(wsName);
    m_data->registerWorkspace(boost::dynamic_pointer_cast<MatrixWorkspace>(ws), m_model); //TODO better handle any incompatibility here.
  }
}

void CreateMDWorkspace::removeWorkspaceClicked()
{
  QModelIndex index = m_uiForm.tableView->currentIndex();
  std::string wsName = m_data->getWorkingData()->cell<std::string>(index.row(), 0);
  //Find the selected workspace names
  m_data->unregisterWorkspace(wsName, m_model);
}

void CreateMDWorkspace::createMDWorkspaceClicked()
{
  //Kick-off finalisation wizard. See mockups.

  //Must always update the model-view.
}

} //namespace CustomInterfaces
} //namespace MantidQt
