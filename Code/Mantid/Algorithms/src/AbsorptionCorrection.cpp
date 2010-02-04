//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AbsorptionCorrection.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Fast_Exponential.h"

namespace Mantid
{
namespace Algorithms
{

using namespace Kernel;
using namespace Geometry;
using namespace API;

AbsorptionCorrection::AbsorptionCorrection() : API::Algorithm(),
  m_sampleObject(NULL), m_L1s(), m_elementVolumes(), m_elementPositions(),
  m_numVolumeElements(0), m_sampleVolume(0.0),
  m_refAtten(0.0), m_scattering(0), n_lambda(0), x_step(0)
{
}

void AbsorptionCorrection::init()
{
  declareProperty(new WorkspaceProperty<> ("InputWorkspace", "", Direction::Input,
    new WorkspaceUnitValidator<> ("Wavelength")),
    "The X values for the input workspace must be in units of wavelength");
  declareProperty(new WorkspaceProperty<> ("OutputWorkspace", "", Direction::Output),
    "Output workspace name");

  BoundedValidator<double> *mustBePositive = new BoundedValidator<double> ();
  mustBePositive->setLower(0.0);
  declareProperty("AttenuationXSection", -1.0, mustBePositive,
    "The attenuation cross-section for the sample material in barns");
  declareProperty("ScatteringXSection", -1.0, mustBePositive->clone(),
    "The scattering cross-section for the sample material in barns");
  declareProperty("SampleNumberDensity", -1.0, mustBePositive->clone(),
    "The number density of the sample in number per cubic angstrom");

  BoundedValidator<int> *positiveInt = new BoundedValidator<int> ();
  positiveInt->setLower(1);
  declareProperty("NumberOfWavelengthPoints", EMPTY_INT(), positiveInt,
    "The number of wavelength points for which the numerical integral is\n"
    "calculated (default: all points)");

  std::vector<std::string> exp_options;
  exp_options.push_back("Normal");
  exp_options.push_back("FastApprox");
  declareProperty("ExpMethod", "Normal", new ListValidator(exp_options),
    "Select the method to use to calculate exponentials, normal or a\n"
    "fast approximation (default: Normal)" );

  // Call the virtual method for concrete algorithm to define any other properties
  defineProperties();
}

void AbsorptionCorrection::exec()
{
  // Retrieve the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Get the input parameters
  retrieveBaseProperties();

  // Create the output workspace
  MatrixWorkspace_sptr correctionFactors = WorkspaceFactory::Instance().create(inputWS);
  correctionFactors->isDistribution(true); // The output of this is a distribution
  correctionFactors->setYUnit(""); // Need to explicitly set YUnit to nothing
  correctionFactors->setYUnitLabel("Attenuation factor");

  constructSample(correctionFactors->mutableSample());

  const int numHists = inputWS->getNumberHistograms();
  const int specSize = inputWS->blocksize();

  // If the number of wavelength points has not been given, use them all
  if ( isEmpty(n_lambda) ) n_lambda = specSize;
  x_step = specSize / n_lambda; // Bin step between points to calculate

  if (x_step == 0) //Number of wavelength points >number of histogram points
    x_step = 1;

  std::ostringstream message;
  message << "Numerical integration performed every " << x_step << " wavelength points" << std::endl;
  g_log.information(message.str());
  message.str("");

  const bool isHist = inputWS->isHistogramData();

  //calculate the cached values of L1 and element volumes
  initialiseCachedDistances();

  Progress prog(this,0.0,1.0,numHists);
  // Loop over the spectra
  PARALLEL_FOR2(inputWS,correctionFactors)
  for (int i = 0; i < numHists; ++i)
  {
    PARALLEL_START_INTERUPT_REGION

    // Copy over bin boundaries
    const MantidVec& X = inputWS->readX(i);
    correctionFactors->dataX(i) = X;

    // Get detector position
    IDetector_const_sptr det;
    try
    {
      det = inputWS->getDetector(i);
    } catch (Exception::NotFoundError)
    {
      // Catch when a spectrum doesn't have an attached detector and go to next one
      continue;
    }

    std::vector<double> lTotal(m_numVolumeElements);
    calculateDistances(det,lTotal);

    // Get a reference to the Y's in the output WS for storing the factors
    MantidVec& Y = correctionFactors->dataY(i);

    // Loop through the bins in the current spectrum every x_step
    for (int j = 0; j < specSize; j = j + x_step)
    {
      const double lambda = (isHist ? (0.5 * (X[j] + X[j + 1])) : X[j]);
      Y[j] = this->doIntegration(lambda, lTotal);
      Y[j] /= m_sampleVolume; // Divide by total volume of the cylinder
    }

    if (x_step > 1) // Interpolate linearly between points separated by x_step, last point required
    {
      const double lambda = (isHist ? (0.5 * (X[specSize - 1] + X[specSize])) : X[specSize - 1]);
      Y[specSize - 1] = this->doIntegration(lambda, lTotal);
      Y[specSize - 1] /= m_sampleVolume;
      interpolate(X, Y, isHist);
    }

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  g_log.information() << "Total number of elements in the integration was " << m_L1s.size() << std::endl;
  setProperty("OutputWorkspace", correctionFactors);

  // Now do some cleaning-up since destructor may not be called immediately
  m_L1s.clear();
  m_elementVolumes.clear();
  m_elementPositions.clear();
}

/// Fetch the properties and set the appropriate member variables
void AbsorptionCorrection::retrieveBaseProperties()
{
  const double sigma_atten = getProperty("AttenuationXSection"); // in barns
  const double sigma_s = getProperty("ScatteringXSection"); // in barns
  double rho = getProperty("SampleNumberDensity"); // in Angstroms-3
  rho *= 100;  // Needed to get the units right
  m_refAtten = -sigma_atten * rho / 1.798;
  m_scattering = -sigma_s * rho;
  
  n_lambda = getProperty("NumberOfWavelengthPoints");

  std::string exp_string = getProperty("ExpMethod");
  if (exp_string == "Normal") // Use the system exp function
    EXPONENTIAL = exp;
  else if (exp_string == "FastApprox") // Use the compact approximation
    EXPONENTIAL = fast_exp;

  // Call the virtual function for any further properties
  retrieveProperties();
}

/// Create the cylinder object using the Geometry classes
void AbsorptionCorrection::constructSample(API::Sample& sample)
{
  boost::shared_ptr<Object> shape = ShapeFactory().createShape(sampleXML());
  sample.setShapeObject( shape );
  m_sampleObject = shape.get();

  g_log.information("Successfully constructed the sample object");
}

/// Calculate the distances traversed by the neutrons within the sample
/// @param detector the detector we are working on
/// @param lTotal a vestor of the total length (L1+ L2) for  each segment of the sample
void AbsorptionCorrection::calculateDistances(const Geometry::IDetector_const_sptr& detector, std::vector<double>& lTotal) const
{
  // We need to make sure this is right for grouped detectors - should use average theta & phi
  V3D detectorPos;
  
  for (int i = 0; i < m_numVolumeElements; ++i)
  {
    // We need to make sure this is right for grouped detectors - should use average theta & phi
    // *** ASSUMES THAT SAMPLE AT ORIGIN AND BEAM ALONG Z ***
    detectorPos.spherical(detector->getDistance(Component("dummy",m_elementPositions[i])),
      detector->getTwoTheta(m_elementPositions[i],V3D(0.0,0.0,1.0))*180.0/M_PI,detector->getPhi()*180.0/M_PI);
    // Create track for distance in cylinder between scattering point and detector
    V3D direction = detectorPos - m_elementPositions[i];
    direction.normalize();
    Track outgoing(m_elementPositions[i], direction);
    int temp = m_sampleObject->interceptSurface(outgoing);

    /* Most of the time, the number of hits is 1. Sometime, we have more than one intersection due to
     * arithmetic imprecision. If it is the case, then selecting the first intersection is valid.
     * In principle, one could check the consistency of all distances if hits is larger than one by doing:
     * Mantid::Geometry::Track::LType::const_iterator it=outgoing.begin();
     * and looping until outgoing.end() checking the distances with it->Dist
     */
    // Not hitting the cylinder from inside, usually means detector is badly defined,
    // i.e, position is (0,0,0).
    if (temp < 1)
    {
      std::ostringstream message;
      message << "Problem with detector at" << detectorPos << std::endl;
      message << "This usually means that this detector is defined inside the sample cylinder";
      g_log.error(message.str());
      throw std::runtime_error("Problem in AbsorptionCorrection::calculateDistances");
    }
    lTotal[i] = outgoing.begin()->Dist + m_L1s[i];
  }
}

/// Carries out the numerical integration over the cylinder
double AbsorptionCorrection::doIntegration(const double& lambda, const std::vector<double>& lTotal) const
{
  double integral = 0.0;
  double exponent;

  int el = lTotal.size();

  // Iterate over all the elements, summing up the integral
  for (int i = 0; i < el; ++i)
  {
    // Equation is exponent * element volume
    // where exponent is e^(-mu * wavelength/1.8 * (L1+L2) )  (N.B. distances are in cm)
    exponent = ((m_refAtten * lambda) + m_scattering) * (lTotal[i]);
    integral += (EXPONENTIAL(exponent) * (m_elementVolumes[i]));
  }
  
  return integral;
}

/// Calculate the value of the correction at the points not explicitly numerically integrated
void AbsorptionCorrection::interpolate(const MantidVec& x, MantidVec& y, bool is_histogram)
{
  int step = x_step, index2;
  double x1 = 0, x2 = 0, y1 = 0, y2 = 0, xp, overgap = 0;
  int specsize = y.size();
  for (int i = 0; i < specsize - 1; ++i) // Last point has been calculated
  {
    if (step == x_step) //Point numerically integrated, does not need interpolation
    {
      x1 = (is_histogram ? (0.5 * (x[i] + x[i + 1])) : x[i]);
      index2 = ((i + x_step) >= specsize ? specsize - 1 : (i + x_step));
      x2 = (is_histogram ? (0.5 * (x[index2] + x[index2 + 1])) : x[index2]);
      overgap = 1.0 / (x2 - x1);
      y1 = y[i];
      y2 = y[index2];
      step = 1;
      continue;
    }
    xp = (is_histogram ? (0.5 * (x[i] + x[i + 1])) : x[i]);
    // Interpolation
    y[i] = (xp - x1) * y2 + (x2 - xp) * y1;
    y[i] *= overgap;
    step++;
  }
  return;
}

} // namespace Algorithms
} // namespace Mantid
