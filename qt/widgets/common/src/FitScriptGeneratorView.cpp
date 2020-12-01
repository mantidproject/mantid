// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorView.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorDataTable.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorPresenter.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

#include <QMessageBox>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {

void addWorkspacesFromGroup(std::vector<MatrixWorkspace_const_sptr> &workspaces,
                            WorkspaceGroup_const_sptr const &group) {
  auto const groupSize = static_cast<std::size_t>(group->getNumberOfEntries());
  for (auto i = 0u; i < groupSize; ++i) {
    if (auto const workspace =
            std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(i))) {
      workspaces.emplace_back(workspace);
    }
  }
}

std::vector<MatrixWorkspace_const_sptr>
getWorkspaces(std::string const &workspaceName) {
  auto &ads = AnalysisDataService::Instance();

  std::vector<MatrixWorkspace_const_sptr> workspaces;
  if (auto const workspace = ads.retrieveWS<MatrixWorkspace>(workspaceName))
    workspaces.emplace_back(workspace);
  else if (auto const group = ads.retrieveWS<WorkspaceGroup>(workspaceName))
    addWorkspacesFromGroup(workspaces, group);
  return workspaces;
}

std::vector<WorkspaceIndex>
convertToWorkspaceIndex(std::vector<int> const indices) {
  std::vector<WorkspaceIndex> workspaceIndices;
  workspaceIndices.reserve(indices.size());
  std::transform(indices.cbegin(), indices.cend(),
                 std::back_inserter(workspaceIndices),
                 [](int index) { return WorkspaceIndex(index); });
  return workspaceIndices;
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

using FittingType = FitOptionsBrowser::FittingType;

FitScriptGeneratorView::FitScriptGeneratorView(
    QWidget *parent, QMap<QString, QString> const &fitOptions)
    : API::MantidWidget(parent), m_presenter(), m_dialog(this),
      m_dataTable(std::make_unique<FitScriptGeneratorDataTable>()),
      m_functionBrowser(std::make_unique<FunctionBrowser>(nullptr, true)),
      m_fitOptionsBrowser(std::make_unique<FitOptionsBrowser>(
          nullptr, FittingType::SimultaneousAndSequential)) {
  m_ui.setupUi(this);

  m_ui.fDataTable->layout()->addWidget(m_dataTable.get());
  m_ui.splitter->addWidget(m_functionBrowser.get());
  m_ui.splitter->addWidget(m_fitOptionsBrowser.get());

  setFitBrowserOptions(fitOptions);
  connectUiSignals();
}

FitScriptGeneratorView::~FitScriptGeneratorView() {
  m_functionBrowser.reset();
  m_fitOptionsBrowser.reset();
}

void FitScriptGeneratorView::connectUiSignals() {
  connect(m_ui.pbRemove, SIGNAL(clicked()), this, SLOT(onRemoveClicked()));
  connect(m_ui.pbAddWorkspace, SIGNAL(clicked()), this,
          SLOT(onAddWorkspaceClicked()));
}

void FitScriptGeneratorView::setFitBrowserOptions(
    QMap<QString, QString> const &fitOptions) {
  for (auto it = fitOptions.constBegin(); it != fitOptions.constEnd(); ++it)
    setFitBrowserOption(it.key(), it.value());
}

void FitScriptGeneratorView::setFitBrowserOption(QString const &name,
                                                 QString const &value) {
  if (name == "FittingType")
    setFittingType(value);
  else
    m_fitOptionsBrowser->setProperty(name, value);
}

void FitScriptGeneratorView::setFittingType(QString const &fitType) {
  if (fitType == "Sequential")
    m_fitOptionsBrowser->setCurrentFittingType(FittingType::Sequential);
  else if (fitType == "Simultaneous")
    m_fitOptionsBrowser->setCurrentFittingType(FittingType::Simultaneous);
  else
    throw std::invalid_argument("Invalid fitting type '" +
                                fitType.toStdString() + "' provided.");
}

void FitScriptGeneratorView::subscribePresenter(
    FitScriptGeneratorPresenter *presenter) {
  m_presenter = presenter;
}

void FitScriptGeneratorView::onRemoveClicked() {
  m_presenter->notifyPresenter(ViewEvent::RemoveClicked);
}

void FitScriptGeneratorView::onAddWorkspaceClicked() {
  m_presenter->notifyPresenter(ViewEvent::AddClicked);
}

std::string FitScriptGeneratorView::workspaceName(FitDomainIndex index) const {
  return m_dataTable->workspaceName(index);
}

WorkspaceIndex
FitScriptGeneratorView::workspaceIndex(FitDomainIndex index) const {
  return m_dataTable->workspaceIndex(index);
}

double FitScriptGeneratorView::startX(FitDomainIndex index) const {
  return m_dataTable->startX(index);
}

double FitScriptGeneratorView::endX(FitDomainIndex index) const {
  return m_dataTable->endX(index);
}

std::vector<FitDomainIndex> FitScriptGeneratorView::selectedRows() const {
  return m_dataTable->selectedRows();
}

void FitScriptGeneratorView::removeWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) {
  m_dataTable->removeDomain(workspaceName, workspaceIndex);
}

void FitScriptGeneratorView::addWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    double startX, double endX) {
  m_dataTable->addDomain(QString::fromStdString(workspaceName), workspaceIndex,
                         startX, endX);
}

bool FitScriptGeneratorView::openAddWorkspaceDialog() {
  return m_dialog.exec() == QDialog::Accepted;
}

std::vector<MatrixWorkspace_const_sptr>
FitScriptGeneratorView::getDialogWorkspaces() {
  auto const workspaceName = m_dialog.workspaceName().trimmed().toStdString();

  std::vector<MatrixWorkspace_const_sptr> workspaces;
  if (AnalysisDataService::Instance().doesExist(workspaceName))
    workspaces = getWorkspaces(workspaceName);
  else
    displayWarning("Failed to add workspace '" + workspaceName +
                   "': workspace doesn't exist.");
  return workspaces;
}

std::vector<WorkspaceIndex>
FitScriptGeneratorView::getDialogWorkspaceIndices() const {
  return convertToWorkspaceIndex(m_dialog.workspaceIndices());
}

void FitScriptGeneratorView::displayWarning(std::string const &message) {
  QMessageBox::warning(this, "Warning!", QString::fromStdString(message));
}

} // namespace MantidWidgets
} // namespace MantidQt
