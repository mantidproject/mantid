// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MDFAddWorkspaceDialog.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"

#include <QMessageBox>
#include <limits>

namespace MantidQt {
namespace CustomInterfaces {
namespace MDF {

using namespace Mantid::API;

namespace {
MatrixWorkspace_sptr getMatrixWorkspace(const QString &name) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      name.toStdString());
}
WorkspaceGroup_sptr getWorkspaceGroup(const QString &name) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
      name.toStdString());
}
} // namespace

/// Constructor.
/// @param parent :: A parent widget.
AddWorkspaceDialog::AddWorkspaceDialog(QWidget *parent)
    : QDialog(parent), m_maxIndex(0) {
  m_uiForm.setupUi(this);
  // populate the combo box with names of eligible workspaces
  QStringList workspaceNames = availableWorkspaces();
  connect(m_uiForm.cbWorkspaceName,
          SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(workspaceNameChanged(const QString &)));
  m_uiForm.cbWorkspaceName->addItems(workspaceNames);

  connect(m_uiForm.cbAllSpectra, SIGNAL(stateChanged(int)), this,
          SLOT(selectAllSpectra(int)));
}

/// Slot. Reacts on change of workspace name in the selection combo box.
/// @param wsName :: Name of newly selected workspace.
void AddWorkspaceDialog::workspaceNameChanged(const QString &wsName) {
  findCommonMaxIndex(wsName);
  auto text = m_maxIndex > 0 ? QString("0-%1").arg(m_maxIndex) : "0";
  if (m_uiForm.cbAllSpectra->isChecked() || m_maxIndex == 0) {
    m_uiForm.leWSIndices->setText(text);
  } else {
    m_uiForm.leWSIndices->clear();
    m_uiForm.leWSIndices->setPlaceholderText(text);
  }
}

/// Slot. Called when "All Spectra" check box changes its state
/// @param state :: The state of the check box (Qt::Checked or not).
void AddWorkspaceDialog::selectAllSpectra(int state) {
  if (state == Qt::Checked) {
    m_uiForm.leWSIndices->setText(QString("0-%1").arg(m_maxIndex));
    m_uiForm.leWSIndices->setEnabled(false);
  } else {
    m_uiForm.leWSIndices->setEnabled(true);
  }
}

QStringList AddWorkspaceDialog::availableWorkspaces() const {
  auto &ADS = Mantid::API::AnalysisDataService::Instance();
  QStringList workspaceNames;
  auto wsNames = ADS.getObjectNames(Mantid::Kernel::DataServiceSort::Sorted);
  for (auto &wsName : wsNames) {
    if (ADS.retrieveWS<Mantid::API::MatrixWorkspace>(wsName)) {
      workspaceNames << QString::fromStdString(wsName);
      continue;
    }
    auto grp = ADS.retrieveWS<Mantid::API::WorkspaceGroup>(wsName);
    if (grp) {
      bool hasMatrixWorkspace = false;
      for (auto ws : grp->getAllItems()) {
        if (dynamic_cast<Mantid::API::MatrixWorkspace *>(ws.get())) {
          hasMatrixWorkspace = true;
          break;
        }
      }
      if (hasMatrixWorkspace) {
        workspaceNames << QString::fromStdString(wsName);
      }
    }
  }
  return workspaceNames;
}

void AddWorkspaceDialog::findCommonMaxIndex(const QString &wsName) {
  m_maxIndex = 0;
  auto mws = getMatrixWorkspace(wsName);
  if (mws) {
    m_maxIndex = static_cast<int>(mws->getNumberHistograms()) - 1;
  } else {
    auto grp = getWorkspaceGroup(wsName);
    if (grp) {
      int maxIndex = std::numeric_limits<int>::max();
      for (auto ws : grp->getAllItems()) {
        auto mws = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
        if (mws) {
          maxIndex = std::min(maxIndex,
                              static_cast<int>(mws->getNumberHistograms()) - 1);
        }
      }
      m_maxIndex = maxIndex < std::numeric_limits<int>::max() ? maxIndex : 0;
    }
  }
  if (m_maxIndex < 0) {
    m_maxIndex = 0;
  }
}

/// Called on close if selection accepted.
void AddWorkspaceDialog::accept() {
  m_workspaceName = m_uiForm.cbWorkspaceName->currentText();
  m_wsIndices.clear();
  QString indexInput = m_uiForm.leWSIndices->text();
  if (!m_workspaceName.isEmpty() && !indexInput.isEmpty()) {
    auto validator =
        boost::make_shared<Mantid::Kernel::ArrayBoundedValidator<int>>(
            0, m_maxIndex);
    Mantid::Kernel::ArrayProperty<int> prop("Indices", validator);
    std::string err = prop.setValue(indexInput.toStdString());
    if (err.empty()) {
      m_wsIndices = prop;
    } else {
      QMessageBox::warning(
          this, "MantidPlot - Error",
          QString("Some of the indices are outside the allowed range [0,%1]")
              .arg(m_maxIndex));
    }
  }
  if (m_wsIndices.empty()) {
    QMessageBox::warning(this, "MantidPlot - Warning",
                         QString("No indices have been selected."));
    return;
  }
  QDialog::accept();
}

/// Called on close if selection rejected.
void AddWorkspaceDialog::reject() {
  m_workspaceName.clear();
  m_wsIndices.clear();
  QDialog::reject();
}

} // namespace MDF
} // namespace CustomInterfaces
} // namespace MantidQt
