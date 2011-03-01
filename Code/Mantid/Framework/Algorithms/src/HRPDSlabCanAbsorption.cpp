//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/HRPDSlabCanAbsorption.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(HRPDSlabCanAbsorption)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void HRPDSlabCanAbsorption::init()
{
  this->setWikiSummary("Calculates attenuation due to absorption and scattering in an HRPD 'slab' can.");
  this->setOptionalMessage("Calculates attenuation due to absorption and scattering in an HRPD 'slab' can.");

  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  BoundedValidator<double> *mustBePositive = new BoundedValidator<double> ();
  mustBePositive->setLower(0.0);
  declareProperty("SampleAttenuationXSection", -1.0, mustBePositive,
    "The attenuation cross-section for the sample material in barns");
  declareProperty("SampleScatteringXSection", -1.0, mustBePositive->clone(),
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

  std::vector<std::string> thicknesses(4);
  thicknesses[0] = "0.2";
  thicknesses[1] = "0.5";
  thicknesses[2] = "1.0";
  thicknesses[3] = "1.5";
  declareProperty("Thickness", "", new ListValidator(thicknesses));
  
  BoundedValidator<double> *moreThanZero = new BoundedValidator<double> ();
  moreThanZero->setLower(0.001);
  declareProperty("ElementSize", 1.0, moreThanZero, 
    "The size of one side of an integration element cube in mm");
}

void HRPDSlabCanAbsorption::exec()
{
  // First run the numerical absorption correction for the sample
  MatrixWorkspace_sptr workspace = runFlatPlateAbsorption();

  /* Now do the corrections for the sample holder.
     Using an analytical formula for a flat plate correction:
        exp( -mt/cos(theta) )
     Fullprof says it then divides by another cos(theta), but I
     can't see where this comes from - it would make the transmission
     higher at higher angle, which isn't intuitive.
     In this equation, it's not obvious to me where theta should be
     calculated from - I'm taking the centre of the plate (in any
     case this would only make a very small difference)
   */

  /* The HRPD sample holders consist of a cuboid void for the sample of
     18x23 mm and some thickness, 0.125mm vanadium foil windows front
     and back, and an aluminium surround of total width 40mm, meaning
     that there is 11mm of Al to be traversed en route to the 90 degree
     bank.
   */

  const double vanWinThickness = 0.000125; // in m
  const double vanRho = 0.07192*100; // in Angstroms-3 * 100
  const double vanRefAtten = -5.08 * vanRho / 1.798;
  const double vanScat = -5.1 * vanRho;


  const int numHists = workspace->getNumberHistograms();
  const int specSize = workspace->blocksize();
  const bool isHist = workspace->isHistogramData();
  //
  Progress progress(this,0.91,1.0,numHists);
  for (int i = 0; i < numHists; ++i)
  {
    const MantidVec& X = workspace->readX(i);
    MantidVec& Y = workspace->dataY(i);

    // Get detector position
    IDetector_const_sptr det;
    try
    {
      det = workspace->getDetector(i);
    } catch (Exception::NotFoundError)
    {
      // Catch when a spectrum doesn't have an attached detector and go to next one
      continue;
    }

    V3D detectorPos;
    detectorPos.spherical(det->getDistance(Component("dummy",V3D(0.0,0.0,0.0))),
                          det->getTwoTheta(V3D(0.0,0.0,0.0),V3D(0.0,0.0,1.0))*180.0/M_PI,
                          det->getPhi()*180.0/M_PI);

    const int detID = det->getID();
    double angleFactor;
    // If the low angle or backscattering bank, want angle wrt beamline
    if ( detID < 900000 )
    {
      double theta = detectorPos.angle(V3D(0.0,0.0,1.0));
      angleFactor = 1.0 / std::abs(cos(theta));
    }
    // For 90 degree bank need it wrt X axis
    else
    {
      double theta = detectorPos.angle(V3D(1.0,0.0,0.0));
      angleFactor = 1.0 / std::abs(cos(theta));
    }
    
    for (int j = 0; j < specSize; ++j)
    {
      const double lambda = (isHist ? (0.5 * (X[j] + X[j+1])) : X[j]);

      // Front vanadium window - 0.5-1% effect, increasing with lambda
      Y[j] *= exp(vanWinThickness * ((vanRefAtten*lambda) + vanScat));

      // 90 degree bank aluminium sample holder
      if ( detID > 900000 )
      {
        // Below values are for aluminium
        Y[j] *= exp(-angleFactor * 0.011 * 6.02 * ((0.231*lambda/1.798) + 1.503));
      }
      else // Another vanadium window
      {
        Y[j] *= exp(angleFactor * vanWinThickness * ((vanRefAtten*lambda) + vanScat));
      }
    }

    progress.report();
  }
  
  setProperty("OutputWorkspace", workspace);
}

API::MatrixWorkspace_sptr HRPDSlabCanAbsorption::runFlatPlateAbsorption()
{
  // Call FlatPlateAbsorption as a sub-algorithm
  IAlgorithm_sptr childAlg = createSubAlgorithm("FlatPlateAbsorption",0.0,0.9);
  // Pass through all the properties
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", getProperty("InputWorkspace"));
  childAlg->setProperty<double>("AttenuationXSection", getProperty("SampleAttenuationXSection"));
  childAlg->setProperty<double>("ScatteringXSection", getProperty("SampleScatteringXSection"));
  childAlg->setProperty<double>("SampleNumberDensity", getProperty("SampleNumberDensity"));
  childAlg->setProperty<int>("NumberOfWavelengthPoints", getProperty("NumberOfWavelengthPoints"));
  childAlg->setProperty<std::string>("ExpMethod", getProperty("ExpMethod"));
  // The height and width of the sample holder are standard for HRPD
  const double HRPDCanHeight = 2.3;
  const double HRPDCanWidth = 1.8;
  childAlg->setProperty("SampleHeight", HRPDCanHeight);
  childAlg->setProperty("SampleWidth", HRPDCanWidth);
  // Valid values are 0.2,0.5,1.0 & 1.5 - would be nice to have a numeric list validator
  const std::string thickness = getPropertyValue("Thickness");
  childAlg->setPropertyValue("SampleThickness", thickness);

  // Now execute the sub-algorithm. Catch and log any error
  try
  {
    childAlg->execute();
  }
  catch (std::runtime_error&)
  {
    g_log.error("Unable to successfully run FlatPlateAbsorption sub-algorithm");
    throw;
  }

  if ( ! childAlg->isExecuted() )
  {
    g_log.error("Unable to successfully run FlatPlateAbsorption sub-algorithm");
    throw std::runtime_error("Unable to successfully run FlatPlateAbsorption sub-algorithm");
  }

  return childAlg->getProperty("OutputWorkspace");
}


} // namespace Algorithms
} // namespace Mantid

