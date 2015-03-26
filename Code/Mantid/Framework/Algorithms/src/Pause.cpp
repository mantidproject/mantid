#include "MantidAlgorithms/Pause.h"
#include "MantidAPI/Algorithm.h"

#include <Poco/Thread.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Pause)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
Pause::Pause() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
Pause::~Pause() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string Pause::name() const { return "Pause"; }

/// Algorithm's version for identification. @see Algorithm::version
int Pause::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string Pause::category() const { return "Utility\\Development"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void Pause::init() {
  declareProperty("Duration", 1.0,
                  "How long to pause, in seconds. Default 1.\n"
                  "Enter a negative number to pause forever until cancelled.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void Pause::exec() {
  DateAndTime startTime = DateAndTime::getCurrentTime();
  double Duration = getProperty("Duration");

  // Keep going until you get cancelled
  while (true) {
    bool breakOut = false;
    try {
      // This call throws if the user presses cancel
      this->interruption_point();
    } catch (CancelException &) {
      // Break out of the lo
      breakOut = true;
      g_log.notice() << "User stopped the Pause." << std::endl;
    }
    if (breakOut)
      break;

    // Sleep for 50 msec
    Poco::Thread::sleep(50);

    DateAndTime now = DateAndTime::getCurrentTime();
    double seconds = DateAndTime::secondsFromDuration(now - startTime);

    if (Duration > 0) {
      // Break when you've waited long enough
      if (seconds > Duration)
        break;
      // Report progress for non-infinite runs
      this->progress(seconds / Duration);
    }
  }
}

} // namespace Mantid
} // namespace Algorithms
