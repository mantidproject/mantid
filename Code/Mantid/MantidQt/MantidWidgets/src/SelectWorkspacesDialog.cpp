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

//---------------------------------------
// Public member functions
//---------------------------------------

/// Constructor
SelectWorkspacesDialog::SelectWorkspacesDialog(QDialog* parent) :
QDialog(parent)
{
  setWindowTitle("MantidPlot - Select workspace");
  m_wsList = new QListWidget(parent);

  QStringList tmp;
  std::set<std::string> sv = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  for (std::set<std::string>::const_iterator it = sv.begin(); it != sv.end(); ++it)
    tmp<<QString::fromStdString(*it);

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