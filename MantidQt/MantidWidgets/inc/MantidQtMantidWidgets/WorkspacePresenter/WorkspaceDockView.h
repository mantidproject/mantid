#ifndef MANTID_MANTIDWIDGETS_WORKSPACEDOCKVIEW_H_
#define MANTID_MANTIDWIDGETS_WORKSPACEDOCKVIEW_H_

#include "MantidQtMantidWidgets/WorkspacePresenter/IWorkspaceDockView.h"
#include <QDockWidget>
#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace MantidWidgets {

/**
\class  WorkspaceDockView
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
class WorkspaceDockView : public IWorkspaceDockView, public QDockWidget {
public:
  explicit WorkspaceDockView();
  ~WorkspaceDockView() override;

  void init() override;
  WorkspacePresenter_wptr getPresenterWeakPtr() override;
  WorkspacePresenter_sptr getPresenterSharedPtr() override;

  std::string getSelectedWorkspaceName() const override;
  Mantid::API::Workspace_sptr getSelectedWorkspace() const override;
  void showLoadDialog() override;
  bool deleteConfirmation() const override;
  void deleteWorkspaces() override;
  void renameWorkspace() override;
  void updateTree(
      const std::map<std::string, Mantid::API::Workspace_sptr> &items) override;

private:
  void populateTopLevel(
      const std::map<std::string, Mantid::API::Workspace_sptr> &topLevelItems,
      const StringList &expanded);

private:
  WorkspacePresenter_sptr presenter;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTID_MANTIDWIDGETS_WORKSPACEDOCKVIEW_H_