//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CorrectForAttenuation.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Cylinder.h"
#include "MantidGeometry/Plane.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CorrectForAttenuation)

using namespace Kernel;
using namespace Geometry;
using namespace API;

// Get a reference to the logger. It is used to print out information, warning and error messages
Logger& CorrectForAttenuation::g_log = Logger::get("CorrectForAttenuation");

void CorrectForAttenuation::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,new WorkspaceUnitValidator<>("Wavelength")));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0.0);
  declareProperty("CylinderSampleHeight",-1.0,mustBePositive);
  declareProperty("CylinderSampleRadius",-1.0,mustBePositive->clone());
  declareProperty("AttenuationXSection",-1.0,mustBePositive->clone());
  declareProperty("ScatteringXSection",-1.0,mustBePositive->clone());
  declareProperty("SampleNumberDensity",-1.0,mustBePositive->clone());
  BoundedValidator<int> *positiveInt = new BoundedValidator<int>();
  positiveInt->setLower(1);
  declareProperty("NumberOfSlices",1,positiveInt);
  declareProperty("NumberOfAnnuli",1,positiveInt->clone());
}

void CorrectForAttenuation::exec()
{
  // Retrieve the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Get the input parameters
  this->retrieveProperties();
  const double cylinder_volume=m_cylHeight*M_PI*m_cylRadius*m_cylRadius;
  this->constructCylinderSample();

  // Create the output workspace
  MatrixWorkspace_sptr correctionFactors = WorkspaceFactory::Instance().create(inputWS);
  correctionFactors->setYUnit("Attenuation factor");

  const int numHists = inputWS->getNumberHistograms();
  const int specSize = inputWS->blocksize();
  const bool isHist = inputWS->isHistogramData();
  int iprogress_step = numHists / 100;
  if (iprogress_step == 0) iprogress_step = 1;
  // Loop over the spectra
  for (int i = 0; i < numHists; ++i)
  {
    // Copy over bin boundaries
    const std::vector<double> &X = inputWS->readX(i);
    correctionFactors->dataX(i) = X;

    // Get detector position
    V3D detPos;
    try {
      detPos = inputWS->getDetector(i)->getPos();
      // Need to convert detPos from metres to cm
      detPos *= 100.0;
    } catch (Exception::NotFoundError) {
      // Catch when a spectrum doesn't have an attached detector and go to next one
      continue;
    }

    this->calculateDistances(detPos);

    // Get a reference to the Y's in the output WS for storing the factors
    std::vector<double> &Y = correctionFactors->dataY(i);

    // Loop through the bins in the current spectrum
    for (int j = 0; j < specSize; ++j)
    {
      const double lambda = ( isHist ? (0.5*(X[j]+X[j+1])) : X[j] );
      Y[j] = this->doIntegration(lambda);
      Y[j]/= cylinder_volume; // Divide by total volume of the cylinder
    }
    // Element-detector distances are different for each spectrum (i.e. detector)
    m_L2s.clear();

    if ( i % iprogress_step == 0)
    {
        progress( double(i)/numHists );
        interruption_point();
    }

  }

  g_log.information() << "Total number of elements in the integration was " << m_L1s.size() << std::endl;
  // Clear these out so it's not taking up memory after the algorithm is finished
  m_L1s.clear();
  m_elementVolumes.clear();

  setProperty("OutputWorkspace",correctionFactors);
}

/// Fetch the properties and set the appropriate member variables
void CorrectForAttenuation::retrieveProperties()
{
  m_cylHeight = getProperty("CylinderSampleHeight"); // in cm
  m_cylRadius = getProperty("CylinderSampleRadius"); // in cm
  const double sigma_atten = getProperty("AttenuationXSection");   // in barns
  const double sigma_s = getProperty("ScatteringXSection");         // in barns
  const double rho = getProperty("SampleNumberDensity");        // in Angstroms-3
  m_refAtten = sigma_atten*rho;
  m_scattering = sigma_s*rho;
}

