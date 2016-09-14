#ifndef MANTID_MANTIDWIDGETS_IWORKSPACEDOCKVIEW_H_
#define MANTID_MANTIDWIDGETS_IWORKSPACEDOCKVIEW_H_

#include <MantidAPI/Workspace_fwd.h>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <map>

namespace MantidQt {
namespace MantidWidgets {

class WorkspaceProviderNotifiable;
class WorkspacePresenter;

using WorkspacePresenter_wptr = boost::weak_ptr<WorkspaceProviderNotifiable>;
using WorkspacePresenter_sptr = boost::shared_ptr<WorkspacePresenter>;
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
class IWorkspaceDockView
    : public boost::enable_shared_from_this<IWorkspaceDockView> {
public:
  enum class SortDirection { Ascending, Descending };
  enum class SortCriteria { ByName, ByLastModified };

  virtual ~IWorkspaceDockView() = default;

  virtual void init() = 0;
  virtual WorkspacePresenter_wptr getPresenterWeakPtr() = 0;
  virtual WorkspacePresenter_sptr getPresenterSharedPtr() = 0;

  virtual void showLoadDialog() = 0;
  virtual void showRenameDialog(const StringList &names) const = 0;
  virtual void groupWorkspaces(const StringList &names) const = 0;
  virtual void ungroupWorkspaces(const StringList &names) const = 0;
  virtual bool deleteConfirmation() const = 0;
  virtual void deleteWorkspaces() = 0;
  virtual SortDirection getSortDirection() const = 0;
  virtual SortCriteria getSortCriteria() const = 0;
  virtual void sortWorkspaces(SortCriteria criteria,
                              SortDirection direction) = 0;
  virtual StringList getSelectedWorkspaceNames() const = 0;
  virtual Mantid::API::Workspace_sptr getSelectedWorkspace() const = 0;
  virtual void updateTree(
      const std::map<std::string, Mantid::API::Workspace_sptr> &items) = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTID_MANTIDWIDGETS_IWORKSPACEDOCKVIEW_H_