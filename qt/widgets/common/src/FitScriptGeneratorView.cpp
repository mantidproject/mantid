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
#include <QClipboard>
#include <QMessageBox>

namespace {
using MantidQt::MantidWidgets::WorkspaceIndex;
std::vector<WorkspaceIndex> convertToWorkspaceIndex(std::vector<int> const &indices) {
  std::vector<WorkspaceIndex> workspaceIndices;
  workspaceIndices.reserve(indices.size());
  std::transform(indices.cbegin(), indices.cend(), std::back_inserter(workspaceIndices),
                 [](int index) { return WorkspaceIndex(index); });
  return workspaceIndices;
}

std::vector<std::string> convertToStdVector(QStringList const &qList) {
  std::vector<std::string> vec;
  vec.reserve(qList.size());
  std::transform(qList.cbegin(), qList.cend(), std::back_inserter(vec),
                 [](QString const &element) { return element.toStdString(); });
  return vec;
}

QStringList convertToQStringList(std::vector<MantidQt::MantidWidgets::GlobalParameter> const &vec) {
  QStringList qList;
  qList.reserve(static_cast<int>(vec.size()));
  std::transform(vec.cbegin(), vec.cend(), std::back_inserter(qList),
                 [](MantidQt::MantidWidgets::GlobalParameter const &element) {
                   return QString::fromStdString(element.m_parameter);
                 });
  return qList;
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

using ColumnIndex = FitScriptGeneratorDataTable::ColumnIndex;
using ViewEvent = IFitScriptGeneratorView::Event;

FitScriptGeneratorView::FitScriptGeneratorView(QWidget *parent, FittingMode fittingMode,
                                               QMap<QString, QString> const &fitOptions)
    : IFitScriptGeneratorView(parent), m_presenter(), m_dialog(std::make_unique<AddWorkspaceDialog>(this)),
      m_dataTable(std::make_unique<FitScriptGeneratorDataTable>()),
      m_functionTreeView(std::make_unique<FunctionTreeView>(nullptr, true)),
      m_fitOptionsBrowser(std::make_unique<BasicFitOptionsBrowser>(nullptr)) {
  m_ui.setupUi(this);

  m_ui.fDataTable->layout()->addWidget(m_dataTable.get());
  m_ui.splitterVertical->addWidget(m_functionTreeView.get());
  m_ui.splitterVertical->addWidget(m_fitOptionsBrowser.get());
  m_ui.splitterVertical->setSizes({360, 120});

  setFittingMode(fittingMode);
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
  connect(m_ui.pbAddWorkspace, SIGNAL(clicked()), this, SLOT(onAddWorkspaceClicked()));
  connect(m_dataTable.get(), SIGNAL(cellChanged(int, int)), this, SLOT(onCellChanged(int, int)));
  connect(m_dataTable.get(), SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelected()));

  connect(m_functionTreeView.get(), SIGNAL(functionRemovedString(QString const &)), this,
          SLOT(onFunctionRemoved(QString const &)));
  connect(m_functionTreeView.get(), SIGNAL(functionAdded(QString const &)), this,
          SLOT(onFunctionAdded(const QString &)));
  connect(m_functionTreeView.get(), SIGNAL(functionReplaced(QString const &)), this,
          SLOT(onFunctionReplaced(QString const &)));
  connect(m_functionTreeView.get(), SIGNAL(parameterChanged(QString const &)), this,
          SLOT(onParameterChanged(QString const &)));
  connect(m_functionTreeView.get(), SIGNAL(attributePropertyChanged(QString const &)), this,
          SLOT(onAttributeChanged(QString const &)));
  connect(m_functionTreeView.get(), SIGNAL(parameterTieChanged(QString const &, QString const &)), this,
          SLOT(onParameterTieChanged(QString const &, QString const &)));
  connect(m_functionTreeView.get(), SIGNAL(parameterConstraintRemoved(QString const &)), this,
          SLOT(onParameterConstraintRemoved(QString const &)));
  connect(m_functionTreeView.get(), SIGNAL(parameterConstraintAdded(QString const &, QString const &)), this,
          SLOT(onParameterConstraintChanged(QString const &, QString const &)));
  connect(m_functionTreeView.get(), SIGNAL(globalsChanged(QStringList const &)), this,
          SLOT(onGlobalParametersChanged(QStringList const &)));
  connect(m_functionTreeView.get(), SIGNAL(copyToClipboardRequest()), this, SLOT(onCopyFunctionToClipboard()));
  connect(m_functionTreeView.get(), SIGNAL(functionHelpRequest()), this, SLOT(onFunctionHelpRequested()));

  connect(m_fitOptionsBrowser.get(), SIGNAL(fittingModeChanged(FittingMode)), this,
          SLOT(onFittingModeChanged(FittingMode)));

  /// Disconnected because it causes a crash when selecting a table row while
  /// editing a parameters value. This is because selecting a different row will
  /// change the current function in the FunctionTreeView. The closeEditor slot
  /// is then called after this, but the memory location of the old function is
  /// now a nullptr so there is a read access violation.
  disconnect(m_functionTreeView->doubleEditorFactory(), SIGNAL(closeEditor()), m_functionTreeView->treeBrowser(),
             SLOT(closeEditor()));
}

void FitScriptGeneratorView::setFitBrowserOptions(QMap<QString, QString> const &fitOptions) {
  for (auto it = fitOptions.constBegin(); it != fitOptions.constEnd(); ++it)
    m_fitOptionsBrowser->setProperty(it.key().toStdString(), it.value().toStdString());
}

void FitScriptGeneratorView::setFittingMode(FittingMode fittingMode) {
  m_fitOptionsBrowser->setFittingMode(fittingMode);
}

void FitScriptGeneratorView::subscribePresenter(IFitScriptGeneratorPresenter *presenter) {
  m_presenter = presenter;
  m_presenter->notifyPresenter(ViewEvent::FittingModeChanged, m_fitOptionsBrowser->getFittingMode());
}

void FitScriptGeneratorView::onRemoveClicked() { m_presenter->notifyPresenter(ViewEvent::RemoveClicked); }

void FitScriptGeneratorView::onAddWorkspaceClicked() { m_presenter->notifyPresenter(ViewEvent::AddClicked); }

void FitScriptGeneratorView::onCellChanged(int row, int column) {
  UNUSED_ARG(row);
  m_dataTable->formatSelection();

  if (column == ColumnIndex::StartX)
    m_presenter->notifyPresenter(ViewEvent::StartXChanged);
  else if (column == ColumnIndex::EndX)
    m_presenter->notifyPresenter(ViewEvent::EndXChanged);
}

void FitScriptGeneratorView::onItemSelected() { m_presenter->notifyPresenter(ViewEvent::SelectionChanged); }

void FitScriptGeneratorView::onFunctionRemoved(QString const &function) {
  m_presenter->notifyPresenter(ViewEvent::FunctionRemoved, function.toStdString());
}

void FitScriptGeneratorView::onFunctionAdded(QString const &function) {
  m_presenter->notifyPresenter(ViewEvent::FunctionAdded, function.toStdString());
}

void FitScriptGeneratorView::onFunctionReplaced(QString const &function) {
  m_presenter->notifyPresenter(ViewEvent::FunctionReplaced, function.toStdString());
}

void FitScriptGeneratorView::onParameterChanged(QString const &parameter) {
  m_presenter->notifyPresenter(ViewEvent::ParameterChanged, parameter.toStdString());
}

void FitScriptGeneratorView::onAttributeChanged(QString const &attribute) {
  m_presenter->notifyPresenter(ViewEvent::AttributeChanged, attribute.toStdString());
}

void FitScriptGeneratorView::onParameterTieChanged(QString const &parameter, QString const &tie) {
  m_presenter->notifyPresenter(ViewEvent::ParameterTieChanged, parameter.toStdString(), tie.toStdString());
}

void FitScriptGeneratorView::onParameterConstraintRemoved(QString const &parameter) {
  m_presenter->notifyPresenter(ViewEvent::ParameterConstraintRemoved, parameter.toStdString());
}

void FitScriptGeneratorView::onParameterConstraintChanged(QString const &functionIndex, QString const &constraint) {
  m_presenter->notifyPresenter(ViewEvent::ParameterConstraintChanged, functionIndex.toStdString(),
                               constraint.toStdString());
}

void FitScriptGeneratorView::onGlobalParametersChanged(QStringList const &globalParameters) {
  m_presenter->notifyPresenter(ViewEvent::GlobalParametersChanged, convertToStdVector(globalParameters));
}

void FitScriptGeneratorView::onCopyFunctionToClipboard() {
  if (auto const function = m_functionTreeView->getSelectedFunction())
    QApplication::clipboard()->setText(QString::fromStdString(function->asString()));
}

void FitScriptGeneratorView::onFunctionHelpRequested() {
  if (auto const function = m_functionTreeView->getSelectedFunction())
    m_functionTreeView->showFunctionHelp(QString::fromStdString(function->name()));
}

void FitScriptGeneratorView::onFittingModeChanged(FittingMode fittingMode) {
  m_presenter->notifyPresenter(ViewEvent::FittingModeChanged, fittingMode);
}

std::string FitScriptGeneratorView::workspaceName(FitDomainIndex index) const {
  return m_dataTable->workspaceName(index);
}

WorkspaceIndex FitScriptGeneratorView::workspaceIndex(FitDomainIndex index) const {
  return m_dataTable->workspaceIndex(index);
}

double FitScriptGeneratorView::startX(FitDomainIndex index) const { return m_dataTable->startX(index); }

double FitScriptGeneratorView::endX(FitDomainIndex index) const { return m_dataTable->endX(index); }

std::vector<FitDomainIndex> FitScriptGeneratorView::allRows() const { return m_dataTable->allRows(); }

std::vector<FitDomainIndex> FitScriptGeneratorView::selectedRows() const { return m_dataTable->selectedRows(); }

FitDomainIndex FitScriptGeneratorView::currentRow() const { return m_dataTable->currentRow(); }

bool FitScriptGeneratorView::hasLoadedData() const { return m_dataTable->hasLoadedData(); }

double FitScriptGeneratorView::parameterValue(std::string const &parameter) const {
  return m_functionTreeView->getParameter(QString::fromStdString(parameter));
}

IFunction::Attribute FitScriptGeneratorView::attributeValue(std::string const &attribute) const {
  return m_functionTreeView->getAttribute(QString::fromStdString(attribute));
}

void FitScriptGeneratorView::removeWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex) {
  m_dataTable->removeDomain(workspaceName, workspaceIndex);
}

