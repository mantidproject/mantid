
#ifndef _EVENT_HANDLER_H_
#define _EVENT_HANDLER_H_

#include <Poco/ActiveMethod.h>
#include <Poco/NotificationCenter.h>
#include <Poco/Notification.h>
#include <Poco/NObserver.h>
#include <MantidAPI/Algorithm.h>
namespace Mantid
{
namespace VATES
{

/** Abstract update event type. Allows notification of visualisation top layer/viewer about events without specifying a concrete interactor.

 @author Owen Arnold, Tessella plc
 @date 10/02/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

class DLLExport ProgressAction
{

public:

  /// Constructor
  ProgressAction();

  /// Handle event updates.
  virtual void eventRaised(int progressPercent) = 0;

  void handler(const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification>& pNf);

};
}
}
#endif