/// Create the cylinder object using the Geometry classes
void CorrectForAttenuation::constructCylinderSample()
{
  std::map<int,Surface*> surfaces;

  const V3D normVec(0.0,1.0,0.0);

  Cylinder* cyl = new Cylinder();
  // For now, assume beam comes in along z axis, that y is up and that sample is at origin
  cyl->setCentre(V3D(0.0,0.0,0.0));
  cyl->setNorm(normVec);
  cyl->setRadius(m_cylRadius);
  surfaces[1] = cyl;

  Plane* top = new Plane();
  V3D pointInPlane = normVec * (m_cylHeight/2.0);
  top->setPlane(pointInPlane,normVec);
  surfaces[2] = top;
  Plane* bottom = new Plane();
  pointInPlane[1] *= -1.0;
  bottom->setPlane(pointInPlane,normVec);
  surfaces[3] = bottom;

  int success = m_cylinderSample.setObject(21,"-1 -2 3");
  assert( success );
  success = m_cylinderSample.populate(surfaces);
  assert( !success );

  const double radius = m_cylRadius;
  assert( m_cylinderSample.isValid(V3D(0.0,0.0,0.0)) );
  assert( ! m_cylinderSample.isValid(V3D(radius+0.001,0.0,0.0)) );
  assert( m_cylinderSample.isValid(V3D(radius-0.001,0.0,0.0)) );
  assert( ! m_cylinderSample.isValid(V3D(0.0, m_cylHeight, 0.0)) );
  assert( ! m_cylinderSample.isValid(V3D(0.0, -1.0*m_cylHeight, 0.0)) );
  assert( m_cylinderSample.isOnSide(V3D(radius,0.0,0.0)) );
  assert( m_cylinderSample.isOnSide(V3D(0.0,m_cylHeight/2.0,0.0)) );
  assert( m_cylinderSample.isOnSide(V3D(radius,m_cylHeight/2.0,0.0)) );

  g_log.information("Successfully constructed the sample object");
}

/// Calculate the distances traversed by the neutrons within the sample
void CorrectForAttenuation::calculateDistances(const Geometry::V3D& detectorPos)
{
  // Test whether I need to calculate distances
  const bool calculateL1s = m_L1s.empty();

  const int numSlices = getProperty("NumberOfSlices");
  const double sliceThickness = m_cylHeight/numSlices;
  const int numAnnuli = getProperty("NumberOfAnnuli");;
  const double deltaR = m_cylRadius/numAnnuli;

  // loop over slices
  for (int i = 0; i < numSlices; ++i)
  {
    const double z = (i+0.5)*sliceThickness - 0.5*m_cylHeight;

    // Number of elements in 1st annulus
    int Ni = 0;
    // loop over annuli
    for (int j = 0; j < numAnnuli; ++j)
    {
      Ni += 6;
      const double R = (j*m_cylRadius/numAnnuli)+(deltaR/2.0);
      // loop over elements in current annulus
      for (int k = 0; k < Ni; ++k)
      {
        const double phi = 2*M_PI*k/Ni;
        // Calculate the current position in the sample in cartesian coordinates.
        // Remember that our cylinder has its axis along the y axis
        V3D currentPosition(R*sin(phi),z,R*cos(phi));
        assert( m_cylinderSample.isValid(currentPosition) );

        // These need only be calculated once
        if (calculateL1s)
        {
          // Create track for distance in cylinder before scattering point
          // Remember beam along Z direction
          Track incoming(currentPosition,V3D(0.0,0.0,-1.0));
          int tmp = m_cylinderSample.interceptSurface(incoming);
          assert( tmp == 1 );
          m_L1s.push_back( incoming.begin()->Dist );

          // Also calculate element volumes here
          const double outerR = R + (deltaR/2.0);
          const double innerR = R - (deltaR/2.0);
          const double elementVolume = M_PI*(outerR*outerR-innerR*innerR)*sliceThickness/Ni;
          m_elementVolumes.push_back(elementVolume);
        }

        // Create track for distance in cylinder between scattering point and detector
        V3D direction = currentPosition - detectorPos;
        direction.normalize();
        Track outgoing(currentPosition,direction);
        int tmp = m_cylinderSample.interceptSurface(outgoing);
        assert( tmp == 1 );
        m_L2s.push_back( outgoing.begin()->Dist );
      }
    }
  }
}

/// Carries out the numerical integration over the cylinder
double CorrectForAttenuation::doIntegration(const double& lambda)
{
  double integral = 0.0;
  double exponent;

  assert( m_L1s.size() == m_L2s.size() );
  assert( m_L1s.size() == m_elementVolumes.size() );
  std::vector<double>::const_iterator l1it;
  std::vector<double>::const_iterator l2it = m_L2s.begin();
  std::vector<double>::const_iterator elit = m_elementVolumes.begin();

  // Iterate over all the elements, summing up the integral
  for (l1it = m_L1s.begin(); l1it != m_L1s.end(); ++l1it, ++l2it, ++elit)
  {
    // Equation is exponent * element volume
    // where exponent is e^(-mu * wavelength/1.8 * (L1+L2) )  (N.B. distances are in cm)
    exponent = -1.0 * ((m_refAtten * lambda / 1.798) + m_scattering ) * ( *l1it + *l2it );
    integral += ( exp(exponent) * (*elit) );
  }

  return integral;
}

} // namespace Algorithms
} // namespace Mantid

