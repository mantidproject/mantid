#ifndef MANTID_MANTIDWIDGETS_WORKSPACEPROVIDER_H_
#define MANTID_MANTIDWIDGETS_WORKSPACEPROVIDER_H_

#include <MantidAPI/Workspace_fwd.h>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <map>

namespace MantidQt {
namespace MantidWidgets {

class WorkspaceProviderNotifiable;

using Presenter_sptr = boost::shared_ptr<WorkspaceProviderNotifiable>;
using Presenter_wptr = boost::weak_ptr<WorkspaceProviderNotifiable>;

/**
\class  WorkspaceProvider
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
class WorkspaceProvider {
public:
  virtual ~WorkspaceProvider() = default;

  virtual void registerPresenter(Presenter_wptr presenter) = 0;
  virtual bool doesWorkspaceExist(const std::string &wsname) const = 0;
  virtual std::map<std::string, Mantid::API::Workspace_sptr>
  topLevelItems() const = 0;
  virtual std::string getOldName() const = 0;
  virtual std::string getNewName() const = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTID_MANTIDWIDGETS_WORKSPACEPROVIDER_H_