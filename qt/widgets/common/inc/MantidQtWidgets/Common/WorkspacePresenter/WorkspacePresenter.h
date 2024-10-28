// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/WorkspacePresenter/ViewNotifiable.h"
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceProvider.h"
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceProviderNotifiable.h"
#include <memory>

namespace MantidQt {
namespace MantidWidgets {

class IWorkspaceDockView;
class WorkspaceProvider;

using ADSAdapter_uptr = std::unique_ptr<WorkspaceProvider>;
/**
\class  WorkspacePresenter
\brief  Presenter class for Workspace dock in MantidPlot UI
\author Lamar Moore
\date   24-08-2016
\version 1.0
*/
class EXPORT_OPT_MANTIDQT_COMMON WorkspacePresenter : public WorkspaceProviderNotifiable, public ViewNotifiable {

public:
  explicit WorkspacePresenter(IWorkspaceDockView *view);
  ~WorkspacePresenter() override;

  void init();

  void notifyFromWorkspaceProvider(WorkspaceProviderNotifiable::Flag flag) override;
  void notifyFromView(ViewNotifiable::Flag flag) override;

private:
  void loadWorkspace();
  void loadLiveData();
  void renameWorkspace();
  void groupWorkspaces();
  void ungroupWorkspaces();
  void sortWorkspaces();
  void deleteWorkspaces();
  void clearWorkspaces();
  void saveSingleWorkspace();
  void saveWorkspaceCollection();
  void filterWorkspaces();
  void populateAndShowWorkspaceContextMenu();
  void showWorkspaceData();
  void showInstrumentView();
  void saveToProgram();
  void plotSpectrum();
  void plotSpectrumWithErrors();
  void plotSpectrumAdvanced();
  void showColourFillPlot();
  void showDetectorsTable();
  void showBoxDataTable();
  void showMDPlot();
  void showListData();
  void showSpectrumViewer();
  void showSliceViewer();
  void showLogs();
  void showSampleMaterialWindow();
  void showAlgorithmHistory();
  void showTransposed();
  void convertToMatrixWorkspace();
  void convertMDHistoToMatrixWorkspace();
  void clearUBMatrix();
  void refreshWorkspaces();

  void workspaceLoaded();
  void workspaceRenamed();
  void workspacesGrouped();
  void workspacesUngrouped();
  void workspaceGroupUpdated();
  void workspacesDeleted();
  void workspacesCleared();

  void updateView();

private:
  IWorkspaceDockView *m_view;
  ADSAdapter_uptr m_adapter;
};
} // namespace MantidWidgets
} // namespace MantidQt
