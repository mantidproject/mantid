// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/HRPDSlabCanAbsorption.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(HRPDSlabCanAbsorption)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace Mantid::PhysicalConstants;

namespace { // anonymous

const V3D X_AXIS{1.0, 0.0, 0.0};
const V3D Z_AXIS{0.0, 0.0, 1.0};

/* The HRPD sample holders consist of a cuboid void for the sample of
   18x23 mm and some thickness, 0.125mm vanadium foil windows front
   and back, and an aluminium surround of total width 40mm, meaning
   that there is 11mm of Al to be traversed en route to the 90 degree
   bank.
 */

constexpr double VAN_WINDOW_THICKNESS{0.000125}; // in m
constexpr double VAN_RHO{0.07192 * 100};         // in Angstroms-3 * 100
constexpr double VAN_REF_ATTEN{-5.08 * VAN_RHO / NeutronAtom::ReferenceLambda};
constexpr double VAN_SCATT{-5.1 * VAN_RHO};

namespace PropertyNames {
const std::string INPUT_WKSP("InputWorkspace");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string SAMPLE_ATTEN_XS("SampleAttenuationXSection");
const std::string SAMPLE_SCATT_XS("SampleScatteringXSection");
const std::string SAMPLE_NUM_DENS("SampleNumberDensity");
const std::string THICKNESS("Thickness");
const std::string NUM_WL_POINTS("NumberOfWavelengthPoints");
const std::string EXP_METHOD("ExpMethod");
const std::string ELE_SIZE("ElementSize"); // is not used
} // namespace PropertyNames

} // namespace

void HRPDSlabCanAbsorption::init() {
  declareProperty(make_unique<WorkspaceProperty<>>(PropertyNames::INPUT_WKSP,
                                                   "", Direction::Input));
  declareProperty(make_unique<WorkspaceProperty<>>(PropertyNames::OUTPUT_WKSP,
                                                   "", Direction::Output));

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty(PropertyNames::SAMPLE_ATTEN_XS, EMPTY_DBL(), mustBePositive,
                  "The ABSORPTION cross-section for the sample material in "
                  "barns if not set with SetSampleMaterial");
  declareProperty(PropertyNames::SAMPLE_SCATT_XS, EMPTY_DBL(), mustBePositive,
                  "The scattering cross-section (coherent + incoherent) for "
                  "the sample material in barns if not set with "
                  "SetSampleMaterial");
  declareProperty(PropertyNames::SAMPLE_NUM_DENS, EMPTY_DBL(), mustBePositive,
                  "The number density of the sample in number of atoms per "
                  "cubic angstrom if not set with SetSampleMaterial");

  const std::vector<std::string> thicknesses{"0.2", "0.5", "1.0", "1.5"};
  declareProperty(PropertyNames::THICKNESS, thicknesses[0],
                  boost::make_shared<StringListValidator>(thicknesses));

  auto positiveInt = boost::make_shared<BoundedValidator<int64_t>>();
  positiveInt->setLower(1);
  declareProperty(
      PropertyNames::NUM_WL_POINTS, int64_t(EMPTY_INT()), positiveInt,
      "The number of wavelength points for which the numerical integral is\n"
      "calculated (default: all points)");

  std::vector<std::string> exp_options{"Normal", "FastApprox"};
  declareProperty(
      PropertyNames::EXP_METHOD, exp_options[0],
      boost::make_shared<StringListValidator>(exp_options),
      "Select the method to use to calculate exponentials, normal or a\n"
      "fast approximation (default: Normal)");

  auto moreThanZero = boost::make_shared<BoundedValidator<double>>();
  moreThanZero->setLower(0.001);
  declareProperty(PropertyNames::ELE_SIZE, 1.0, moreThanZero,
                  "The size of one side of an integration element cube in mm");
}

