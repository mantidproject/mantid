#ifndef MANTIDQT_MANTIDWIDGETS_WORKSPACEPRESENTER_H_
#define MANTIDQT_MANTIDWIDGETS_WORKSPACEPRESENTER_H_

#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/ViewNotifiable.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspaceProvider.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspaceProviderNotifiable.h"
#include <boost/weak_ptr.hpp>
#include <memory>

namespace MantidQt {
namespace MantidWidgets {

class IWorkspaceDockView;
class WorkspaceProvider;

using DockView_sptr = boost::shared_ptr<IWorkspaceDockView>;
using DockView_wptr = boost::weak_ptr<IWorkspaceDockView>;
using ADSAdapter_uptr = std::unique_ptr<WorkspaceProvider>;
/**
\class  WorkspacePresenter
\brief  Presenter class for Workspace dock in MantidPlot UI
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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS WorkspacePresenter
    : public WorkspaceProviderNotifiable,
      public ViewNotifiable {
public:
  explicit WorkspacePresenter(DockView_wptr view);
  ~WorkspacePresenter() override;

  void init();

  void
  notifyFromWorkspaceProvider(WorkspaceProviderNotifiable::Flag flag) override;
  void notifyFromView(ViewNotifiable::Flag flag) override;

private:
  void loadWorkspace();
  void loadLiveData();
  void renameWorkspace();
  void groupWorkspaces();
  void ungroupWorkspaces();
  void sortWorkspaces();
  void deleteWorkspaces();
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
  void showVatesGUI();
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

  DockView_sptr lockView();
  void updateView();

private:
  DockView_wptr m_view;
  ADSAdapter_uptr m_adapter;
};
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQT_MANTIDWIDGETS_WORKSPACEPRESENTER_H_