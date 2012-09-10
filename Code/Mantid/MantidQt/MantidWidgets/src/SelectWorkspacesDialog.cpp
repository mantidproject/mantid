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

namespace MantidQt
{
namespace MantidWidgets
{

  /**
  Helper comparitor class used to determine if a workspace is of a given type.
  */
  class WorkspaceIsOfType
  {
  private:
    const std::string m_type;
  public:
    WorkspaceIsOfType(const std::string type) : m_type(type)
    {
    }
    bool operator()(Mantid::API::Workspace_sptr ws) const
    {
      return ws->id() != m_type;
    }
  };

//---------------------------------------
// Public member functions
//---------------------------------------

/** Constructor
@param parent : Parent widget
@param typeFilter : optional filter for filtering workspaces by type.
*/
SelectWorkspacesDialog::SelectWorkspacesDialog(QWidget* parent, const std::string& typeFilter) :
QDialog(parent)
{
  setWindowTitle("MantidPlot - Select workspace");
  m_wsList = new QListWidget(parent);

  
  Mantid::API::AnalysisDataServiceImpl& ADS = Mantid::API::AnalysisDataService::Instance();
  typedef std::vector<Mantid::API::Workspace_sptr> VecWorkspaces;
  VecWorkspaces workspaces = ADS.getObjects();
  WorkspaceIsOfType comparitor(typeFilter);
  workspaces.erase(std::remove_if(workspaces.begin(), workspaces.end(), comparitor), workspaces.end() );
  QStringList tmp;
  for (VecWorkspaces::const_iterator it = workspaces.begin(); it != workspaces.end(); ++it)
  {
    //if(useFilter && ADS::
    tmp<<QString::fromStdString((*it)->name());
  }

  m_wsList->addItems(tmp);
  m_wsList->setSelectionMode(QAbstractItemView::MultiSelection);

  QPushButton* okButton = new QPushButton("Select");
  QPushButton* cancelButton = new QPushButton("Cancel");
  QDialogButtonBox* btnBox = new QDialogButtonBox(Qt::Horizontal);
  btnBox->addButton(okButton,QDialogButtonBox::AcceptRole);
  btnBox->addButton(cancelButton,QDialogButtonBox::RejectRole);
  connect(btnBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(btnBox, SIGNAL(rejected()), this, SLOT(reject()));

  QVBoxLayout* vLayout = new QVBoxLayout();
  vLayout->addWidget(m_wsList);
  vLayout->addWidget(btnBox);

  setLayout(vLayout);

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

}
}