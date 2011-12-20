/*WIKI* 

This algorithm uses a numerical integration method to calculate attenuation factors resulting from absorption and single scattering within a cuboid region of a sample with the dimensions and material properties given. '''The gauge volume generated will be an axis-aligned cuboid centred on the sample (centre) position. The sample must fully enclose this cuboid. If this does not meet your needs you can instead use the general [[AbsorptionCorrection]] algorithm in conjunction with [[DefineGaugeVolume]].'''

Factors are calculated for each spectrum (i.e. detector position) and wavelength point, as defined by the input workspace. The sample is divided up into cuboids having sides of as close to the size given in the ElementSize property as the sample dimensions will allow. Thus the calculation speed depends linearly on the total number of bins in the workspace and goes as <math>\rm{ElementSize}^{-3}</math>.

Path lengths through the sample are then calculated for the centre-point of each element and a numerical integration is carried out using these path lengths over the volume elements.

==== Restrictions on the input workspace ====
The input workspace must have units of wavelength. The [[instrument]] associated with the workspace must be fully defined because detector, source & sample position are needed. A sample shape must have been defined using, e.g., [[CreateSampleShape]] and the gauge volume must be fully within the sample.



*WIKI*/
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

/// Sets documentation strings for this algorithm
void CuboidGaugeVolumeAbsorption::initDocs()
{
  this->setWikiSummary("Calculates bin-by-bin correction factors for attenuation due to absorption and (single) scattering within a cuboid shaped 'gauge volume' of a generic sample. The sample shape can be defined by, e.g., the [[CreateSampleShape]] algorithm. ");
  this->setOptionalMessage("Calculates bin-by-bin correction factors for attenuation due to absorption and (single) scattering within a cuboid shaped 'gauge volume' of a generic sample. The sample shape can be defined by, e.g., the CreateSampleShape algorithm.");
}


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

void CuboidGaugeVolumeAbsorption::initialiseCachedDistances()
{
  // Just call the superclass method within a try-catch to fail the algorithm gracefully
  // if the sample object does not fully enclose the gauge volume requested
  try
  {
    FlatPlateAbsorption::initialiseCachedDistances();
  } catch (Exception::InstrumentDefinitionError &) {
    // Create and throw a new exception with a more specific message
    throw Exception::InstrumentDefinitionError(
        "This algorithm requires that the sample fully encloses the requested gauge volume.");
  }
}

} // namespace Algorithms
} // namespace Mantid
