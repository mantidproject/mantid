// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorView.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorPresenter.h"

#include <stdexcept>

namespace MantidQt {
namespace MantidWidgets {

using FittingType = FitOptionsBrowser::FittingType;

FitScriptGeneratorView::FitScriptGeneratorView(
    QWidget *parent, QStringList const &workspaceNames, double startX,
    double endX, QMap<QString, QString> const &fitOptions)
    : API::MantidWidget(parent), m_presenter() {
  m_ui.setupUi(this);

  m_ui.twDomainData->setColumnWidth(0, 220);
  m_ui.twDomainData->setColumnWidth(1, 90);
  m_ui.twDomainData->setColumnWidth(2, 90);
  m_ui.twDomainData->setColumnWidth(3, 90);

  m_functionBrowser = std::make_unique<FunctionBrowser>(nullptr, true);
  m_fitOptionsBrowser = std::make_unique<FitOptionsBrowser>(
      nullptr, FittingType::SimultaneousAndSequential);

  m_ui.splitter->addWidget(m_functionBrowser.get());
  m_ui.splitter->addWidget(m_fitOptionsBrowser.get());

  setWorkspaces(workspaceNames, startX, endX);
  setFitBrowserOptions(fitOptions);
  connectUiSignals();
}

FitScriptGeneratorView::~FitScriptGeneratorView() {
  m_functionBrowser.reset();
  m_fitOptionsBrowser.reset();
}

void FitScriptGeneratorView::connectUiSignals() {
  connect(m_ui.pbRemove, SIGNAL(clicked()), this, SLOT(onRemoveClicked()));
}

void FitScriptGeneratorView::setWorkspaces(QStringList const &workspaceNames,
                                           double startX, double endX) {
  for (auto const &workspaceName : workspaceNames)
    addWorkspaceDomain(workspaceName, 0, startX, endX);
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

void FitScriptGeneratorView::addWorkspaceDomain(QString const &workspaceName,
                                                int workspaceIndex,
                                                double startX, double endX) {
  auto const rowIndex = m_ui.twDomainData->rowCount();
  m_ui.twDomainData->insertRow(rowIndex);

  setItemInDomainTable(rowIndex, ColumnIndex::WorkspaceName,
                       QVariant(workspaceName));
  setItemInDomainTable(rowIndex, ColumnIndex::WorkspaceIndex,
                       QVariant(workspaceIndex));
  setItemInDomainTable(rowIndex, ColumnIndex::StartX, QVariant(startX));
  setItemInDomainTable(rowIndex, ColumnIndex::EndX, QVariant(endX));
}

void FitScriptGeneratorView::setItemInDomainTable(int rowIndex, int columnIndex,
                                                  QVariant const &value) {
  m_ui.twDomainData->setItem(rowIndex, columnIndex,
                             new QTableWidgetItem(value.toString()));
}

} // namespace MantidWidgets
} // namespace MantidQt
