// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspacePresenter.h"

#include "MantidQtWidgets/Common/WorkspacePresenter/ADSAdapter.h"
#include "MantidQtWidgets/Common/WorkspacePresenter/IWorkspaceDockView.h"
#include <MantidAPI/AlgorithmManager.h>

using namespace Mantid;

namespace MantidQt {
namespace MantidWidgets {

WorkspacePresenter::WorkspacePresenter(IWorkspaceDockView *view)
    : m_view(std::move(view)), m_adapter(std::make_unique<ADSAdapter>()) {}

WorkspacePresenter::~WorkspacePresenter() {}

/// Initialises the view weak pointer for the Workspace Provider.
void WorkspacePresenter::init() {
  m_adapter->registerPresenter(m_view->getPresenterWeakPtr());
}

/// Handle WorkspaceProvider (ADS) notifications
void WorkspacePresenter::notifyFromWorkspaceProvider(
    WorkspaceProviderNotifiable::Flag flag) {
  switch (flag) {
  case WorkspaceProviderNotifiable::Flag::WorkspaceLoaded:
    workspaceLoaded();
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspaceRenamed:
    workspaceRenamed();
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspacesGrouped:
    workspacesGrouped();
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspacesUngrouped:
    workspacesUngrouped();
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspaceGroupUpdated:
    workspaceGroupUpdated();
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspaceDeleted:
    workspacesDeleted();
    break;
  case WorkspaceProviderNotifiable::Flag::WorkspacesCleared:
    workspacesCleared();
    break;
  case WorkspaceProviderNotifiable::Flag::GenericUpdateNotification:
    updateView();
    break;
  }
}

/// Handle notifications from the view.
void WorkspacePresenter::notifyFromView(ViewNotifiable::Flag flag) {
  switch (flag) {
  case ViewNotifiable::Flag::LoadWorkspace:
    loadWorkspace();
    break;
  case ViewNotifiable::Flag::LoadLiveDataWorkspace:
    loadLiveData();
    break;
  case ViewNotifiable::Flag::RenameWorkspace:
    renameWorkspace();
    break;
  case ViewNotifiable::Flag::GroupWorkspaces:
    groupWorkspaces();
    break;
  case ViewNotifiable::Flag::UngroupWorkspaces:
    ungroupWorkspaces();
    break;
  case ViewNotifiable::Flag::SortWorkspaces:
    sortWorkspaces();
    break;
  case ViewNotifiable::Flag::DeleteWorkspaces:
    deleteWorkspaces();
    break;
  case ViewNotifiable::Flag::SaveSingleWorkspace:
    saveSingleWorkspace();
    break;
  case ViewNotifiable::Flag::SaveWorkspaceCollection:
    saveWorkspaceCollection();
    break;
  case ViewNotifiable::Flag::FilterWorkspaces:
    filterWorkspaces();
    break;
  case ViewNotifiable::Flag::PopulateAndShowWorkspaceContextMenu:
    populateAndShowWorkspaceContextMenu();
    break;
  case ViewNotifiable::Flag::ShowWorkspaceData:
    showWorkspaceData();
    break;
  case ViewNotifiable::Flag::ShowInstrumentView:
    showInstrumentView();
    break;
  case ViewNotifiable::Flag::SaveToProgram:
    saveToProgram();
    break;
  case ViewNotifiable::Flag::PlotSpectrum:
    plotSpectrum();
    break;
  case ViewNotifiable::Flag::PlotSpectrumWithErrors:
    plotSpectrumWithErrors();
    break;
  case ViewNotifiable::Flag::PlotSpectrumAdvanced:
    plotSpectrumAdvanced();
    break;
  case ViewNotifiable::Flag::ShowColourFillPlot:
    showColourFillPlot();
    break;
  case ViewNotifiable::Flag::ShowDetectorsTable:
    showDetectorsTable();
    break;
  case ViewNotifiable::Flag::ShowBoxDataTable:
    showBoxDataTable();
    break;
  case ViewNotifiable::Flag::ShowVatesGUI:
    showVatesGUI();
    break;
  case ViewNotifiable::Flag::ShowMDPlot:
    showMDPlot();
    break;
  case ViewNotifiable::Flag::ShowListData:
    showListData();
    break;
  case ViewNotifiable::Flag::ShowSpectrumViewer:
    showSpectrumViewer();
    break;
  case ViewNotifiable::Flag::ShowSliceViewer:
    showSliceViewer();
    break;
  case ViewNotifiable::Flag::ShowLogs:
    showLogs();
    break;
  case ViewNotifiable::Flag::ShowSampleMaterialWindow:
    showSampleMaterialWindow();
    break;
  case ViewNotifiable::Flag::ShowAlgorithmHistory:
    showAlgorithmHistory();
    break;
  case ViewNotifiable::Flag::ShowTransposed:
    showTransposed();
    break;
  case ViewNotifiable::Flag::ConvertToMatrixWorkspace:
    convertToMatrixWorkspace();
    break;
  case ViewNotifiable::Flag::ConvertMDHistoToMatrixWorkspace:
    convertMDHistoToMatrixWorkspace();
    break;
  case ViewNotifiable::Flag::ClearUBMatrix:
    clearUBMatrix();
    break;
  case ViewNotifiable::Flag::RefreshWorkspaces:
    refreshWorkspaces();
    break;
  }
}

void WorkspacePresenter::loadWorkspace() { m_view->showLoadDialog(); }

void WorkspacePresenter::loadLiveData() { m_view->showLiveDataDialog(); }

void WorkspacePresenter::renameWorkspace() {
  m_view->showRenameDialog(m_view->getSelectedWorkspaceNames());
}

void WorkspacePresenter::groupWorkspaces() {
  auto selected = m_view->getSelectedWorkspaceNames();

  std::string groupName("NewGroup");
  // get selected workspaces
  if (selected.size() < 2) {
    m_view->showCriticalUserMessage("Cannot Group Workspaces",
                                    "Select at least two workspaces to group ");
    return;
  }

  if (m_adapter->doesWorkspaceExist(groupName)) {
    if (!m_view->askUserYesNo(
            "", "Workspace " + groupName +
                    " already exists. Do you want to replace it?"))
      return;
  }

  try {
    std::string algName("GroupWorkspaces");
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create(algName, -1);
    alg->initialize();
    alg->setProperty("InputWorkspaces", selected);
    alg->setPropertyValue("OutputWorkspace", groupName);
    // execute the algorithm
    bool bStatus = alg->execute();
    if (!bStatus) {
      m_view->showCriticalUserMessage("MantidPlot - Algorithm error",
                                      " Error in GroupWorkspaces algorithm");
    }
  } catch (...) {
    m_view->showCriticalUserMessage("MantidPlot - Algorithm error",
                                    " Error in GroupWorkspaces algorithm");
  }
}

void WorkspacePresenter::ungroupWorkspaces() {
  auto selected = m_view->getSelectedWorkspaceNames();

  if (selected.size() == 0) {
    m_view->showCriticalUserMessage("Error Ungrouping Workspaces",
                                    "Select a group workspace to Ungroup.");
    return;
  }

  try {
    // workspace name
    auto wsname = selected[0];

    std::string algName("UnGroupWorkspace");
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create(algName, -1);
    alg->initialize();
    alg->setProperty("InputWorkspace", wsname);

    // execute the algorithm
    bool bStatus = alg->execute();
    if (!bStatus) {
      m_view->showCriticalUserMessage("MantidPlot - Algorithm error",
                                      " Error in UnGroupWorkspace algorithm");
    }
  } catch (...) {
    m_view->showCriticalUserMessage("MantidPlot - Algorithm error",
                                    " Error in UnGroupWorkspace algorithm");
  }
}

void WorkspacePresenter::sortWorkspaces() {
  m_view->sortWorkspaces(m_view->getSortCriteria(), m_view->getSortDirection());
}

void WorkspacePresenter::deleteWorkspaces() {
  bool deleteWs = true;
  auto selected = m_view->getSelectedWorkspaceNames();

  // Ensure all workspaces exist in the ADS
  if (!std::all_of(selected.cbegin(), selected.cend(),
                   [=](const std::string &ws) {
                     return m_adapter->doesWorkspaceExist(ws);
                   })) {
    m_view->showCriticalUserMessage(
        "Delete Workspaces",
        "Unabel to delete workspaces. Invalid workspace names provided.");
    return;
  }

  if (m_view->isPromptDelete())
    deleteWs = m_view->deleteConfirmation();

  if (deleteWs)
    m_view->deleteWorkspaces(selected);
}

void WorkspacePresenter::saveSingleWorkspace() {
  m_view->saveWorkspace(m_view->getSelectedWorkspace()->getName(),
                        m_view->getSaveFileType());
}

void WorkspacePresenter::saveWorkspaceCollection() {
  m_view->saveWorkspaces(m_view->getSelectedWorkspaceNames());
}

void WorkspacePresenter::filterWorkspaces() {
  m_view->filterWorkspaces(m_view->getFilterText());
}

void WorkspacePresenter::populateAndShowWorkspaceContextMenu() {
  m_view->popupContextMenu();
}

void WorkspacePresenter::showWorkspaceData() { m_view->showWorkspaceData(); }

void WorkspacePresenter::showInstrumentView() { m_view->showInstrumentView(); }

void WorkspacePresenter::saveToProgram() { m_view->saveToProgram(); }

void WorkspacePresenter::plotSpectrum() { m_view->plotSpectrum("Simple"); }

void WorkspacePresenter::plotSpectrumWithErrors() {
  m_view->plotSpectrum("Errors");
}

void WorkspacePresenter::plotSpectrumAdvanced() {
  m_view->plotSpectrum("Advanced");
}

void WorkspacePresenter::showColourFillPlot() { m_view->showColourFillPlot(); }

void WorkspacePresenter::showDetectorsTable() { m_view->showDetectorsTable(); }

void WorkspacePresenter::showBoxDataTable() { m_view->showBoxDataTable(); }

void WorkspacePresenter::showVatesGUI() { m_view->showVatesGUI(); }

void WorkspacePresenter::showMDPlot() { m_view->showMDPlot(); }

void WorkspacePresenter::showListData() { m_view->showListData(); }

void WorkspacePresenter::showSpectrumViewer() { m_view->showSpectrumViewer(); }

void WorkspacePresenter::showSliceViewer() { m_view->showSliceViewer(); }

void WorkspacePresenter::showLogs() { m_view->showLogs(); }

void WorkspacePresenter::showSampleMaterialWindow() {
  m_view->showSampleMaterialWindow();
}

void WorkspacePresenter::showAlgorithmHistory() {
  m_view->showAlgorithmHistory();
}

void WorkspacePresenter::showTransposed() { m_view->showTransposed(); }

void WorkspacePresenter::convertToMatrixWorkspace() {
  m_view->convertToMatrixWorkspace();
}

void WorkspacePresenter::convertMDHistoToMatrixWorkspace() {
  m_view->convertMDHistoToMatrixWorkspace();
}

void WorkspacePresenter::clearUBMatrix() {
  auto wsNames = m_view->getSelectedWorkspaceNames();

  for (auto &ws : wsNames) {
    auto alg = Mantid::API::AlgorithmManager::Instance().create("ClearUB", -1);
    if (alg) {
      alg->initialize();
      alg->setPropertyValue("Workspace", ws);
      // Run in this manner due to Qt dependencies within this method.
      // otherwise it would have been implemented here.
      m_view->executeAlgorithmAsync(alg);
    } else
      break;
  }
}

void WorkspacePresenter::refreshWorkspaces() { updateView(); }

void WorkspacePresenter::workspaceLoaded() { updateView(); }

void WorkspacePresenter::workspaceRenamed() {
  m_view->recordWorkspaceRename(m_adapter->getOldName(),
                                m_adapter->getNewName());
  m_view->updateTree(m_adapter->topLevelItems());
}

void WorkspacePresenter::workspacesGrouped() { updateView(); }
void WorkspacePresenter::workspacesUngrouped() { updateView(); }
void WorkspacePresenter::workspaceGroupUpdated() { updateView(); }

void WorkspacePresenter::workspacesDeleted() { updateView(); }

void WorkspacePresenter::workspacesCleared() { m_view->clearView(); }

/// Update the view by publishing the ADS contents.
void WorkspacePresenter::updateView() {
  m_view->updateTree(m_adapter->topLevelItems());
}

} // namespace MantidWidgets
} // namespace MantidQt
