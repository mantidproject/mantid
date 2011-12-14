//----------------------
// Includes
//----------------------

#include "MantidQtCustomInterfaces/CreateMDWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"
#include "DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
  #pragma warning enable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic pop
  #endif
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
CreateMDWorkspace::CreateMDWorkspace(QWidget *) //: m_data(new WorkspaceMementoCollection), m_memento(NULL)
{
  //Generate memento view model.
  //m_model = new QtWorkspaceMementoModel(m_data->getWorkingData());
}

/*
Initalize the layout.
*/
void CreateMDWorkspace::initLayout()
{
  m_uiForm.setupUi(this);
 /* connect(m_uiForm.btn_revert, SIGNAL(clicked()), this, SLOT(revertClicked()));
  connect(m_uiForm.btn_create, SIGNAL(clicked()), this, SLOT(createMDWorkspaceClicked()));
  connect(m_uiForm.btn_apply_all, SIGNAL(clicked()), this, SLOT(applyToAllClicked()));
  connect(m_uiForm.btn_add_workspace, SIGNAL(clicked()), this, SLOT(addWorkspaceClicked()));
  connect(m_uiForm.btn_remove_workspace, SIGNAL(clicked()), this, SLOT(removeWorkspaceClicked()));*/
  //Set MVC Model
  //m_uiForm.tableView->setModel(m_model);



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


/// Destructor
CreateMDWorkspace::~CreateMDWorkspace()
{
}

} //namespace CustomInterfaces
} //namespace MantidQt
