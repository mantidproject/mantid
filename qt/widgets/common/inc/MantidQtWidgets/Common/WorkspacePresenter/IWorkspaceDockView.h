// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include <map>
#include <memory>
namespace MantidQt {
namespace MantidWidgets {

class WorkspaceProviderNotifiable;
class ViewNotifiable;

using WorkspacePresenterWN_wptr = std::weak_ptr<WorkspaceProviderNotifiable>;
using WorkspacePresenterVN_sptr = std::shared_ptr<ViewNotifiable>;
using StringList = std::vector<std::string>;
/**
\class  IWorkspaceDockView
\author Lamar Moore
\date   24-08-2016
\version 1.0
*/
class IWorkspaceDockView {
public:
  enum class SortDirection { Ascending, Descending };
  enum class SortCriteria { ByName, ByLastModified, ByMemorySize };
  enum class SaveFileType { Nexus, ASCII };

  virtual ~IWorkspaceDockView() {};

  virtual WorkspacePresenterWN_wptr getPresenterWeakPtr() = 0;

  virtual bool askUserYesNo(const std::string &caption, const std::string &message) const = 0;
  virtual void showCriticalUserMessage(const std::string &caption, const std::string &message) const = 0;
  virtual void showLoadDialog() = 0;
  virtual void showLiveDataDialog() = 0;
  virtual void showRenameDialog(const StringList &wsNames) = 0;
  virtual void recordWorkspaceRename(const std::string &oldName, const std::string &newName) = 0;
  virtual void enableDeletePrompt(bool enable) = 0;
  virtual bool isPromptDelete() const = 0;
  virtual bool deleteConfirmation() const = 0;
  virtual void deleteWorkspaces(const StringList &wsNames) = 0;
  virtual bool clearWorkspacesConfirmation() const = 0;
  virtual void enableClearButton(bool enable) = 0;
  virtual void clearView() = 0;
  virtual SortDirection getSortDirection() const = 0;
  virtual SortCriteria getSortCriteria() const = 0;
  virtual void sortWorkspaces(SortCriteria criteria, SortDirection direction) = 0;
  virtual SaveFileType getSaveFileType() const = 0;
  virtual void saveWorkspace(const std::string &wsName, const SaveFileType type) = 0;
  virtual void saveWorkspaces(const StringList &wsNames) = 0;
  virtual std::string getFilterText() const = 0;
  virtual void filterWorkspaces(const std::string &filterText) = 0;
  virtual StringList getSelectedWorkspaceNames() const = 0;
  virtual Mantid::API::Workspace_sptr getSelectedWorkspace() const = 0;
  virtual void refreshWorkspaces() = 0;
  virtual void updateTree(const std::map<std::string, Mantid::API::Workspace_sptr> &items) = 0;

  // Workspace Context Menu Handlers
  virtual void popupContextMenu() = 0;
  virtual void showWorkspaceData() = 0;
  virtual void showInstrumentView() = 0;
  virtual void saveToProgram() = 0;
  virtual void plotSpectrum(const std::string &type) = 0;
  virtual void showColourFillPlot() = 0;
  virtual void showDetectorsTable() = 0;
  virtual void showBoxDataTable() = 0;
  virtual void showMDPlot() = 0;
  virtual void showListData() = 0;
  virtual void showSpectrumViewer() = 0;
  virtual void showSliceViewer() = 0;
  virtual void showLogs() = 0;
  virtual void showSampleMaterialWindow() = 0;
  virtual void showAlgorithmHistory() = 0;
  virtual void showTransposed() = 0;
  virtual void convertToMatrixWorkspace() = 0;
  virtual void convertMDHistoToMatrixWorkspace() = 0;

  virtual bool executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg, const bool wait = true) = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt
