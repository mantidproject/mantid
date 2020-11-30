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

#include <QDialog>

#include <stdexcept>

using namespace Mantid::API;

namespace MantidQt {
namespace MantidWidgets {

using FittingType = FitOptionsBrowser::FittingType;

FitScriptGeneratorView::FitScriptGeneratorView(
    QWidget *parent, QMap<QString, QString> const &fitOptions)
    : API::MantidWidget(parent), m_presenter(),
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

void FitScriptGeneratorView::openAddWorkspaceDialog() {
  auto dialog = QDialog();
  dialog.exec();

  if (static_cast<QDialog::DialogCode>(dialog.result()) ==
      QDialog::DialogCode::Accepted)
    return;
  // return dialog.getWorkspaceData();
  return;
}

} // namespace MantidWidgets
} // namespace MantidQt
