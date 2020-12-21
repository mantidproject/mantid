// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorView.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorDataTable.h"
#include "MantidQtWidgets/Common/FittingGlobals.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorPresenter.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleDialogEditor.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

#include "MantidAPI/MatrixWorkspace.h"

#include <algorithm>
#include <iterator>

#include <QApplication>
#include <QClipBoard>
#include <QMessageBox>

namespace {
using MantidQt::MantidWidgets::WorkspaceIndex;
std::vector<WorkspaceIndex>
convertToWorkspaceIndex(std::vector<int> const &indices) {
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

using ColumnIndex = FitScriptGeneratorDataTable::ColumnIndex;
using ViewEvent = IFitScriptGeneratorView::Event;

FitScriptGeneratorView::FitScriptGeneratorView(
    QWidget *parent, QMap<QString, QString> const &fitOptions)
    : IFitScriptGeneratorView(parent), m_presenter(),
      m_dialog(std::make_unique<AddWorkspaceDialog>(this)),
      m_dataTable(std::make_unique<FitScriptGeneratorDataTable>()),
      m_functionTreeView(std::make_unique<FunctionTreeView>(nullptr, true)),
      m_fitOptionsBrowser(std::make_unique<FitOptionsBrowser>(
          nullptr, FittingMode::SimultaneousAndSequential)) {
  m_ui.setupUi(this);

  m_ui.fDataTable->layout()->addWidget(m_dataTable.get());
  m_ui.splitter->addWidget(m_functionTreeView.get());
  m_ui.splitter->addWidget(m_fitOptionsBrowser.get());

  setFitBrowserOptions(fitOptions);
  connectUiSignals();
}

FitScriptGeneratorView::~FitScriptGeneratorView() {
  m_dialog.reset();
  m_dataTable.reset();
  m_functionTreeView.reset();
  m_fitOptionsBrowser.reset();
}

void FitScriptGeneratorView::connectUiSignals() {
  connect(m_ui.pbRemove, SIGNAL(clicked()), this, SLOT(onRemoveClicked()));
  connect(m_ui.pbAddWorkspace, SIGNAL(clicked()), this,
          SLOT(onAddWorkspaceClicked()));
  connect(m_dataTable.get(), SIGNAL(cellChanged(int, int)), this,
          SLOT(onCellChanged(int, int)));
  connect(m_dataTable.get(), SIGNAL(itemPressed(QTableWidgetItem *)), this,
          SLOT(onItemPressed()));

  connect(m_functionTreeView.get(),
          SIGNAL(functionRemovedString(const QString &)), this,
          SLOT(onFunctionRemoved(const QString &)));
  connect(m_functionTreeView.get(), SIGNAL(functionAdded(const QString &)),
          this, SLOT(onFunctionAdded(const QString &)));
  connect(m_functionTreeView.get(), SIGNAL(functionReplaced(const QString &)),
          this, SLOT(onFunctionReplaced(const QString &)));
  connect(m_functionTreeView.get(), SIGNAL(parameterChanged(const QString &)),
          this, SLOT(onParameterChanged(const QString &)));
  connect(m_functionTreeView.get(),
          SIGNAL(attributePropertyChanged(const QString &)), this,
          SLOT(onAttributeChanged(const QString &)));
  connect(m_functionTreeView.get(),
          SIGNAL(parameterTieChanged(const QString &, const QString &)), this,
          SLOT(onParameterTieChanged(const QString &, const QString &)));
  connect(m_functionTreeView.get(), SIGNAL(copyToClipboardRequest()), this,
          SLOT(onCopyFunctionToClipboard()));
  connect(m_functionTreeView.get(), SIGNAL(functionHelpRequest()), this,
          SLOT(onFunctionHelpRequested()));

  connect(m_fitOptionsBrowser.get(), SIGNAL(changedToSequentialFitting()), this,
          SLOT(onChangeToSequentialFitting()));
  connect(m_fitOptionsBrowser.get(), SIGNAL(changedToSimultaneousFitting()),
          this, SLOT(onChangeToSimultaneousFitting()));

  /// Disconnected because it causes a crash when selecting a table row while
  /// editing a parameters value. This is because selecting a different row will
  /// change the current function in the FunctionTreeView. The closeEditor slot
  /// is then called after this, but the memory location of the old function is
  /// now a nullptr so there is a read access violation.
  disconnect(m_functionTreeView->doubleEditorFactory(), SIGNAL(closeEditor()),
             m_functionTreeView->treeBrowser(), SLOT(closeEditor()));
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
    m_fitOptionsBrowser->setCurrentFittingType(FittingMode::Sequential);
  else if (fitType == "Simultaneous")
    m_fitOptionsBrowser->setCurrentFittingType(FittingMode::Simultaneous);
  else
    throw std::invalid_argument("Invalid fitting type '" +
                                fitType.toStdString() + "' provided.");
}

void FitScriptGeneratorView::subscribePresenter(
    IFitScriptGeneratorPresenter *presenter) {
  m_presenter = presenter;
  m_presenter->notifyPresenter(ViewEvent::FittingModeChanged,
                               m_fitOptionsBrowser->getCurrentFittingType());
}

void FitScriptGeneratorView::onRemoveClicked() {
  m_presenter->notifyPresenter(ViewEvent::RemoveClicked);
}

void FitScriptGeneratorView::onAddWorkspaceClicked() {
  m_presenter->notifyPresenter(ViewEvent::AddClicked);
}

void FitScriptGeneratorView::onCellChanged(int row, int column) {
  UNUSED_ARG(row);
  m_dataTable->formatSelection();

  if (column == ColumnIndex::StartX)
    m_presenter->notifyPresenter(ViewEvent::StartXChanged);
  else if (column == ColumnIndex::EndX)
    m_presenter->notifyPresenter(ViewEvent::EndXChanged);
}

void FitScriptGeneratorView::onItemPressed() {
  m_presenter->notifyPresenter(ViewEvent::SelectionChanged);
}

void FitScriptGeneratorView::onFunctionRemoved(QString const &function) {
  m_presenter->notifyPresenter(ViewEvent::FunctionRemoved,
                               function.toStdString());
}

void FitScriptGeneratorView::onFunctionAdded(QString const &function) {
  m_presenter->notifyPresenter(ViewEvent::FunctionAdded,
                               function.toStdString());
}

void FitScriptGeneratorView::onFunctionReplaced(QString const &function) {
  m_presenter->notifyPresenter(ViewEvent::FunctionReplaced,
                               function.toStdString());
}

void FitScriptGeneratorView::onParameterChanged(QString const &parameter) {
  m_presenter->notifyPresenter(ViewEvent::ParameterChanged,
                               parameter.toStdString());
}

void FitScriptGeneratorView::onAttributeChanged(QString const &attribute) {
  m_presenter->notifyPresenter(ViewEvent::AttributeChanged,
                               attribute.toStdString());
}

void FitScriptGeneratorView::onParameterTieChanged(QString const &parameter,
                                                   QString const &tie) {
  m_presenter->notifyPresenter(ViewEvent::ParameterTieChanged,
                               parameter.toStdString(), tie.toStdString());
}

void FitScriptGeneratorView::onCopyFunctionToClipboard() {
  if (auto const function = m_functionTreeView->getSelectedFunction())
    QApplication::clipboard()->setText(
        QString::fromStdString(function->asString()));
}

void FitScriptGeneratorView::onFunctionHelpRequested() {
  if (auto const function = m_functionTreeView->getSelectedFunction())
    m_functionTreeView->showFunctionHelp(
        QString::fromStdString(function->name()));
}

void FitScriptGeneratorView::onChangeToSequentialFitting() {
  m_presenter->notifyPresenter(ViewEvent::FittingModeChanged,
                               FittingMode::Sequential);
}

void FitScriptGeneratorView::onChangeToSimultaneousFitting() {
  m_presenter->notifyPresenter(ViewEvent::FittingModeChanged,
                               FittingMode::Simultaneous);
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

std::vector<FitDomainIndex> FitScriptGeneratorView::allRows() const {
  return m_dataTable->allRows();
}

std::vector<FitDomainIndex> FitScriptGeneratorView::selectedRows() const {
  return m_dataTable->selectedRows();
}

double
FitScriptGeneratorView::parameterValue(std::string const &parameter) const {
  return m_functionTreeView->getParameter(QString::fromStdString(parameter));
}

IFunction::Attribute
FitScriptGeneratorView::attributeValue(std::string const &attribute) const {
  return m_functionTreeView->getAttribute(QString::fromStdString(attribute));
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
  return m_dialog->exec() == QDialog::Accepted;
}

std::vector<MatrixWorkspace_const_sptr>
FitScriptGeneratorView::getDialogWorkspaces() {
  auto const workspaces = m_dialog->getWorkspaces();
  if (workspaces.empty())
    displayWarning("Failed to add workspace: '" +
                   m_dialog->workspaceName().toStdString() +
                   "' doesn't exist.");
  return workspaces;
}

std::vector<WorkspaceIndex>
FitScriptGeneratorView::getDialogWorkspaceIndices() const {
  return convertToWorkspaceIndex(m_dialog->workspaceIndices());
}

void FitScriptGeneratorView::resetSelection() { m_dataTable->resetSelection(); }

bool FitScriptGeneratorView::isApplyFunctionChangesToAllChecked() const {
  return m_ui.ckApplyFunctionChangesToAll->isChecked();
}

void FitScriptGeneratorView::clearFunction() { m_functionTreeView->clear(); }

void FitScriptGeneratorView::setSimultaneousMode(bool simultaneousMode) {
  m_dataTable->setFunctionPrefixVisible(simultaneousMode);
  m_functionTreeView->setMultiDomainFunctionPrefix(
      simultaneousMode ? m_dataTable->selectedDomainFunctionPrefix() : "");

  if (simultaneousMode)
    m_functionTreeView->showGlobals();
  else
    m_functionTreeView->hideGlobals();
}

void FitScriptGeneratorView::setFunction(IFunction_sptr const &function) const {
  if (function)
    m_functionTreeView->setFunction(function);
  else
    m_functionTreeView->clear();
}

void FitScriptGeneratorView::setGlobalTies(
    std::vector<GlobalTie> const &globalTies) {
  m_functionTreeView->setGlobalTies(globalTies);
}

void FitScriptGeneratorView::displayWarning(std::string const &message) {
  QMessageBox::warning(this, "Warning!", QString::fromStdString(message));
}

} // namespace MantidWidgets
} // namespace MantidQt
