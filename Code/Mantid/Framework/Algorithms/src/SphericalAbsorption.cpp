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

using namespace Mantid::PhysicalConstants;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SphericalAbsorption)

using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;

SphericalAbsorption::SphericalAbsorption()
    : API::Algorithm(), m_inputWS(), m_refAtten(0.0), m_scattering(0) {}

void SphericalAbsorption::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,
                              wsValidator),
      "The X values for the input workspace must be in units of wavelength");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Output workspace name");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("AttenuationXSection", EMPTY_DBL(), mustBePositive,
                  "The '''absorption''' cross-section, at 1.8 Angstroms, for "
                  "the sample material in barns, if not set with "
                  "SetSampleMaterial.");
  declareProperty("ScatteringXSection", EMPTY_DBL(), mustBePositive,
                  "The (coherent + incoherent) scattering cross-section for "
                  "the sample material in barns, if not set with "
                  "SetSampleMaterial.");
  declareProperty("SampleNumberDensity", EMPTY_DBL(), mustBePositive,
                  "The number density of the sample in number of atoms per "
                  "cubic angstrom, if not set with SetSampleMaterial");
  declareProperty("SphericalSampleRadius", EMPTY_DBL(), mustBePositive,
                  "The radius of the spherical sample in centimetres");
}

void SphericalAbsorption::exec() {
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");

  // Get the input parameters
  retrieveBaseProperties();

  // Create the output workspace
  MatrixWorkspace_sptr correctionFactors =
      WorkspaceFactory::Instance().create(m_inputWS);
  correctionFactors->isDistribution(
      true);                       // The output of this is a distribution
  correctionFactors->setYUnit(""); // Need to explicitly set YUnit to nothing
  correctionFactors->setYUnitLabel("Attenuation factor");
  double m_sphRadius = getProperty("SphericalSampleRadius"); // in cm

  IAlgorithm_sptr anvred = createChildAlgorithm("AnvredCorrection");
  anvred->setProperty<MatrixWorkspace_sptr>("InputWorkspace", m_inputWS);
  anvred->setProperty<MatrixWorkspace_sptr>("OutputWorkspace",
                                            correctionFactors);
  anvred->setProperty("PreserveEvents", true);
  anvred->setProperty("ReturnTransmissionOnly", true);
  anvred->setProperty("LinearScatteringCoef", m_scattering);
  anvred->setProperty("LinearAbsorptionCoef", m_refAtten);
  anvred->setProperty("Radius", m_sphRadius);
  anvred->executeAsChildAlg();
  // Get back the result
  correctionFactors = anvred->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", correctionFactors);
}

/// Fetch the properties and set the appropriate member variables
void SphericalAbsorption::retrieveBaseProperties() {
  double sigma_atten = getProperty("AttenuationXSection"); // in barns
  double sigma_s = getProperty("ScatteringXSection");      // in barns
  double rho = getProperty("SampleNumberDensity");         // in Angstroms-3
  const Material &sampleMaterial = m_inputWS->sample().getMaterial();
  if (sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda) !=
      0.0) {
    if (rho == EMPTY_DBL())
      rho = sampleMaterial.numberDensity();
    if (sigma_s == EMPTY_DBL())
      sigma_s =
          sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda);
    if (sigma_atten == EMPTY_DBL())
      sigma_atten = sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda);
  } else // Save input in Sample with wrong atomic number and name
  {
    NeutronAtom neutron(static_cast<uint16_t>(EMPTY_DBL()),
                        static_cast<uint16_t>(0), 0.0, 0.0, sigma_s, 0.0,
                        sigma_s, sigma_atten);
    Object shape = m_inputWS->sample().getShape(); // copy
    shape.setMaterial(Material("SetInSphericalAbsorption", neutron, rho));
    m_inputWS->mutableSample().setShape(shape);
  }

  m_refAtten = sigma_atten * rho;
  m_scattering = sigma_s * rho;
}

} // namespace Algorithms
} // namespace Mantid
