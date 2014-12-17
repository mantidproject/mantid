#include "MantidAlgorithms/Logarithm.h"
#include "MantidAPI/WorkspaceProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

#include <cmath>
namespace Mantid {
namespace Algorithms {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(Logarithm)

Logarithm::Logarithm() : UnaryOperation(), log_Min(0), is_natural(true) {
  this->useHistogram = true;
}

void Logarithm::defineProperties() {
  declareProperty("Filler", 0.0, "The value that will be placed into the "
                                 "output workspace if an input value is equal "
                                 "or less than 0. Default value is 0");
  declareProperty("Natural", true, "Logical value which specifies if user "
                                   "wands to calculate natural or base 10 "
                                   "logarithm.");
}

void Logarithm::retrieveProperties() {
  this->log_Min = getProperty("Filler");
  this->is_natural = getProperty("Natural");
}

void Logarithm::performUnaryOperation(const double XIn, const double YIn,
                                      const double EIn, double &YOut,
                                      double &EOut) {
  (void)XIn; // Avoid compiler warning
  if (YIn <= 0) {
    YOut = this->log_Min;
    EOut = 0;
  } else {
    if (this->is_natural) {
      YOut = std::log(YIn);
      EOut = EIn / YIn;
    } else {
      YOut = std::log10(YIn);
      EOut = 0.434 * EIn / YIn;
    }
  }
}

} // End Namespace Algorithms
} // End Namespace Mantid
