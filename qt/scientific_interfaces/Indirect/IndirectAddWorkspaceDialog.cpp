// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectAddWorkspaceDialog.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <boost/optional.hpp>

namespace {
using namespace Mantid::API;

MatrixWorkspace_sptr getWorkspace(const std::string &name) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
}

bool doesExistInADS(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

bool validWorkspace(std::string const &name) {
  return !name.empty() && doesExistInADS(name);
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

std::unique_ptr<QRegExpValidator> createValidator(const QString &regex,
                                                  QObject *parent) {
  return Mantid::Kernel::make_unique<QRegExpValidator>(QRegExp(regex), parent);
}

QString OR(const QString &lhs, const QString &rhs) {
  return "(" + lhs + "|" + rhs + ")";
}

QString NATURAL_NUMBER(std::size_t digits) {
  return OR("0", "[1-9][0-9]{," + QString::number(digits - 1) + "}");
}

const QString EMPTY = "^$";
const QString SPACE = "(\\s)*";
const QString COMMA = SPACE + "," + SPACE;
const QString MINUS = "\\-";

const QString NUMBER = NATURAL_NUMBER(4);
const QString NATURAL_RANGE = "(" + NUMBER + MINUS + NUMBER + ")";
const QString NATURAL_OR_RANGE = OR(NATURAL_RANGE, NUMBER);
const QString SPECTRA_LIST =
    "(" + NATURAL_OR_RANGE + "(" + COMMA + NATURAL_OR_RANGE + ")*)";
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

AddWorkspaceDialog::AddWorkspaceDialog(QWidget *parent)
    : IAddWorkspaceDialog(parent) {
  m_uiForm.setupUi(this);
  m_uiForm.leWorkspaceIndices->setValidator(
      createValidator(SPECTRA_LIST, this).release());
  setAllSpectraSelectionEnabled(false);

  connect(m_uiForm.dsWorkspace, SIGNAL(dataReady(const QString &)), this,
          SLOT(workspaceChanged(const QString &)));
  connect(m_uiForm.ckAllSpectra, SIGNAL(stateChanged(int)), this,
          SLOT(selectAllSpectra(int)));
  connect(m_uiForm.pbAdd, SIGNAL(clicked()), this, SIGNAL(addData()));
  connect(m_uiForm.pbClose, SIGNAL(clicked()), this, SIGNAL(closeDialog()));
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
  auto const name = workspaceName();
  if (validWorkspace(name) && state == Qt::Checked) {
    m_uiForm.leWorkspaceIndices->setText(getIndexString(name));
    m_uiForm.leWorkspaceIndices->setEnabled(false);
  } else
    m_uiForm.leWorkspaceIndices->setEnabled(true);
}

void AddWorkspaceDialog::workspaceChanged(const QString &workspaceName) {
  const auto name = workspaceName.toStdString();
  const auto workspace = getWorkspace(name);
  if (workspace)
    setWorkspace(name);
  else
    setAllSpectraSelectionEnabled(false);
}

void AddWorkspaceDialog::setWorkspace(const std::string &workspace) {
  setAllSpectraSelectionEnabled(true);
  if (m_uiForm.ckAllSpectra->isChecked()) {
    m_uiForm.leWorkspaceIndices->setText(getIndexString(workspace));
    m_uiForm.leWorkspaceIndices->setEnabled(false);
  }
}

void AddWorkspaceDialog::setAllSpectraSelectionEnabled(bool doEnable) {
  m_uiForm.ckAllSpectra->setEnabled(doEnable);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
