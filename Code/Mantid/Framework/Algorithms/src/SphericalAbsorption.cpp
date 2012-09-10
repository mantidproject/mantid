/*WIKI* 


Calculates bin-by-bin correction factors for attenuation due to absorption and scattering in a '''spherical''' sample. Sample data must be divided by these corrections.  Algorithm calls AnvredCorrection.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SphericalAbsorption.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Fast_Exponential.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SphericalAbsorption)

/// Sets documentation strings for this algorithm
void SphericalAbsorption::initDocs()
{
  this->setWikiSummary("Calculates bin-by-bin correction factors for attenuation due to absorption and scattering in a '''spherical''' sample. ");
  this->setOptionalMessage("Calculates bin-by-bin correction factors for attenuation due to absorption and scattering in a 'spherical' sample.");
}

using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;

SphericalAbsorption::SphericalAbsorption() : API::Algorithm(), m_inputWS(),
  m_refAtten(0.0), m_scattering(0)
{
}

void SphericalAbsorption::init()
{
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(new WorkspaceProperty<> ("InputWorkspace", "", Direction::Input,wsValidator),
    "The X values for the input workspace must be in units of wavelength");
  declareProperty(new WorkspaceProperty<> ("OutputWorkspace", "", Direction::Output),
    "Output workspace name");

  auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
  mustBePositive->setLower(0.0);
  declareProperty("AttenuationXSection", -1.0, mustBePositive,
    "The ABSORPTION cross-section for the sample material in barns");
  declareProperty("ScatteringXSection", -1.0, mustBePositive,
    "The scattering cross-section (coherent + incoherent) for the sample material in barns");
  declareProperty("SampleNumberDensity", -1.0, mustBePositive,
    "The number density of the sample in number per cubic angstrom");
  declareProperty("SphericalSampleRadius", -1.0, mustBePositive,
    "The radius of the spherical sample in centimetres");

}

void SphericalAbsorption::exec()
{
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");

  // Get the input parameters
  retrieveBaseProperties();

  // Create the output workspace
  MatrixWorkspace_sptr correctionFactors = WorkspaceFactory::Instance().create(m_inputWS);
  correctionFactors->isDistribution(true); // The output of this is a distribution
  correctionFactors->setYUnit(""); // Need to explicitly set YUnit to nothing
  correctionFactors->setYUnitLabel("Attenuation factor");
  double m_sphRadius = getProperty("SphericalSampleRadius"); // in cm


  IAlgorithm_sptr anvred = createSubAlgorithm("AnvredCorrection");
  anvred->setProperty<MatrixWorkspace_sptr>("InputWorkspace", m_inputWS);
  anvred->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", correctionFactors);
  anvred->setProperty("PreserveEvents", true);
  anvred->setProperty("ReturnTransmissionOnly", true);
  anvred->setProperty("LinearScatteringCoef", m_scattering);
  anvred->setProperty("LinearAbsorptionCoef", m_refAtten);
  anvred->setProperty("Radius", m_sphRadius);
  anvred->executeAsSubAlg();
  // Get back the result
  correctionFactors = anvred->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", correctionFactors);

}

/// Fetch the properties and set the appropriate member variables
void SphericalAbsorption::retrieveBaseProperties()
{
  const double sigma_atten = getProperty("AttenuationXSection"); // in barns
  const double sigma_s = getProperty("ScatteringXSection"); // in barns
  double rho = getProperty("SampleNumberDensity"); // in Angstroms-3
  m_refAtten = sigma_atten * rho;
  m_scattering = sigma_s * rho;
  
}

} // namespace Algorithms
} // namespace Mantid
