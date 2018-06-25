#ifndef MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWVIEW_H
#define MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWVIEW_H

#include <string>
#include <vector>
#include "IReflBatchView.h"

namespace MantidQt {
namespace CustomInterfaces {

/** @class IReflMainWindowView

IReflMainWindowView is the interface defining the functions that the main
window view needs to implement. It is empty and not necessary at the moment, but
can be used in the future if widgets common to all tabs are added, for instance,
the help button.

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
class ReflMainWindowSubscriber {
public:
  virtual void notifyHelpPressed() = 0;
  virtual void notifyNewBatchRequested() = 0;
  virtual void notifyCloseBatchRequested(int batchIndex) = 0;
  virtual ~ReflMainWindowSubscriber() = default;
};

class IReflMainWindowView {
public:
  virtual void subscribe(ReflMainWindowSubscriber *notifyee) = 0;
  virtual IReflBatchView *newBatch() = 0;
  virtual void removeBatch(int index) = 0;
  virtual std::vector<IReflBatchView *> batches() const = 0;
  virtual std::string runPythonAlgorithm(const std::string &pythonCode) = 0;
  virtual ~IReflMainWindowView() = default;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWVIEW_H */
