// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//---------------------------------------
// Includes
//---------------------------------------

#include "MantidQtWidgets/Common/SelectWorkspacesDialog.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <set>

namespace MantidQt::MantidWidgets {

/**
Helper comparitor class used to determine if a workspace is not of a given type.
*/
class WorkspaceIsNotOfType {
private:
  const std::string m_type;
  const bool m_isMatrixWorkspace;

public:
  explicit WorkspaceIsNotOfType(const std::string &type)
      : m_type(type), m_isMatrixWorkspace(type == "MatrixWorkspace") {}
  bool operator()(const Mantid::API::Workspace_sptr &ws) const {
    if (m_type.empty())
      return false;
    if (m_isMatrixWorkspace) {
      return dynamic_cast<Mantid::API::MatrixWorkspace *>(ws.get()) == nullptr;
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
@param customButtonLabel : optional label for another custom button, return code
for this is defined by CustomButton.
@param mode : optional selection mode for the list widget, default is MultiSelection
*/
SelectWorkspacesDialog::SelectWorkspacesDialog(QWidget *parent, const std::string &typeFilter,
                                               const std::string &customButtonLabel,
                                               QAbstractItemView::SelectionMode mode)
    : QDialog(parent), m_wsList(nullptr), m_okButton(nullptr), m_customButton(nullptr) {
  setWindowTitle("Mantid - Select workspace");
  m_wsList = new QListWidget(parent);

  const Mantid::API::AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();
  using VecWorkspaces = std::vector<Mantid::API::Workspace_sptr>;
  VecWorkspaces workspaces = ADS.getObjects();
  WorkspaceIsNotOfType comparitor(typeFilter);
  workspaces.erase(std::remove_if(workspaces.begin(), workspaces.end(), comparitor), workspaces.end());
  QStringList tmp;
  for (VecWorkspaces::const_iterator it = workspaces.begin(); it != workspaces.end(); ++it) {
    // if(useFilter && ADS::
    tmp << QString::fromStdString((*it)->getName());
  }

  m_wsList->addItems(tmp);
  m_wsList->setSelectionMode(mode);

  auto *btnBox = new QDialogButtonBox(Qt::Horizontal);

  if (!customButtonLabel.empty()) {
    m_customButton = new QPushButton(QString::fromStdString(customButtonLabel));
    btnBox->addButton(m_customButton, QDialogButtonBox::DestructiveRole);
    connect(m_customButton, SIGNAL(clicked()), this, SLOT(customButtonPress()));
  }

  m_okButton = new QPushButton("Select");
  auto *cancelButton = new QPushButton("Cancel");
  btnBox->addButton(m_okButton, QDialogButtonBox::AcceptRole);
  btnBox->addButton(cancelButton, QDialogButtonBox::RejectRole);
  connect(btnBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(btnBox, SIGNAL(rejected()), this, SLOT(reject()));

  auto *vLayout = new QVBoxLayout();
  vLayout->addWidget(m_wsList);
  vLayout->addWidget(btnBox);

  setLayout(vLayout);

  connect(m_wsList, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChanged()));

  selectionChanged();
}

QStringList SelectWorkspacesDialog::getSelectedNames() const {
  QList<QListWidgetItem *> items = m_wsList->selectedItems();
  QStringList res;
  foreach (QListWidgetItem *item, items) { res << item->text(); }
  return res;
}

/// Slot to monitor the workspace selection status
void SelectWorkspacesDialog::selectionChanged() { m_okButton->setEnabled(m_wsList->selectionModel()->hasSelection()); }

/// slot to handle the custom button press
void SelectWorkspacesDialog::customButtonPress() { this->done(CustomButton); }
} // namespace MantidQt::MantidWidgets
