//---------------------------------------
// Includes
//---------------------------------------

#include "SelectWorkspacesDialog.h"
#include "../ApplicationWindow.h"
#include "MantidUI.h"

#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>

//---------------------------------------
// Public member functions
//---------------------------------------

/// Constructor
SelectWorkspacesDialog::SelectWorkspacesDialog(ApplicationWindow* appWindow) :
QDialog(appWindow)
{
  setWindowTitle("MantidPlot - Select workspace");
  m_wsList = new QListWidget(appWindow);
  m_wsList->addItems(appWindow->mantidUI->getWorkspaceNames());
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
