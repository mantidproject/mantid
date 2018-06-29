#include "ConvFitAddWorkspaceDialog.h"

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
  if (maximum)
    return QString("0-%1").arg(*maximum);
  return "";
}

QString getIndexString(const std::string &workspaceName) {
  return getIndexString(getWorkspace(workspaceName));
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

ConvFitAddWorkspaceDialog::ConvFitAddWorkspaceDialog(QWidget *parent)
    : IAddWorkspaceDialog(parent) {
  m_uiForm.setupUi(this);
  m_uiForm.dsWorkspace->showWorkspaceGroups(false);

  connect(m_uiForm.dsWorkspace, SIGNAL(dataReady(const QString &)), this,
          SLOT(workspaceChanged(const QString &)));
  connect(m_uiForm.ckAllSpectra, SIGNAL(stateChanged(int)), this,
          SLOT(selectAllSpectra(int)));
}

std::string ConvFitAddWorkspaceDialog::workspaceName() const {
  return m_uiForm.dsWorkspace->getCurrentDataName().toStdString();
}

std::string ConvFitAddWorkspaceDialog::resolutionName() const {
  return m_uiForm.dsResolution->getCurrentDataName().toStdString();
}

std::string ConvFitAddWorkspaceDialog::workspaceIndices() const {
  return m_uiForm.leWorkspaceIndices->text().toStdString();
}

void ConvFitAddWorkspaceDialog::setWSSuffices(const QStringList &suffices) {
  m_uiForm.dsWorkspace->setWSSuffixes(suffices);
}

void ConvFitAddWorkspaceDialog::setFBSuffices(const QStringList &suffices) {
  m_uiForm.dsWorkspace->setFBSuffixes(suffices);
}

void ConvFitAddWorkspaceDialog::setResolutionWSSuffices(
    const QStringList &suffices) {
  m_uiForm.dsResolution->setWSSuffixes(suffices);
}

void ConvFitAddWorkspaceDialog::setResolutionFBSuffices(
    const QStringList &suffices) {
  m_uiForm.dsResolution->setFBSuffixes(suffices);
}

void ConvFitAddWorkspaceDialog::selectAllSpectra(int state) {
  if (state == Qt::Checked) {
    m_uiForm.leWorkspaceIndices->setText(getIndexString(workspaceName()));
    m_uiForm.leWorkspaceIndices->setEnabled(false);
  } else
    m_uiForm.leWorkspaceIndices->setEnabled(true);
}

void ConvFitAddWorkspaceDialog::workspaceChanged(const QString &workspaceName) {
  const auto workspace = getWorkspace(workspaceName.toStdString());
  if (workspace && m_uiForm.ckAllSpectra->isChecked())
    m_uiForm.leWorkspaceIndices->setText(getIndexString(workspace));
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
