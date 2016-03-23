#ifndef MANTID_CUSTOMINTERFACES_IREFLOUTERPRESENTER_H
#define MANTID_CUSTOMINTERFACES_IREFLOUTERPRESENTER_H

#include <map>
#include <string>
#include <vector>

#include "MantidKernel/System.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class IReflOuterPresenter

IReflOuterPresenter is an interface that defines the functions needed to receive
information from a table presenter. IReflTablePresenter uses this interface
to notify changes to an outer, concrete presenter. Any outer presenter that
needs to receive information from IReflTablePresenter should inherit from this
class.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class IReflOuterPresenter {
public:
  virtual ~IReflOuterPresenter(){};

  // Update the presenter with the list of workspaces the user can open
  virtual void
  pushWorkspaceList(const std::set<std::string> &workspaceList) = 0;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_IREFLOUTERPRESENTER_H*/