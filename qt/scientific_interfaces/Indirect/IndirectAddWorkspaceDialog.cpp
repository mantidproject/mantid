#include "IndirectAddWorkspaceDialog.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <boost/optional.hpp>

namespace {
using namespace Mantid::API;

MatrixWorkspace_sptr getWorkspace(const std::string &name) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
}

boost::optional<std::size_t> maximumIndex(MatrixWorkspace_sptr workspace) {
  if (workspace) {
    const auto numberOfHistograms = workspace->getNumberHistograms();
    if (numberOfHistograms > 0)
      return numberOfHistograms - 1;
  }
  return boost::none;
}

QString getIndexString(MatrixWorkspace_sptr workspace) {
  const auto maximum = maximumIndex(workspace);
  if (maximum) {
    if (*maximum > 0)
      return QString("0-%1").arg(*maximum);
    return "0";
  }
  return "";
}

QString getIndexString(const std::string &workspaceName) {
  return getIndexString(getWorkspace(workspaceName));
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

AddWorkspaceDialog::AddWorkspaceDialog(QWidget *parent)
    : IAddWorkspaceDialog(parent) {
  m_uiForm.setupUi(this);

  connect(m_uiForm.dsWorkspace, SIGNAL(dataReady(const QString &)), this,
          SLOT(workspaceChanged(const QString &)));
  connect(m_uiForm.ckAllSpectra, SIGNAL(stateChanged(int)), this,
          SLOT(selectAllSpectra(int)));
}

std::string AddWorkspaceDialog::workspaceName() const {
  return m_uiForm.dsWorkspace->getCurrentDataName().toStdString();
}

std::string AddWorkspaceDialog::workspaceIndices() const {
  return m_uiForm.leWorkspaceIndices->text().toStdString();
}

void AddWorkspaceDialog::setWSSuffices(const QStringList &suffices) {
  m_uiForm.dsWorkspace->setWSSuffixes(suffices);
}

void AddWorkspaceDialog::setFBSuffices(const QStringList &suffices) {
  m_uiForm.dsWorkspace->setFBSuffixes(suffices);
}

void AddWorkspaceDialog::selectAllSpectra(int state) {
  if (state == Qt::Checked) {
    m_uiForm.leWorkspaceIndices->setText(getIndexString(workspaceName()));
    m_uiForm.leWorkspaceIndices->setEnabled(false);
  } else
    m_uiForm.leWorkspaceIndices->setEnabled(true);
}

void AddWorkspaceDialog::workspaceChanged(const QString &workspaceName) {
  const auto workspace = getWorkspace(workspaceName.toStdString());
  if (workspace && m_uiForm.ckAllSpectra->isChecked())
    m_uiForm.leWorkspaceIndices->setText(getIndexString(workspace));
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
