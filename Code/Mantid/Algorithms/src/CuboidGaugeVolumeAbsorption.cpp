//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CuboidGaugeVolumeAbsorption.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CuboidGaugeVolumeAbsorption)

using namespace Kernel;
using namespace Geometry;
using namespace API;

CuboidGaugeVolumeAbsorption::CuboidGaugeVolumeAbsorption() : FlatPlateAbsorption()
{
}

std::string CuboidGaugeVolumeAbsorption::sampleXML()
{
  // Returning an empty string signals to the base class that it should
  // use the object already attached to the sample.
  return std::string();
}

} // namespace Algorithms
} // namespace Mantid