void FitScriptGeneratorView::addWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                                double startX, double endX) {
  m_dataTable->addDomain(QString::fromStdString(workspaceName), workspaceIndex, startX, endX);
}

bool FitScriptGeneratorView::openAddWorkspaceDialog() { return m_dialog->exec() == QDialog::Accepted; }

std::vector<MatrixWorkspace_const_sptr> FitScriptGeneratorView::getDialogWorkspaces() {
  auto const workspaces = m_dialog->getWorkspaces();
  if (workspaces.empty())
    displayWarning("Failed to add workspace: '" + m_dialog->workspaceName().toStdString() + "' doesn't exist.");
  return workspaces;
}

std::vector<WorkspaceIndex> FitScriptGeneratorView::getDialogWorkspaceIndices() const {
  return convertToWorkspaceIndex(m_dialog->workspaceIndices());
}

void FitScriptGeneratorView::resetSelection() { m_dataTable->resetSelection(); }

bool FitScriptGeneratorView::applyFunctionChangesToAll() const {
  return m_ui.cbApplyFunctionChangesTo->currentText() == "All Domains";
}

void FitScriptGeneratorView::clearFunction() { m_functionTreeView->clear(); }

void FitScriptGeneratorView::setSimultaneousMode(bool simultaneousMode) {
  m_dataTable->setFunctionPrefixVisible(simultaneousMode);
  m_functionTreeView->setMultiDomainFunctionPrefix(simultaneousMode ? m_dataTable->selectedDomainFunctionPrefix() : "");

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

void FitScriptGeneratorView::setGlobalTies(std::vector<GlobalTie> const &globalTies) {
  m_functionTreeView->setGlobalTies(globalTies);
}

void FitScriptGeneratorView::setGlobalParameters(std::vector<GlobalParameter> const &globalParameters) {
  m_functionTreeView->setGlobalParameters(convertToQStringList(globalParameters));
}

void FitScriptGeneratorView::displayWarning(std::string const &message) {
  QMessageBox::warning(this, "Warning!", QString::fromStdString(message));
}

} // namespace MantidWidgets
} // namespace MantidQt
