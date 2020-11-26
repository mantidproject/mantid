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

#include <stdexcept>

using namespace Mantid::API;

namespace MantidQt {
namespace MantidWidgets {

using FittingType = FitOptionsBrowser::FittingType;

FitScriptGeneratorView::FitScriptGeneratorView(
    QWidget *parent, QStringList const &workspaceNames, double startX,
    double endX, QMap<QString, QString> const &fitOptions)
    : API::MantidWidget(parent), m_presenter(),
      m_dataTable(std::make_unique<FitScriptGeneratorDataTable>()),
      m_functionBrowser(std::make_unique<FunctionBrowser>(nullptr, true)),
      m_fitOptionsBrowser(std::make_unique<FitOptionsBrowser>(
          nullptr, FittingType::SimultaneousAndSequential)) {
  m_ui.setupUi(this);

  m_ui.fDataTable->layout()->addWidget(m_dataTable.get());
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
    addWorkspace(workspaceName, startX, endX);
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

void FitScriptGeneratorView::addWorkspace(QString const &workspaceName,
                                          double startX, double endX) {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName.toStdString()))
    addWorkspace(ads.retrieveWS<MatrixWorkspace>(workspaceName.toStdString()),
                 startX, endX);
}

void FitScriptGeneratorView::addWorkspace(
    MatrixWorkspace_const_sptr const &workspace, double startX, double endX) {
  for (auto i = 0; i < static_cast<int>(workspace->getNumberHistograms()); ++i)
    addWorkspaceDomain(QString::fromStdString(workspace->getName()), i, startX,
                       endX);
}

void FitScriptGeneratorView::addWorkspaceDomain(QString const &workspaceName,
                                                int workspaceIndex,
                                                double startX, double endX) {
  m_dataTable->addWorkspaceDomain(workspaceName, workspaceIndex, startX, endX);
}

} // namespace MantidWidgets
} // namespace MantidQt
