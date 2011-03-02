//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AnyShapeAbsorption.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AnyShapeAbsorption)

/// Sets documentation strings for this algorithm
void AnyShapeAbsorption::initDocs()
{
  this->setWikiSummary("Calculates an approximation of the attenuation due to absorption and single scattering in a generic sample shape. The sample shape can be defined by, e.g., the [[CreateSampleShape]] algorithm.  '''Note that if your sample is of cuboid or cylinder geometry, you will get a more accurate result from the [[FlatPlateAbsorption]] or [[CylinderAbsorption]] algorithms respectively.''' ");
  this->setOptionalMessage("Calculates an approximation of the attenuation due to absorption and single scattering in a generic sample shape. The sample shape can be defined by, e.g., the CreateSampleShape algorithm.  'Note that if your sample is of cuboid or cylinder geometry, you will get a more accurate result from the FlatPlateAbsorption or CylinderAbsorption algorithms respectively.'");
}


using namespace Kernel;
using namespace Geometry;
using namespace API;

AnyShapeAbsorption::AnyShapeAbsorption() : AbsorptionCorrection(), m_cubeSide(0.0)
{
}

void AnyShapeAbsorption::defineProperties()
{
  BoundedValidator<double> *moreThanZero = new BoundedValidator<double> ();
  moreThanZero->setLower(0.001);
  declareProperty("ElementSize", 1.0, moreThanZero, 
    "The size of one side of an integration element cube in mm");
}

/// Fetch the properties and set the appropriate member variables
void AnyShapeAbsorption::retrieveProperties()
{
  m_cubeSide = getProperty("ElementSize"); // in mm
  m_cubeSide *= 0.001; // now in m
}

std::string AnyShapeAbsorption::sampleXML()
{
  // Returning an empty string signals to the base class that it should
  // use the object already attached to the sample.
  return std::string();
}

/// Calculate the distances for L1 and element size for each element in the sample
void AnyShapeAbsorption::initialiseCachedDistances()
{
  // First, check if a 'gauge volume' has been defined. If not, it's the same as the sample.
  Object integrationVolume = *m_sampleObject;
  if ( m_inputWS->run().hasProperty("GaugeVolume") )
  {
    integrationVolume = constructGaugeVolume();
  }

  // Construct the trial set of elements from the object's bounding box
  const double big(10.0); // Seems like bounding box code searches inwards, 10m should be enough!
  double minX(-big), maxX(big), minY(-big), maxY(big), minZ(-big), maxZ(big);
  integrationVolume.getBoundingBox(maxX,maxY,maxZ,minX,minY,minZ);
  assert(maxX > minX);
  assert(maxY > minY);
  assert(maxZ > minZ);

  const double xLength = maxX - minX;
  const double yLength = maxY - minY;
  const double zLength = maxZ - minZ;
  const int numXSlices = static_cast<int>(xLength/m_cubeSide);
  const int numYSlices = static_cast<int>(yLength/m_cubeSide);
  const int numZSlices = static_cast<int>(zLength/m_cubeSide);
  const double XSliceThickness = xLength/numXSlices;
  const double YSliceThickness = yLength/numYSlices;
  const double ZSliceThickness = zLength/numZSlices;

  m_numVolumeElements = numXSlices * numYSlices * numZSlices;

  try
  {
    m_L1s.reserve(m_numVolumeElements);
    m_elementVolumes.reserve(m_numVolumeElements);
    m_elementPositions.reserve(m_numVolumeElements);
  } catch (...) {
    // Typically get here if the number of volume elements is too large
    // Provide a bit more information
    g_log.error("Too many volume elements requested - try increasing the value of the ElementSize property.");
    throw;
  }
  
  for (int i = 0; i < numZSlices; ++i)
  {
    const double z = (i + 0.5) * ZSliceThickness - 0.5 * zLength;
    
    for (int j = 0; j < numYSlices; ++j)
    {
      const double y = (j + 0.5) * YSliceThickness - 0.5 * yLength;
      
      for (int k = 0; k < numXSlices; ++k)
      {
        const double x = (k + 0.5) * XSliceThickness - 0.5 * xLength;
        // Set the current position in the sample in Cartesian coordinates.
        const V3D currentPosition(x,y,z);
        // Check if the current point is within the object. If not, skip.
        if ( integrationVolume.isValid(currentPosition) )
        {
          // Create track for distance in sample before scattering point
          Track incoming(currentPosition, m_beamDirection*-1.0);
          // We have an issue where occasionally, even though a point is within
          // the object a track segment to the surface isn't correctly created.
          // In the context of this algorithm I think it's safe to just chuck away
          // the element in this case.
          // This will also throw away points that are inside a gauge volume but outside the sample
          if ( m_sampleObject->interceptSurface(incoming) > 0 )
          {
            m_L1s.push_back(incoming.begin()->distFromStart);
            m_elementPositions.push_back(currentPosition);
            // Also calculate element volume here
            m_elementVolumes.push_back(XSliceThickness*YSliceThickness*ZSliceThickness);
          }
        }
      }
    }
  }

  // Record the number of elements we ended up with
  m_numVolumeElements = m_L1s.size();
  m_sampleVolume = m_numVolumeElements * XSliceThickness * YSliceThickness * ZSliceThickness;
}

Geometry::Object AnyShapeAbsorption::constructGaugeVolume()
{
  g_log.information("Calculating scattering within the gauge volume defined on the input workspace");

  // Retrieve and create the gauge volume shape
  boost::shared_ptr<const Geometry::Object> volume = ShapeFactory().createShape(m_inputWS->run().getProperty("GaugeVolume")->value());
  // Although DefineGaugeVolume algorithm will have checked validity of XML, do so again here
  if ( !(volume->topRule()) && volume->getSurfacePtr().empty() )
  {
    g_log.error("Invalid gauge volume definition. Unable to construct integration volume.");
    throw std::invalid_argument("Invalid gauge volume definition.");
  }

  return *volume;
}

} // namespace Algorithms
} // namespace Mantid
