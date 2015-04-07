//---------------------------------------
// Includes
//---------------------------------------

#include "MantidQtMantidWidgets/SelectWorkspacesDialog.h"

#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <set>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt
{
namespace MantidWidgets
{

  /**
  Helper comparitor class used to determine if a workspace is not of a given type.
  */
  class WorkspaceIsNotOfType
  {
  private:
    const std::string m_type;
    const bool m_isMatrixWorkspace;
  public:
    WorkspaceIsNotOfType(const std::string &type) : 
    m_type(type),
    m_isMatrixWorkspace(type == "MatrixWorkspace")
    {
    }
    bool operator()(Mantid::API::Workspace_sptr ws) const
    {
      if ( m_type.empty() ) return false;
      if ( m_isMatrixWorkspace )
      {
        return dynamic_cast<Mantid::API::MatrixWorkspace*>( ws.get() ) == NULL;
      }
      return ws->id() != m_type;
    }
  };

//---------------------------------------
// Public member functions
//---------------------------------------

/** Constructor
@param parent : Parent widget
@param typeFilter : optional filter for filtering workspaces by type.
@param customButtonLabel : optional label for another custom button, return code for this is defined by CustomButton.
*/
SelectWorkspacesDialog::SelectWorkspacesDialog(QWidget* parent, const std::string& typeFilter,
                                               const std::string& customButtonLabel) :
QDialog(parent)
{
  setWindowTitle("MantidPlot - Select workspace");
  m_wsList = new QListWidget(parent);

  
  Mantid::API::AnalysisDataServiceImpl& ADS = Mantid::API::AnalysisDataService::Instance();
  typedef std::vector<Mantid::API::Workspace_sptr> VecWorkspaces;
  VecWorkspaces workspaces = ADS.getObjects();
  WorkspaceIsNotOfType comparitor(typeFilter);
  workspaces.erase(std::remove_if(workspaces.begin(), workspaces.end(), comparitor), workspaces.end() );
  QStringList tmp;
  for (VecWorkspaces::const_iterator it = workspaces.begin(); it != workspaces.end(); ++it)
  {
    //if(useFilter && ADS::
    tmp<<QString::fromStdString((*it)->name());
  }

  m_wsList->addItems(tmp);
  m_wsList->setSelectionMode(QAbstractItemView::MultiSelection);

  QDialogButtonBox* btnBox = new QDialogButtonBox(Qt::Horizontal);

  if (!customButtonLabel.empty()) {
    m_customButton = new QPushButton(QString::fromStdString(customButtonLabel));
    btnBox->addButton(m_customButton,QDialogButtonBox::DestructiveRole);
    connect(m_customButton, SIGNAL(clicked()), this, SLOT(customButtonPress()));
  }

  m_okButton = new QPushButton("Select");
  QPushButton* cancelButton = new QPushButton("Cancel");
  btnBox->addButton(m_okButton,QDialogButtonBox::AcceptRole);
  btnBox->addButton(cancelButton,QDialogButtonBox::RejectRole);
  connect(btnBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(btnBox, SIGNAL(rejected()), this, SLOT(reject()));

  QVBoxLayout* vLayout = new QVBoxLayout();
  vLayout->addWidget(m_wsList);
  vLayout->addWidget(btnBox);

  setLayout(vLayout);

  connect(m_wsList,SIGNAL(itemSelectionChanged()),this,SLOT(selectionChanged()));

  selectionChanged();

}

QStringList SelectWorkspacesDialog::getSelectedNames()const
{
  QList<QListWidgetItem *> items = m_wsList->selectedItems();
  QStringList res;
  foreach(QListWidgetItem* item,items)
  {
    res << item->text();
  }
  return res;
}

/// Slot to monitor the workspace selection status
void SelectWorkspacesDialog::selectionChanged()
{
  m_okButton->setEnabled( m_wsList->selectionModel()->hasSelection() );
}

/// slot to handle the custom button press
void SelectWorkspacesDialog::customButtonPress()
{
  this->done(CustomButton);
}

}
}
