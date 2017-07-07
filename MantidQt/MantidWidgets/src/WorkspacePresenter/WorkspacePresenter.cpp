#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspacePresenter.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/ADSAdapter.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/IWorkspaceDockView.h"
#include <MantidAPI/AlgorithmManager.h>

using namespace Mantid;

namespace MantidQt {
namespace MantidWidgets {

WorkspacePresenter::WorkspacePresenter(DockView_wptr view)
    : m_view(std::move(view)), m_adapter(Kernel::make_unique<ADSAdapter>()) {}

WorkspacePresenter::~WorkspacePresenter() {}

/// Initialises the view weak pointer for the Workspace Provider.
void WorkspacePresenter::init() {
  m_adapter->registerPresenter(m_view.lock()->getPresenterWeakPtr());
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

void WorkspacePresenter::loadWorkspace() {
  auto view = lockView();
  view->showLoadDialog();
}

void WorkspacePresenter::loadLiveData() {
  auto view = lockView();
  view->showLiveDataDialog();
}

void WorkspacePresenter::renameWorkspace() {
  auto view = lockView();
  view->showRenameDialog(view->getSelectedWorkspaceNames());
}

void WorkspacePresenter::groupWorkspaces() {
  auto view = lockView();
  auto selected = view->getSelectedWorkspaceNames();

  std::string groupName("NewGroup");
  // get selected workspaces
  if (selected.size() < 2) {
    view->showCriticalUserMessage("Cannot Group Workspaces",
                                  "Select at least two workspaces to group ");
    return;
  }

  if (m_adapter->doesWorkspaceExist(groupName)) {
    if (!view->askUserYesNo("",
                            "Workspace " + groupName +
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
      view->showCriticalUserMessage("MantidPlot - Algorithm error",
                                    " Error in GroupWorkspaces algorithm");
    }
  } catch (...) {
    view->showCriticalUserMessage("MantidPlot - Algorithm error",
                                  " Error in GroupWorkspaces algorithm");
  }
}

void WorkspacePresenter::ungroupWorkspaces() {
  auto view = lockView();
  auto selected = view->getSelectedWorkspaceNames();

  if (selected.size() == 0) {
    view->showCriticalUserMessage("Error Ungrouping Workspaces",
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
      view->showCriticalUserMessage("MantidPlot - Algorithm error",
                                    " Error in UnGroupWorkspace algorithm");
    }
  } catch (...) {
    view->showCriticalUserMessage("MantidPlot - Algorithm error",
                                  " Error in UnGroupWorkspace algorithm");
  }
}

void WorkspacePresenter::sortWorkspaces() {
  auto view = lockView();

  view->sortWorkspaces(view->getSortCriteria(), view->getSortDirection());
}

void WorkspacePresenter::deleteWorkspaces() {
  auto view = lockView();
  bool deleteWs = true;
  auto selected = view->getSelectedWorkspaceNames();

  // Ensure all workspaces exist in the ADS
  if (!std::all_of(selected.cbegin(), selected.cend(),
                   [=](const std::string &ws) {
                     return m_adapter->doesWorkspaceExist(ws);
                   })) {
    view->showCriticalUserMessage(
        "Delete Workspaces",
        "Unabel to delete workspaces. Invalid workspace names provided.");
    return;
  }

  if (view->isPromptDelete())
    deleteWs = view->deleteConfirmation();

  if (deleteWs)
    view->deleteWorkspaces(selected);
}

void WorkspacePresenter::saveSingleWorkspace() {
  auto view = lockView();
  view->saveWorkspace(view->getSaveFileType());
}

void WorkspacePresenter::saveWorkspaceCollection() {
  auto view = lockView();
  view->saveWorkspaces(view->getSelectedWorkspaceNames());
}

void WorkspacePresenter::filterWorkspaces() {
  auto view = lockView();
  view->filterWorkspaces(view->getFilterText());
}

void WorkspacePresenter::populateAndShowWorkspaceContextMenu() {
  auto view = lockView();
  view->popupContextMenu();
}

void WorkspacePresenter::showWorkspaceData() {
  auto view = lockView();
  view->showWorkspaceData();
}

void WorkspacePresenter::showInstrumentView() {
  auto view = lockView();
  view->showInstrumentView();
}

void WorkspacePresenter::saveToProgram() {
  auto view = lockView();
  view->saveToProgram();
}

void WorkspacePresenter::plotSpectrum() {
  auto view = lockView();
  view->plotSpectrum("Simple");
}

void WorkspacePresenter::plotSpectrumWithErrors() {
  auto view = lockView();
  view->plotSpectrum("Errors");
}

void WorkspacePresenter::plotSpectrumAdvanced() {
  auto view = lockView();
  view->plotSpectrum("Advanced");
}

void WorkspacePresenter::showColourFillPlot() {
  auto view = lockView();
  view->showColourFillPlot();
}

void WorkspacePresenter::showDetectorsTable() {
  auto view = lockView();
  view->showDetectorsTable();
}

void WorkspacePresenter::showBoxDataTable() {
  auto view = lockView();
  view->showBoxDataTable();
}

void WorkspacePresenter::showVatesGUI() {
  auto view = lockView();
  view->showVatesGUI();
}

void WorkspacePresenter::showMDPlot() {
  auto view = lockView();
  view->showMDPlot();
}

void WorkspacePresenter::showListData() {
  auto view = lockView();
  view->showListData();
}

void WorkspacePresenter::showSpectrumViewer() {
  auto view = lockView();
  view->showSpectrumViewer();
}

void WorkspacePresenter::showSliceViewer() {
  auto view = lockView();
  view->showSliceViewer();
}

void WorkspacePresenter::showLogs() {
  auto view = lockView();
  view->showLogs();
}

void WorkspacePresenter::showSampleMaterialWindow() {
  auto view = lockView();
  view->showSampleMaterialWindow();
}

void WorkspacePresenter::showAlgorithmHistory() {
  auto view = lockView();
  view->showAlgorithmHistory();
}

void WorkspacePresenter::showTransposed() {
  auto view = lockView();
  view->showTransposed();
}

void WorkspacePresenter::convertToMatrixWorkspace() {
  auto view = lockView();
  view->convertToMatrixWorkspace();
}

void WorkspacePresenter::convertMDHistoToMatrixWorkspace() {
  auto view = lockView();
  view->convertMDHistoToMatrixWorkspace();
}

void WorkspacePresenter::clearUBMatrix() {
  auto view = lockView();
  auto wsNames = view->getSelectedWorkspaceNames();

  for (auto &ws : wsNames) {
    auto alg = Mantid::API::AlgorithmManager::Instance().create("ClearUB", -1);
    if (alg) {
      alg->initialize();
      alg->setPropertyValue("Workspace", ws);
      // Run in this manner due to Qt dependencies within this method.
      // otherwise it would have been implemented here.
      view->executeAlgorithmAsync(alg);
    } else
      break;
  }
}

void WorkspacePresenter::refreshWorkspaces() { updateView(); }

void WorkspacePresenter::workspaceLoaded() { updateView(); }

void WorkspacePresenter::workspaceRenamed() {
  auto view = lockView();
  view->recordWorkspaceRename(m_adapter->getOldName(), m_adapter->getNewName());
  view->updateTree(m_adapter->topLevelItems());
}

void WorkspacePresenter::workspacesGrouped() { updateView(); }
void WorkspacePresenter::workspacesUngrouped() { updateView(); }
void WorkspacePresenter::workspaceGroupUpdated() { updateView(); }

void WorkspacePresenter::workspacesDeleted() { updateView(); }

void WorkspacePresenter::workspacesCleared() {
  auto view = lockView();
  view->clearView();
}

/// Lock the view weak_ptr and return the shared_ptr generated.
DockView_sptr WorkspacePresenter::lockView() {
  auto view_sptr = m_view.lock();

  if (view_sptr == nullptr)
    throw std::runtime_error("Could not obtain pointer to DockView.");

  return view_sptr;
}

/// Update the view by publishing the ADS contents.
void WorkspacePresenter::updateView() {
  auto view = lockView();
  view->updateTree(m_adapter->topLevelItems());
}

} // namespace MantidQt
} // namespace MantidWidgets
