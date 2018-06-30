
#ifndef _EVENT_HANDLER_H_
#define _EVENT_HANDLER_H_

#include <MantidAPI/Algorithm.h>
#include <Poco/ActiveMethod.h>
#include <Poco/NObserver.h>
#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>
namespace Mantid {
namespace VATES {

/** Abstract update event type. Allows notification of visualisation top
 layer/viewer about events without specifying a concrete interactor.

 @author Owen Arnold, Tessella plc
 @date 10/02/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

class DLLExport ProgressAction {

public:
  /// Constructor
  ProgressAction();

  /// Handle event updates.
  virtual void eventRaised(double progress) = 0;

  void handler(
      const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification> &pNf);
};
} // namespace VATES
} // namespace Mantid
#endif