void HRPDSlabCanAbsorption::exec() {
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

  const size_t numHists = workspace->getNumberHistograms();
  const size_t specSize = workspace->blocksize();

  const auto &spectrumInfo = workspace->spectrumInfo();
  Progress progress(this, 0.91, 1.0, numHists);
  for (size_t i = 0; i < numHists; ++i) {

    if (!spectrumInfo.hasDetectors(i)) {
      // If a spectrum doesn't have an attached detector go to next one instead
      continue;
    }

    // Get detector position
    const V3D detectorPos = spectrumInfo.position(i);

    const int detID = spectrumInfo.detector(i).getID();
    double angleFactor;
    // If the low angle or backscattering bank, want angle wrt beamline
    if (detID < 900000) {
      double theta = detectorPos.angle(Z_AXIS);
      angleFactor = 1.0 / std::abs(cos(theta));
    }
    // For 90 degree bank need it wrt X axis
    else {
      double theta = detectorPos.angle(X_AXIS);
      angleFactor = 1.0 / std::abs(cos(theta));
    }

    auto &Y = workspace->mutableY(i);
    const auto lambdas = workspace->points(i);
    for (size_t j = 0; j < specSize; ++j) {
      const double lambda = lambdas[j];

      // Front vanadium window - 0.5-1% effect, increasing with lambda
      Y[j] *=
          exp(VAN_WINDOW_THICKNESS * ((VAN_REF_ATTEN * lambda) + VAN_SCATT));

      // 90 degree bank aluminium sample holder
      if (detID > 900000) {
        // Below values are for aluminium
        Y[j] *= exp(-angleFactor * 0.011 * 6.02 *
                    ((0.231 * lambda / NeutronAtom::ReferenceLambda) + 1.503));
      } else // Another vanadium window
      {
        Y[j] *= exp(angleFactor * VAN_WINDOW_THICKNESS *
                    ((VAN_REF_ATTEN * lambda) + VAN_SCATT));
      }
    }

    progress.report();
  }

  setProperty(PropertyNames::OUTPUT_WKSP, workspace);
}

API::MatrixWorkspace_sptr HRPDSlabCanAbsorption::runFlatPlateAbsorption() {
  MatrixWorkspace_sptr m_inputWS = getProperty(PropertyNames::INPUT_WKSP);
  double sigma_atten = getProperty(PropertyNames::SAMPLE_ATTEN_XS); // in barns
  double sigma_s = getProperty(PropertyNames::SAMPLE_SCATT_XS);     // in barns
  double rho = getProperty(PropertyNames::SAMPLE_NUM_DENS); // in Angstroms-3
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

  // Call FlatPlateAbsorption as a Child Algorithm
  IAlgorithm_sptr childAlg =
      createChildAlgorithm("FlatPlateAbsorption", 0.0, 0.9);
  // Pass through all the properties
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", m_inputWS);
  childAlg->setProperty<double>("AttenuationXSection", sigma_atten);
  childAlg->setProperty<double>("ScatteringXSection", sigma_s);
  childAlg->setProperty<double>("SampleNumberDensity", rho);
  childAlg->setProperty<int64_t>("NumberOfWavelengthPoints",
                                 getProperty(PropertyNames::NUM_WL_POINTS));
  childAlg->setProperty<std::string>("ExpMethod",
                                     getProperty(PropertyNames::EXP_METHOD));
  // The height and width of the sample holder are standard for HRPD
  const double HRPDCanHeight = 2.3;
  const double HRPDCanWidth = 1.8;
  childAlg->setProperty("SampleHeight", HRPDCanHeight);
  childAlg->setProperty("SampleWidth", HRPDCanWidth);
  // Valid values are 0.2,0.5,1.0 & 1.5 - would be nice to have a numeric list
  // validator
  childAlg->setPropertyValue("SampleThickness",
                             getPropertyValue(PropertyNames::THICKNESS));
  childAlg->executeAsChildAlg();
  return childAlg->getProperty("OutputWorkspace");
}

} // namespace Algorithms
} // namespace Mantid
