#include "MantidAlgorithms/CorrectFlightPaths.h"
#include "MantidAlgorithms/ConvertToConstantL2.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CorrectFlightPaths)

/// Constructor
CorrectFlightPaths::CorrectFlightPaths() : ConvertToConstantL2() { this->useAlgorithm("ConvertToConstantL2", 1); }

} // namespace Algorithm
} // namespace Mantid
