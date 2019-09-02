// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef _EVENT_HANDLER_H_
#define _EVENT_HANDLER_H_

#include "MantidAPI/Algorithm.h"
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
