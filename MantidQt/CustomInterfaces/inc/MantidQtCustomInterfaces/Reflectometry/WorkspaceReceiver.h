#ifndef MANTID_CUSTOMINTERFACES_WORKSPACERECEIVER_H
#define MANTID_CUSTOMINTERFACES_WORKSPACERECEIVER_H

#include "MantidQtCustomInterfaces/Reflectometry/ReflCommandBase.h"
#include <set>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
/** @class WorkspaceReceiver

WorkspaceReceiver is an interface that defines the functions needed to receive
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
class WorkspaceReceiver {
public:
  virtual ~WorkspaceReceiver(){};

  enum Flag { ADSChangedFlag };

  // Notify this receiver that something changed in the ADS
  virtual void notify(WorkspaceReceiver::Flag flag) = 0;
};
}
}
#endif /*MANTID_CUSTOMINTERFACES_WORKSPACERECEIVER_H*/