//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CylinderAbsorption.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CylinderAbsorption)

using namespace Kernel;
using namespace Geometry;
using namespace API;

CylinderAbsorption::CylinderAbsorption() : AbsorptionCorrection(),
  m_cylHeight(0.0), m_cylRadius(0.0),
  m_numSlices(0),m_sliceThickness(0),m_numAnnuli(0),m_deltaR(0)
{
}

void CylinderAbsorption::defineProperties()
{
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double> ();
  mustBePositive->setLower(0.0);
  declareProperty("CylinderSampleHeight", -1.0, mustBePositive,
    "The height of the cylindrical sample in centimetres");
  declareProperty("CylinderSampleRadius", -1.0, mustBePositive->clone(),
    "The radius of the cylindrical sample in centimetres");

  BoundedValidator<int> *positiveInt = new BoundedValidator<int> ();
  positiveInt->setLower(1);
  declareProperty("NumberOfSlices", 1, positiveInt, 
    "The number of slices into which the cylinder is divided for the\n"
    "calculation");
  declareProperty("NumberOfAnnuli", 1, positiveInt->clone(), 
    "The number of annuli into which each slice is divided for the\n"
    "calculation");
}

/// Fetch the properties and set the appropriate member variables
void CylinderAbsorption::retrieveProperties()
{
  m_cylHeight = getProperty("CylinderSampleHeight"); // in cm
  m_cylRadius = getProperty("CylinderSampleRadius"); // in cm
  m_cylHeight *= 0.01;  // now in m 
  m_cylRadius *= 0.01;  // now in m

  m_numSlices = getProperty("NumberOfSlices");
  m_sliceThickness = m_cylHeight / m_numSlices;
  m_numAnnuli = getProperty("NumberOfAnnuli");
  m_deltaR = m_cylRadius / m_numAnnuli;

  /* The number of volume elements is
   * numslices*(1+2+3+.....+numAnnuli)*6
   * Since the first annulus is separated in 6 segments, the next one in 12 and so on.....
   */
  m_numVolumeElements = m_numSlices * m_numAnnuli * (m_numAnnuli + 1) * 3;

  if( m_numVolumeElements == 0 )
  {
    g_log.error() << "Input properties lead to no defined volume elements.\n";
    throw std::runtime_error("No volume elements defined.");
  }

  m_sampleVolume = m_cylHeight * M_PI * m_cylRadius * m_cylRadius;

  if( m_sampleVolume == 0.0 )
  {
    g_log.error() << "Defined sample has zero volume.\n";
    throw std::runtime_error("Sample with zero volume defined.");
  }
}

std::string CylinderAbsorption::sampleXML()
{
  std::ostringstream xmlShapeStream;
  xmlShapeStream 
    << "<cylinder id=\"detector-shape\"> " 
    << "<centre-of-bottom-base x=\"0.0\" y=\"" << -0.5*m_cylHeight << "\" z=\"0.0\" /> "
    << "<axis x=\"0\" y=\"1\" z=\"0\" /> " 
    << "<radius val=\"" << m_cylRadius << "\" /> "
    << "<height val=\"" << m_cylHeight << "\" /> "
    << "</cylinder>";

  return xmlShapeStream.str();
}

/// Calculate the distances for L1 and element size for each element in the sample
void CylinderAbsorption::initialiseCachedDistances()
{
  m_L1s.resize(m_numVolumeElements);
  m_elementVolumes.resize(m_numVolumeElements);
  m_elementPositions.resize(m_numVolumeElements);
  
  int counter = 0;
  // loop over slices
  for (int i = 0; i < m_numSlices; ++i)
  {
    const double z = (i + 0.5) * m_sliceThickness - 0.5 * m_cylHeight;

    // Number of elements in 1st annulus
    int Ni = 0;
    // loop over annuli
    for (int j = 0; j < m_numAnnuli; ++j)
    {
      Ni += 6;
      const double R = (j * m_cylRadius / m_numAnnuli) + (m_deltaR / 2.0);
      // loop over elements in current annulus
      for (int k = 0; k < Ni; ++k)
      {
        const double phi = 2* M_PI * k / Ni;
        // Calculate the current position in the sample in Cartesian coordinates.
        // Remember that our cylinder has its axis along the y axis
        m_elementPositions[counter](R * sin(phi), z, R * cos(phi));
        assert(m_sampleObject->isValid(m_elementPositions[counter]));
        // Create track for distance in cylinder before scattering point
        Track incoming(m_elementPositions[counter], m_beamDirection*-1.0);

        m_sampleObject->interceptSurface(incoming);
        m_L1s[counter] = incoming.begin()->distFromStart;

        // Also calculate element volumes here
        const double outerR = R + (m_deltaR / 2.0);
        const double innerR = outerR - m_deltaR;
        const double elementVolume = M_PI * (outerR * outerR - innerR * innerR) * m_sliceThickness / Ni;
        m_elementVolumes[counter] = elementVolume;
 
        counter++;
      }
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
