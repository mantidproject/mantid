/*WIKI* 


This algorithm is a refinement of the [[FlatPlateAbsorption]] algorithm for the specific case of an HRPD 'slab can' sample holder. It uses the aforementioned generic algorithm to calculate the correction due to the sample itself, using numerical integration. This is done using the standard height x width dimensions of an HRPD sample holder of 23 x 18 mm. Valid values of the thickness are 2,5,10 & 15 mm, although this is not currently enforced.

Further corrections are then carried out to account for the 0.125mm Vanadium windows at the front and rear of the sample, and for the aluminium of the holder itself (which is traversed by neutrons en route to the 90 degree bank). This is carried out using an analytical approximation for a flat plate, the correction factor being calculated as
<math> \rm{exp} \left( \frac{- \rho \left( \sigma_a \frac{ \lambda} {1.798} + \sigma_s \right) t}{\rm{cos} \, \theta} \right) </math>, where <math>\lambda</math> is the wavelength, <math>\theta</math> the angle between the detector and the normal to the plate and the other symbols are as given in the property list above. The assumption is that the neutron enters the plate along the normal.

==== Restrictions on the input workspace ====
The input workspace must have units of wavelength. The [[instrument]] associated with the workspace must be fully defined because detector, source & sample position are needed.

===Subalgorithms used===
The [[FlatPlateAbsorption]] algorithm is used to calculate the correction due to the sample itself.



*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/HRPDSlabCanAbsorption.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(HRPDSlabCanAbsorption)

/// Sets documentation strings for this algorithm
void HRPDSlabCanAbsorption::initDocs()
{
  this->setWikiSummary("Calculates attenuation due to absorption and scattering in an HRPD 'slab' can. ");
  this->setOptionalMessage("Calculates attenuation due to absorption and scattering in an HRPD 'slab' can.");
}


using namespace Kernel;
using namespace API;
using namespace Geometry;

void HRPDSlabCanAbsorption::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
  mustBePositive->setLower(0.0);
  declareProperty("SampleAttenuationXSection", -1.0, mustBePositive,
    "The ABSORPTION cross-section for the sample material in barns");
  declareProperty("SampleScatteringXSection", -1.0, mustBePositive,
    "The scattering cross-section (coherent + incoherent) for the sample material in barns");
  declareProperty("SampleNumberDensity", -1.0, mustBePositive,
    "The number density of the sample in number per cubic angstrom");

  auto positiveInt = boost::make_shared<BoundedValidator<int64_t> >();
  positiveInt->setLower(1);
  declareProperty("NumberOfWavelengthPoints", int64_t(EMPTY_INT()), positiveInt,
    "The number of wavelength points for which the numerical integral is\n"
    "calculated (default: all points)");

  std::vector<std::string> exp_options;
  exp_options.push_back("Normal");
  exp_options.push_back("FastApprox");
  declareProperty("ExpMethod", "Normal", boost::make_shared<StringListValidator>(exp_options),
    "Select the method to use to calculate exponentials, normal or a\n"
    "fast approximation (default: Normal)" );

  std::vector<std::string> thicknesses(4);
  thicknesses[0] = "0.2";
  thicknesses[1] = "0.5";
  thicknesses[2] = "1.0";
  thicknesses[3] = "1.5";
  declareProperty("Thickness", "", boost::make_shared<StringListValidator>(thicknesses));
  
  auto moreThanZero = boost::make_shared<BoundedValidator<double> >();
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


  const size_t numHists = workspace->getNumberHistograms();
  const size_t specSize = workspace->blocksize();
  const bool isHist = workspace->isHistogramData();
  //
  Progress progress(this,0.91,1.0,numHists);
  for (size_t i = 0; i < numHists; ++i)
  {
    const MantidVec& X = workspace->readX(i);
    MantidVec& Y = workspace->dataY(i);

    // Get detector position
    IDetector_const_sptr det;
    try
    {
      det = workspace->getDetector(i);
    } catch (Exception::NotFoundError &)
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
    
    for (size_t j = 0; j < specSize; ++j)
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
  childAlg->setProperty<int64_t>("NumberOfWavelengthPoints", getProperty("NumberOfWavelengthPoints"));
  childAlg->setProperty<std::string>("ExpMethod", getProperty("ExpMethod"));
  // The height and width of the sample holder are standard for HRPD
  const double HRPDCanHeight = 2.3;
  const double HRPDCanWidth = 1.8;
  childAlg->setProperty("SampleHeight", HRPDCanHeight);
  childAlg->setProperty("SampleWidth", HRPDCanWidth);
  // Valid values are 0.2,0.5,1.0 & 1.5 - would be nice to have a numeric list validator
  const std::string thickness = getPropertyValue("Thickness");
  childAlg->setPropertyValue("SampleThickness", thickness);
  childAlg->executeAsSubAlg();
  return childAlg->getProperty("OutputWorkspace");
}


} // namespace Algorithms
} // namespace Mantid

