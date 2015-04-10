#include "MantidAlgorithms/Segfault.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Segfault)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
Segfault::Segfault() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
Segfault::~Segfault() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string Segfault::name() const { return "Segfault"; }

/// Algorithm's version for identification. @see Algorithm::version
int Segfault::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string Segfault::category() const { return "Utility\\Development"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string Segfault::summary() const {
  return "WARNING: THIS CRASHES MANTID";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void Segfault::init() {
  declareProperty("DryRun", true,
                  "Just log to the error channel but don't crash mantid");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void Segfault::exec() {
  bool dryrun = getProperty("DryRun");
  g_log.error("Crashing mantid now");

  if (!dryrun) {
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++11-compat-deprecated-writable-strings"
#pragma clang diagnostic ignored "-Wwritable-strings"
#endif
    // writing to read-only memory
    char *s = "hello world";
    *s = 'H';
#if __clang__
#pragma clang diagnostic pop
#endif
  }
}

} // namespace Algorithms
} // namespace Mantid
