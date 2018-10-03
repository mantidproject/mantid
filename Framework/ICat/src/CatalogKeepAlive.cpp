// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidICat/CatalogKeepAlive.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidKernel/DateAndTime.h"

#include <Poco/Thread.h>

namespace Mantid {
namespace ICat {
DECLARE_ALGORITHM(CatalogKeepAlive)

void CatalogKeepAlive::init() {
  declareProperty("Session", "",
                  "The session information of the catalog to use.");
  declareProperty<int>(
      "TimePeriod", 1200,
      "Frequency to refresh session in seconds. Default 1200 (20 minutes).",
      Kernel::Direction::Input);
}

void CatalogKeepAlive::exec() {
  int timePeriod = getProperty("TimePeriod");
  if (timePeriod <= 0)
    throw std::runtime_error("TimePeriod must be greater than zero.");

  Types::Core::DateAndTime lastTimeExecuted =
      Types::Core::DateAndTime::getCurrentTime();

  // Keep going until cancelled
  while (true) {
    Poco::Thread::sleep(1000);

    // Exit if the user presses cancel
    this->interruption_point();

    double secondsSinceExecuted = Types::Core::DateAndTime::secondsFromDuration(
        Types::Core::DateAndTime::getCurrentTime() - lastTimeExecuted);

    if (secondsSinceExecuted > timePeriod) {
      API::CatalogManager::Instance()
          .getCatalog(getPropertyValue("Session"))
          ->keepAlive();
      lastTimeExecuted = Types::Core::DateAndTime::getCurrentTime();
    }
  }
}
} // namespace ICat
} // namespace Mantid
