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


using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace Mantid::PhysicalConstants;

void HRPDSlabCanAbsorption::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
  mustBePositive->setLower(0.0);
  declareProperty("SampleAttenuationXSection",  EMPTY_DBL(), mustBePositive,
    "The ABSORPTION cross-section for the sample material in barns if not set with SetSampleMaterial");
  declareProperty("SampleScatteringXSection",  EMPTY_DBL(), mustBePositive,
    "The scattering cross-section (coherent + incoherent) for the sample material in barns if not set with SetSampleMaterial");
  declareProperty("SampleNumberDensity",  EMPTY_DBL(), mustBePositive,
    "The number density of the sample in number of atoms per cubic angstrom if not set with SetSampleMaterial");

  std::vector<std::string> thicknesses(4);
  thicknesses[0] = "0.2";
  thicknesses[1] = "0.5";
  thicknesses[2] = "1.0";
  thicknesses[3] = "1.5";
  declareProperty("Thickness", "0.2", boost::make_shared<StringListValidator>(thicknesses));

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
  MatrixWorkspace_sptr m_inputWS = getProperty("InputWorkspace");
  double sigma_atten = getProperty("SampleAttenuationXSection"); // in barns
  double sigma_s = getProperty("SampleScatteringXSection"); // in barns
  double rho = getProperty("SampleNumberDensity"); // in Angstroms-3
  const Material& sampleMaterial = m_inputWS->sample().getMaterial();
  if( sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda) != 0.0)
  {
    if (rho == EMPTY_DBL()) rho = sampleMaterial.numberDensity();
    if (sigma_s == EMPTY_DBL()) sigma_s =  sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda);
    if (sigma_atten == EMPTY_DBL()) sigma_atten = sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda);
  }
  else  //Save input in Sample with wrong atomic number and name
  {
    NeutronAtom neutron(static_cast<uint16_t>(EMPTY_DBL()), static_cast<uint16_t>(0),
                        0.0, 0.0, sigma_s, 0.0, sigma_s, sigma_atten);
    Object shape = m_inputWS->sample().getShape(); // copy
    shape.setMaterial(Material("SetInSphericalAbsorption", neutron, rho));
    m_inputWS->mutableSample().setShape(shape);
  
  }

  // Call FlatPlateAbsorption as a Child Algorithm
  IAlgorithm_sptr childAlg = createChildAlgorithm("FlatPlateAbsorption",0.0,0.9);
  // Pass through all the properties
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", m_inputWS);
  childAlg->setProperty<double>("AttenuationXSection", sigma_atten);
  childAlg->setProperty<double>("ScatteringXSection", sigma_s);
  childAlg->setProperty<double>("SampleNumberDensity", rho);
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
  childAlg->executeAsChildAlg();
  return childAlg->getProperty("OutputWorkspace");
}


} // namespace Algorithms
} // namespace Mantid
