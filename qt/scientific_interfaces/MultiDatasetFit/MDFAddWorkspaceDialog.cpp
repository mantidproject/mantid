#include "MDFAddWorkspaceDialog.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"

#include <QMessageBox>

namespace MantidQt {
namespace CustomInterfaces {
namespace MDF {

/// Constructor.
/// @param parent :: A parent widget.
AddWorkspaceDialog::AddWorkspaceDialog(QWidget *parent)
    : QDialog(parent), m_maxIndex(0) {
  m_uiForm.setupUi(this);
  // populate the combo box with names of eligible workspaces
  QStringList workspaceNames;
  auto wsNames = Mantid::API::AnalysisDataService::Instance().getObjectNames(
      Mantid::Kernel::DataServiceSort::Sorted);
  for (auto name = wsNames.begin(); name != wsNames.end(); ++name) {
    auto mws = Mantid::API::AnalysisDataService::Instance()
                   .retrieveWS<Mantid::API::MatrixWorkspace>(*name);
    auto grp = Mantid::API::AnalysisDataService::Instance()
                   .retrieveWS<Mantid::API::WorkspaceGroup>(*name);
    if (mws || grp) {
      workspaceNames << QString::fromStdString(*name);
    }
  }
  workspaceNames.sort();
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
  auto stdWsName = wsName.toStdString();
  auto mws = Mantid::API::AnalysisDataService::Instance()
                 .retrieveWS<Mantid::API::MatrixWorkspace>(stdWsName);

  auto grp = Mantid::API::AnalysisDataService::Instance()
                 .retrieveWS<Mantid::API::WorkspaceGroup>(stdWsName);
  if (grp && !grp->isEmpty()) {
    mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        grp->getItem(0));
  }

  if (mws) {
    int maxValue = static_cast<int>(mws->getNumberHistograms()) - 1;
    if (maxValue < 0)
      maxValue = 0;
    m_maxIndex = maxValue;
    if (m_uiForm.cbAllSpectra->isChecked() || m_maxIndex == 0) {
      auto text = m_maxIndex > 0 ? QString("0-%1").arg(m_maxIndex) : "0";
      m_uiForm.leWSIndices->setText(text);
    } else {
      m_uiForm.leWSIndices->clear();
    }
  } else {
    m_maxIndex = 0;
    m_uiForm.leWSIndices->clear();
    m_uiForm.cbAllSpectra->setChecked(false);
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
