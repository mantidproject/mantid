// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/Pause.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/DateAndTime.h"

#include <Poco/Thread.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Pause)

/// Algorithm's name for identification. @see Algorithm::name
const std::string Pause::name() const { return "Pause"; }

/// Algorithm's version for identification. @see Algorithm::version
int Pause::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string Pause::category() const { return "Utility\\Development"; }

/** Initialize the algorithm's properties.
 */
void Pause::init() {
  declareProperty("Duration", 1.0,
                  "How long to pause, in seconds. Default 1.\n"
                  "Enter a negative number to pause forever until cancelled.");
}

/** Execute the algorithm.
 */
void Pause::exec() {
  DateAndTime startTime = DateAndTime::getCurrentTime();
  const double duration = getProperty("Duration");

  // Keep going until you get cancelled
  while (true) {
    bool breakOut = false;
    try {
      // This call throws if the user presses cancel
      this->interruption_point();
    } catch (CancelException &) {
      // Break out of the lo
      breakOut = true;
      g_log.notice() << "User stopped the Pause.\n";
    }
    if (breakOut)
      break;

    // Sleep for 50 msec
    Poco::Thread::sleep(50);

    DateAndTime now = DateAndTime::getCurrentTime();
    double seconds = DateAndTime::secondsFromDuration(now - startTime);

    if (duration > 0) {
      // Break when you've waited long enough
      if (seconds > duration)
        break;
      // Report progress for non-infinite runs
      this->progress(seconds / duration, "", duration - seconds);
    }
  }
}

} // namespace Mantid::Algorithms
