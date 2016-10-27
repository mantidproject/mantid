#ifndef MANTID_MANTIDWIDGETS_ADSADAPTER_H_
#define MANTID_MANTIDWIDGETS_ADSADAPTER_H_

#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/WorkspaceProvider.h"
#include <MantidQtAPI/WorkspaceObserver.h>
#include <Poco/NObserver.h>

namespace MantidQt {
namespace MantidWidgets {
/**
\class  ADSAdapter
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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS ADSAdapter : public WorkspaceProvider {
public:
  explicit ADSAdapter();
  ~ADSAdapter() override;
  void registerPresenter(Presenter_wptr presenter) override;
  bool doesWorkspaceExist(const std::string &wsname) const override;

  std::map<std::string, Mantid::API::Workspace_sptr>
  topLevelItems() const override;

  std::string getOldName() const override;
  std::string getNewName() const override;

private:
  std::string m_oldName;
  std::string m_newName;

  Presenter_wptr m_presenter;

  Presenter_sptr lockPresenter();

  // ADS Notification Handlers
  void handleAddWorkspace(Mantid::API::WorkspaceAddNotification_ptr pNf);
  Poco::NObserver<ADSAdapter, Mantid::API::WorkspaceAddNotification>
      m_addObserver;

  void handleReplaceWorkspace(
      Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf);
  Poco::NObserver<ADSAdapter, Mantid::API::WorkspaceAfterReplaceNotification>
      m_replaceObserver;

  void
  handleDeleteWorkspace(Mantid::API::WorkspacePostDeleteNotification_ptr pNf);
  Poco::NObserver<ADSAdapter, Mantid::API::WorkspacePostDeleteNotification>
      m_deleteObserver;

  void handleClearADS(Mantid::API::ClearADSNotification_ptr pNf);
  Poco::NObserver<ADSAdapter, Mantid::API::ClearADSNotification>
      m_clearADSObserver;

  void handleRenameWorkspace(Mantid::API::WorkspaceRenameNotification_ptr pNf);
  Poco::NObserver<ADSAdapter, Mantid::API::WorkspaceRenameNotification>
      m_renameObserver;

  void
  handleGroupWorkspaces(Mantid::API::WorkspacesGroupedNotification_ptr pNf);
  Poco::NObserver<ADSAdapter, Mantid::API::WorkspacesGroupedNotification>
      m_groupworkspacesObserver;

  void
  handleUnGroupWorkspace(Mantid::API::WorkspaceUnGroupingNotification_ptr pNf);
  Poco::NObserver<ADSAdapter, Mantid::API::WorkspaceUnGroupingNotification>
      m_ungroupworkspaceObserver;

  void
  handleWorkspaceGroupUpdate(Mantid::API::GroupUpdatedNotification_ptr pNf);
  Poco::NObserver<ADSAdapter, Mantid::API::GroupUpdatedNotification>
      m_workspaceGroupUpdateObserver;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTID_MANTIDWIDGETS_ADSADAPTER_H_