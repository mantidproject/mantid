#ifndef MANTIDQT_MANTIDWIDGETS_IWORKSPACEDOCKVIEW_H_
#define MANTIDQT_MANTIDWIDGETS_IWORKSPACEDOCKVIEW_H_

#include <MantidAPI/IAlgorithm_fwd.h>
#include <MantidAPI/Workspace_fwd.h>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <map>

namespace MantidQt {
namespace MantidWidgets {

class WorkspaceProviderNotifiable;
class ViewNotifiable;

using WorkspacePresenterWN_wptr = boost::weak_ptr<WorkspaceProviderNotifiable>;
using WorkspacePresenterVN_sptr = boost::shared_ptr<ViewNotifiable>;
using StringList = std::vector<std::string>;
/**
\class  IWorkspaceDockView
\author Lamar Moore
\date   24-08-2016
\version 1.0


Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
*/
class IWorkspaceDockView {
public:
  enum class SortDirection { Ascending, Descending };
  enum class SortCriteria { ByName, ByLastModified };
  enum class SaveFileType { Nexus, ASCII, ASCIIv1 };

  virtual ~IWorkspaceDockView(){};

  virtual void init() = 0;
  virtual WorkspacePresenterWN_wptr getPresenterWeakPtr() = 0;

  virtual bool askUserYesNo(const std::string &caption,
                            const std::string &message) const = 0;
  virtual void showCriticalUserMessage(const std::string &caption,
                                       const std::string &message) const = 0;
  virtual void showLoadDialog() = 0;
  virtual void showLiveDataDialog() = 0;
  virtual void showRenameDialog(const StringList &wsNames) = 0;
  virtual void recordWorkspaceRename(const std::string &oldName,
                                     const std::string &newName) = 0;
  virtual void enableDeletePrompt(bool enable) = 0;
  virtual bool isPromptDelete() const = 0;
  virtual bool deleteConfirmation() const = 0;
  virtual void deleteWorkspaces(const StringList &wsNames) = 0;
  virtual void clearView() = 0;
  virtual SortDirection getSortDirection() const = 0;
  virtual SortCriteria getSortCriteria() const = 0;
  virtual void sortWorkspaces(SortCriteria criteria,
                              SortDirection direction) = 0;
  virtual SaveFileType getSaveFileType() const = 0;
  virtual void saveWorkspace(SaveFileType type) = 0;
  virtual void saveWorkspaces(const StringList &wsNames) = 0;
  virtual std::string getFilterText() const = 0;
  virtual void filterWorkspaces(const std::string &filterText) = 0;
  virtual StringList getSelectedWorkspaceNames() const = 0;
  virtual Mantid::API::Workspace_sptr getSelectedWorkspace() const = 0;
  virtual void refreshWorkspaces() = 0;
  virtual void updateTree(
      const std::map<std::string, Mantid::API::Workspace_sptr> &items) = 0;

  // Workspace Context Menu Handlers
  virtual void popupContextMenu() = 0;
  virtual void showWorkspaceData() = 0;
  virtual void showInstrumentView() = 0;
  virtual void saveToProgram() = 0;
  virtual void plotSpectrum(std::string type) = 0;
  virtual void showColourFillPlot() = 0;
  virtual void showDetectorsTable() = 0;
  virtual void showBoxDataTable() = 0;
  virtual void showVatesGUI() = 0;
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

  virtual bool executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg,
                                     const bool wait = true) = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_MANTIDWIDGETS_IWORKSPACEDOCKVIEW_H_