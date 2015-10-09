//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CuboidGaugeVolumeAbsorption.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CuboidGaugeVolumeAbsorption)

using namespace Kernel;
using namespace Geometry;
using namespace API;

CuboidGaugeVolumeAbsorption::CuboidGaugeVolumeAbsorption()
    : FlatPlateAbsorption() {}

std::string CuboidGaugeVolumeAbsorption::sampleXML() {
  // Returning an empty string signals to the base class that it should
  // use the object already attached to the sample.
  return std::string();
}

void CuboidGaugeVolumeAbsorption::initialiseCachedDistances() {
  // Just call the superclass method within a try-catch to fail the algorithm
  // gracefully
  // if the sample object does not fully enclose the gauge volume requested
  try {
    FlatPlateAbsorption::initialiseCachedDistances();
  } catch (Exception::InstrumentDefinitionError &) {
    // Create and throw a new exception with a more specific message
    throw Exception::InstrumentDefinitionError("This algorithm requires that "
                                               "the sample fully encloses the "
                                               "requested gauge volume.");
  }
}

} // namespace Algorithms
} // namespace Mantid
