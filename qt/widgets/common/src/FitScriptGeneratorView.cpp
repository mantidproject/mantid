// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorView.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorDataTable.h"
#include "MantidQtWidgets/Common/FittingGlobals.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorPresenter.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleDialogEditor.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"

#include <algorithm>
#include <iterator>

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QMetaObject>
#include <QThread>

using namespace Mantid::Kernel;
using namespace MantidQt::API;

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
  vec.reserve(static_cast<std::size_t>(qList.size()));
  std::transform(qList.cbegin(), qList.cend(), std::back_inserter(vec),
                 [](QString const &element) { return element.toStdString(); });
  return vec;
}

template <typename T> std::vector<T> convertQListToStdVector(QList<T> const &qList) {
  std::vector<T> vec;
  vec.reserve(static_cast<std::size_t>(qList.size()));
  std::transform(qList.cbegin(), qList.cend(), std::back_inserter(vec), [](T const &element) { return element; });
  return vec;
}

QString toQString(std::string const &str) { return QString::fromStdString(str); }

QString globalToQString(MantidQt::MantidWidgets::GlobalParameter const &global) {
  return toQString(global.m_parameter);
}

template <typename Function, typename T>
QStringList convertToQStringList(Function const &func, std::vector<T> const &vec) {
  QStringList qList;
  qList.reserve(static_cast<int>(vec.size()));
  std::transform(vec.cbegin(), vec.cend(), std::back_inserter(qList), func);
  return qList;
}

template <typename T> QList<T> convertToQList(std::vector<T> const &vec) {
  QList<T> qList;
  qList.reserve(static_cast<int>(vec.size()));
  std::transform(vec.cbegin(), vec.cend(), std::back_inserter(qList), [](T const &element) { return element; });
  return qList;
}

