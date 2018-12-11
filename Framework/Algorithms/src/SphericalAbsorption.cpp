// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SphericalAbsorption.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Fast_Exponential.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

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
    : API::Algorithm(), m_inputWS(), m_sampleObject(nullptr), m_beamDirection(),
      m_L1s(), m_elementVolumes(), m_elementPositions(), m_numVolumeElements(0),
      m_sampleVolume(0.), m_refAtten(0.0), m_scattering(0.), n_lambda(0),
      x_step(0), m_emode(0), m_lambdaFixed(0.) {}

void SphericalAbsorption::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                       wsValidator),
      "The X values for the input workspace must be in units of wavelength");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
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
  correctionFactors->setDistribution(
      true);                       // The output of this is a distribution
  correctionFactors->setYUnit(""); // Need to explicitly set YUnit to nothing
  correctionFactors->setYUnitLabel("Attenuation factor");
  double m_sphRadius = getProperty("SphericalSampleRadius"); // in cm

  Progress progress(this, 0.0, 1.0, 2);

  progress.report("AnvredCorrection");

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

  progress.report();

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
    NeutronAtom neutron(0, 0, 0.0, 0.0, sigma_s, 0.0, sigma_s, sigma_atten);
    auto shape = boost::shared_ptr<IObject>(
        m_inputWS->sample().getShape().cloneWithMaterial(
            Material("SetInSphericalAbsorption", neutron, rho)));
    m_inputWS->mutableSample().setShape(shape);
  }

  m_refAtten = sigma_atten * rho;
  m_scattering = sigma_s * rho;
}

} // namespace Algorithms
} // namespace Mantid