QString getDefaultScriptDirectory() {
  auto const previousDirectory = AlgorithmInputHistory::Instance().getPreviousDirectory();
  if (previousDirectory.isEmpty())
    return QString::fromStdString(ConfigService::Instance().getString("pythonscripts.directory"));
  return previousDirectory;
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

using ColumnIndex = FitScriptGeneratorDataTable::ColumnIndex;
using ViewEvent = IFitScriptGeneratorView::Event;

FitScriptGeneratorView::FitScriptGeneratorView(QWidget *parent, FittingMode fittingMode,
                                               QMap<QString, QString> const &fitOptions)
    : IFitScriptGeneratorView(parent), m_presenter(), m_dataTable(std::make_unique<FitScriptGeneratorDataTable>()),
      m_functionTreeView(std::make_unique<FunctionTreeView>(nullptr, true)),
      m_fitOptionsBrowser(std::make_unique<FitScriptOptionsBrowser>(nullptr)), m_editLocalParameterDialog(nullptr) {
  m_ui.setupUi(this);

  m_ui.fDataTable->layout()->addWidget(m_dataTable.get());
  m_ui.splitterVertical->addWidget(m_functionTreeView.get());
  m_ui.splitterVertical->addWidget(m_fitOptionsBrowser.get());
  m_ui.splitterVertical->setSizes({360, 120});

  setFittingMode(fittingMode);
  setFitBrowserOptions(fitOptions);
  connectUiSignals();

  observeDelete(true);
  observeClear(true);
  observeRename(true);
}

FitScriptGeneratorView::~FitScriptGeneratorView() {
  observeDelete(false);
  observeClear(false);
  observeRename(false);

  if (m_addWorkspaceDialog)
    closeAddWorkspaceDialog();

  m_dataTable.reset();
  m_functionTreeView.reset();
  m_fitOptionsBrowser.reset();
}

void FitScriptGeneratorView::connectUiSignals() {
  connect(m_ui.pbRemoveDomain, SIGNAL(clicked()), this, SLOT(onRemoveDomainClicked()));
  connect(m_ui.pbAddDomain, SIGNAL(clicked()), this, SLOT(onAddDomainClicked()));
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
  connect(m_functionTreeView.get(), SIGNAL(localParameterButtonClicked(QString const &)), this,
          SLOT(onEditLocalParameterClicked(QString const &)));

  connect(m_fitOptionsBrowser.get(), SIGNAL(outputBaseNameChanged(std::string const &)), this,
          SLOT(onOutputBaseNameChanged(std::string const &)));
  connect(m_fitOptionsBrowser.get(), SIGNAL(fittingModeChanged(FittingMode)), this,
          SLOT(onFittingModeChanged(FittingMode)));

  connect(m_ui.pbGenerateScriptToFile, SIGNAL(clicked()), this, SLOT(onGenerateScriptToFileClicked()));
  connect(m_ui.pbGenerateScriptToClipboard, SIGNAL(clicked()), this, SLOT(onGenerateScriptToClipboardClicked()));
  connect(m_ui.pbHelp, SIGNAL(clicked()), this, SLOT(onHelpClicked()));

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

void FitScriptGeneratorView::deleteHandle(std::string const &wsName, [[maybe_unused]] Workspace_sptr const &ws) {
  if (QThread::currentThread() != QApplication::instance()->thread()) {
    QMetaObject::invokeMethod(this, "notifyADSDeleteEvent", Qt::AutoConnection, Q_ARG(std::string const &, wsName));
  } else {
    notifyADSDeleteEvent(wsName);
  }
}

void FitScriptGeneratorView::clearHandle() {
  if (QThread::currentThread() != QApplication::instance()->thread()) {
    QMetaObject::invokeMethod(this, "notifyADSClearEvent", Qt::AutoConnection);
  } else {
    notifyADSClearEvent();
  }
}

void FitScriptGeneratorView::renameHandle(std::string const &wsName, std::string const &newName) {
  if (QThread::currentThread() != QApplication::instance()->thread()) {
    QMetaObject::invokeMethod(this, "notifyADSRenameEvent", Qt::AutoConnection, Q_ARG(std::string const &, wsName),
                              Q_ARG(std::string const &, newName));
  } else {
    notifyADSRenameEvent(wsName, newName);
  }
}

void FitScriptGeneratorView::notifyADSDeleteEvent(std::string const &workspaceName) {
  m_presenter->notifyPresenter(ViewEvent::ADSDeleteEvent, workspaceName);
}

void FitScriptGeneratorView::notifyADSClearEvent() { m_presenter->notifyPresenter(ViewEvent::ADSClearEvent); }

void FitScriptGeneratorView::notifyADSRenameEvent(std::string const &workspaceName, std::string const &newName) {
  m_presenter->notifyPresenter(ViewEvent::ADSRenameEvent, workspaceName, newName);
}

void FitScriptGeneratorView::onRemoveDomainClicked() { m_presenter->notifyPresenter(ViewEvent::RemoveDomainClicked); }

void FitScriptGeneratorView::onAddDomainClicked() { m_presenter->notifyPresenter(ViewEvent::AddDomainClicked); }

void FitScriptGeneratorView::onCellChanged(int row, int column) {
  UNUSED_ARG(row);

  if (column == ColumnIndex::StartX) {
    m_dataTable->formatSelection();
    m_presenter->notifyPresenter(ViewEvent::StartXChanged);
  } else if (column == ColumnIndex::EndX) {
    m_dataTable->formatSelection();
    m_presenter->notifyPresenter(ViewEvent::EndXChanged);
  }
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
    saveTextToClipboard(function->asString());
}

void FitScriptGeneratorView::onFunctionHelpRequested() {
  if (auto const function = m_functionTreeView->getSelectedFunction())
    m_functionTreeView->showFunctionHelp(QString::fromStdString(function->name()));
}

void FitScriptGeneratorView::onOutputBaseNameChanged(std::string const &outputBaseName) {
  m_presenter->notifyPresenter(ViewEvent::OutputBaseNameChanged, outputBaseName);
}

void FitScriptGeneratorView::onFittingModeChanged(FittingMode fittingMode) {
  m_presenter->notifyPresenter(ViewEvent::FittingModeChanged, fittingMode);
}

void FitScriptGeneratorView::onEditLocalParameterClicked(QString const &parameter) {
  m_presenter->notifyPresenter(ViewEvent::EditLocalParameterClicked, parameter.toStdString());
}

void FitScriptGeneratorView::onEditLocalParameterFinished(int result) {
  if (result == QDialog::Accepted)
    m_presenter->notifyPresenter(ViewEvent::EditLocalParameterFinished);

  m_editLocalParameterDialog = nullptr;
}

void FitScriptGeneratorView::onGenerateScriptToFileClicked() {
  m_presenter->notifyPresenter(ViewEvent::GenerateScriptToFileClicked);
}

void FitScriptGeneratorView::onGenerateScriptToClipboardClicked() {
  m_presenter->notifyPresenter(ViewEvent::GenerateScriptToClipboardClicked);
}

void FitScriptGeneratorView::onHelpClicked() {
  MantidQt::API::HelpWindow::showCustomInterface(this, "Fit Script Generator", QString("utility"));
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

void FitScriptGeneratorView::renameWorkspace(std::string const &workspaceName, std::string const &newName) {
  m_dataTable->renameWorkspace(QString::fromStdString(workspaceName), QString::fromStdString(newName));
}

void FitScriptGeneratorView::removeDomain(FitDomainIndex domainIndex) { m_dataTable->removeDomain(domainIndex); }

void FitScriptGeneratorView::addWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                                double startX, double endX) {
  m_dataTable->addDomain(QString::fromStdString(workspaceName), workspaceIndex, startX, endX);
}

void FitScriptGeneratorView::openAddWorkspaceDialog() {
  m_addWorkspaceDialog = std::make_unique<AddWorkspaceDialog>(this);
  m_addWorkspaceDialog->show();
  connect(m_addWorkspaceDialog.get(), SIGNAL(closeDialog()), this, SLOT(closeAddWorkspaceDialog()));
  connect(m_addWorkspaceDialog.get(), SIGNAL(okClicked(bool)), this, SLOT(addWorkspaceDialogAccepted(bool)));
}

void FitScriptGeneratorView::closeAddWorkspaceDialog() {
  disconnect(m_addWorkspaceDialog.get(), SIGNAL(closeDialog()), this, SLOT(closeAddWorkspaceDialog()));
  disconnect(m_addWorkspaceDialog.get(), SIGNAL(okClicked(bool)), this, SLOT(addWorkspaceDialogAccepted(bool)));
  m_addWorkspaceDialog->close();
  m_addWorkspaceDialog.reset();
}

void FitScriptGeneratorView::addWorkspaceDialogAccepted(bool close) {
  m_presenter->notifyPresenter(ViewEvent::AddDomainAccepted);
  if (close)
    closeAddWorkspaceDialog();
}

std::vector<MatrixWorkspace_const_sptr> FitScriptGeneratorView::getDialogWorkspaces() {
  std::vector<MatrixWorkspace_const_sptr> workspaces;
  if (m_addWorkspaceDialog) {
    workspaces = m_addWorkspaceDialog->getWorkspaces();
    if (workspaces.empty())
      displayWarning("Failed to add workspace: '" + m_addWorkspaceDialog->workspaceName().toStdString() +
                     "' doesn't exist.");
  }
  return workspaces;
}

std::vector<WorkspaceIndex> FitScriptGeneratorView::getDialogWorkspaceIndices() const {
  std::vector<WorkspaceIndex> workspaceIndices;
  if (m_addWorkspaceDialog)
    workspaceIndices = convertToWorkspaceIndex(m_addWorkspaceDialog->workspaceIndices());
  return workspaceIndices;
}

void FitScriptGeneratorView::openEditLocalParameterDialog(
    std::string const &parameter, std::vector<std::string> const &workspaceNames,
    std::vector<std::string> const &domainNames, std::vector<double> const &values, std::vector<bool> const &fixes,
    std::vector<std::string> const &ties, std::vector<std::string> const &constraints) {
  m_editLocalParameterDialog = new EditLocalParameterDialog(
      this, QString::fromStdString(parameter), convertToQStringList(toQString, workspaceNames),
      convertToQStringList(toQString, domainNames), convertToQList(values), convertToQList(fixes),
      convertToQStringList(toQString, ties), convertToQStringList(toQString, constraints));

  connect(m_editLocalParameterDialog, SIGNAL(finished(int)), this, SLOT(onEditLocalParameterFinished(int)));

  m_editLocalParameterDialog->open();
}

std::tuple<std::string, std::vector<double>, std::vector<bool>, std::vector<std::string>, std::vector<std::string>>
FitScriptGeneratorView::getEditLocalParameterResults() const {
  return {m_editLocalParameterDialog->getParameterName().toStdString(),
          convertQListToStdVector(m_editLocalParameterDialog->getValues()),
          convertQListToStdVector(m_editLocalParameterDialog->getFixes()),
          convertToStdVector(m_editLocalParameterDialog->getTies()),
          convertToStdVector(m_editLocalParameterDialog->getConstraints())};
}

std::tuple<std::string, std::string, std::string, std::string, std::string, bool>
FitScriptGeneratorView::fitOptions() const {
  return {m_fitOptionsBrowser->getProperty("Max Iterations"),   m_fitOptionsBrowser->getProperty("Minimizer"),
          m_fitOptionsBrowser->getProperty("Cost Function"),    m_fitOptionsBrowser->getProperty("Evaluation Type"),
          m_fitOptionsBrowser->getProperty("Output Base Name"), m_fitOptionsBrowser->getBoolProperty("Plot Output")};
}

std::string FitScriptGeneratorView::filepath() const {
  auto const defaultDirectory = getDefaultScriptDirectory();
  auto const filePath = QFileDialog::getSaveFileName(this->parentWidget(), tr("Save Script As "), defaultDirectory,
                                                     tr("Script files (*.py)"));

  if (!filePath.isEmpty())
    API::AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(filePath).absoluteDir().path());

  return filePath.toStdString();
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
  m_functionTreeView->setGlobalParameters(convertToQStringList(globalToQString, globalParameters));
}

void FitScriptGeneratorView::setSuccessText(std::string const &text) {
  m_ui.lbMessage->setText(QString::fromStdString(text));
}

void FitScriptGeneratorView::saveTextToClipboard(std::string const &text) const {
  if (QClipboard *clipboard = QApplication::clipboard())
    clipboard->setText(QString::fromStdString(text));
}

void FitScriptGeneratorView::displayWarning(std::string const &message) {
  QMessageBox::warning(this, "Warning!", QString::fromStdString(message));
}

void FitScriptGeneratorView::closeEvent(QCloseEvent *event) {
  if (m_addWorkspaceDialog)
    closeAddWorkspaceDialog();
  QWidget::closeEvent(event);
}

} // namespace MantidWidgets
} // namespace MantidQt
