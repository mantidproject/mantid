// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation MuscatElastic,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MuscatElastic.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Material.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::DataObjects::Workspace2D;

namespace {
constexpr int DEFAULT_NEVENTS = 1000;
constexpr int DEFAULT_SEED = 123456789;
constexpr int DEFAULT_LATITUDINAL_DETS = 5;
constexpr int DEFAULT_LONGITUDINAL_DETS = 10;
} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MuscatElastic)

/**
 * Initialize the algorithm
 */
void MuscatElastic::init() {
  // The input workspace must have an instrument and units of MomentumTransfer
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("MomentumTransfer");
  wsValidator->add<InstrumentValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "SofqWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace.  The input workspace must "
                  "have X units of momentum transfer.");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("ScatteringCrossSection", "",
                                            Direction::Input, wsValidator),
      "A workspace containing the scattering cross section as a function of "
      "k.");

  auto positiveInt = std::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);
  declareProperty(
      "NeutronEventsSingle", DEFAULT_NEVENTS, positiveInt,
      "The number of \"neutron\" events to generate for single scattering");
  declareProperty(
      "NeutronEventsMultiple", DEFAULT_NEVENTS, positiveInt,
      "The number of \"neutron\" events to generate for multiple scattering");
  declareProperty("SeedValue", DEFAULT_SEED, positiveInt,
                  "Seed the random number generator with this value");
  auto nScatteringsValidator =
      std::make_shared<Kernel::BoundedValidator<int>>();
  nScatteringsValidator->setLower(1);
  nScatteringsValidator->setUpper(5);
  declareProperty("NumberScatterings", DEFAULT_SEED, nScatteringsValidator,
                  "Number of scatterings");

  declareProperty("SparseInstrument", false,
                  "Enable simulation on special "
                  "instrument with a sparse grid of "
                  "detectors interpolating the "
                  "results to the real instrument.");
  auto threeOrMore = std::make_shared<Kernel::BoundedValidator<int>>();
  threeOrMore->setLower(3);
  declareProperty(
      "NumberOfDetectorRows", DEFAULT_LATITUDINAL_DETS, threeOrMore,
      "Number of detector rows in the detector grid of the sparse instrument.");
  setPropertySettings(
      "NumberOfDetectorRows",
      std::make_unique<EnabledWhenProperty>(
          "SparseInstrument", ePropertyCriterion::IS_NOT_DEFAULT));
  auto twoOrMore = std::make_shared<Kernel::BoundedValidator<int>>();
  twoOrMore->setLower(2);
  declareProperty("NumberOfDetectorColumns", DEFAULT_LONGITUDINAL_DETS,
                  twoOrMore,
                  "Number of detector columns in the detector grid "
                  "of the sparse instrument.");
  setPropertySettings(
      "NumberOfDetectorColumns",
      std::make_unique<EnabledWhenProperty>(
          "SparseInstrument", ePropertyCriterion::IS_NOT_DEFAULT));
}

/**
 * Execution code
 */
void MuscatElastic::exec() {
  const MatrixWorkspace_sptr inputWS = getProperty("SofqWorkspace");
  auto outputWS = createOutputWorkspace(*inputWS);

  const MatrixWorkspace_sptr sigmaSSWS = getProperty("ScatteringCrossSection");

  const bool useSparseInstrument = getProperty("SparseInstrument");
  SparseWorkspace_sptr sparseWS;
  if (useSparseInstrument) {
    const int latitudinalDets = getProperty("NumberOfDetectorRows");
    const int longitudinalDets = getProperty("NumberOfDetectorColumns");
    const auto inputNbins = static_cast<int>(inputWS->blocksize());
    sparseWS = createSparseWorkspace(*inputWS, inputNbins, latitudinalDets,
                                     longitudinalDets);
  }
  MatrixWorkspace &simulationWS = useSparseInstrument ? *sparseWS : *outputWS;
  const MatrixWorkspace &instrumentWS =
      useSparseInstrument ? simulationWS : *inputWS;
  auto instrument = instrumentWS.getInstrument();
  const auto nhists = static_cast<int64_t>(instrumentWS.getNumberHistograms());

  const int nScatters = getProperty("NumberScatterings");

  PARALLEL_FOR_IF(Kernel::threadSafe(simulationWS))
  for (int64_t i = 0; i < nhists; ++i) {
    PARALLEL_START_INTERUPT_REGION
    double vmfp, sigma_total, scatteringXSection;
    auto numberDensity =
        inputWS->sample().getMaterial().numberDensityEffective();
    for (int bin = 0; bin < inputWS->blocksize(); bin++) {
      // use Kernel::UnitConversion::run instead?? Momentum, MomentumTransfer
      double kinc = inputWS->histogram(i).x()[bin] /
                    (2 * sin(inputWS->detectorInfo().twoTheta(i)));
      double wavelength = 2 * M_PI / kinc;
      auto absorbXsection = inputWS->sample().getMaterial().absorbXSection(wavelength);
      if (sigmaSSWS) {
        scatteringXSection = interpolateSigmaS(sigmaSSWS, kinc);
      } else {
        scatteringXSection =
            inputWS->sample().getMaterial().totalScatterXSection();
      }

      std::tie(vmfp, sigma_total) =
          new_vector(absorbXsection, numberDensity, scatteringXSection);

      const int nSingleScatterEvents = getProperty("NeutronEventsSingle");
      const int nMultiScatterEvents = getProperty("NeutronEventsMultiple");
      scatter(false, nSingleScatterEvents, 0);
      scatter(false, nSingleScatterEvents, absorbXsection);
      for (int ne = 0; ne < nScatters; ne++) {
        scatter(false, nMultiScatterEvents, absorbXsection);
      }
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

std::tuple<double, double>
MuscatElastic::new_vector(double absorbXsection, double numberDensity,
                          double totalScatterXsection) {
  double sig_scat = 0.;
  auto sig_total = sig_scat + absorbXsection;
  auto vmu = numberDensity * sig_total;
  auto vmfp = 1.00 / vmu;
  return std::make_tuple(vmfp, sig_total);
}

double MuscatElastic::interpolateSigmaS(const MatrixWorkspace_sptr sigmaS,
                                        double kinc) {
  if (kinc > sigmaS->x(0).back()) {
    return sigmaS->x(0).back();
  }
  if (kinc < sigmaS->x(0).front()) {
    return sigmaS->x(0).front();
  }
  // assume log(cross section) is quadratic in k
  auto idx = sigmaS->yIndexOfX(kinc);
  auto U = kinc - sigmaS->x(0)[idx];
  auto &y = sigmaS->y(0);
  auto A = (y[idx] - 2 * y[idx + 1] + y[idx + 2]) / 2;
  auto B = (-3 * y[idx] - 4 * y[idx + 1] - y[idx + 2]) / 2;
  auto C = y[idx];
  return exp(A * U * U + B * U + C);
}

void MuscatElastic::scatter(bool doMultipleScattering, int nScatters,
                            double absorbXsection) {

}

Kernel::V3D MuscatElastic::start_point() {

}

/**
 * Validate the input properties.
 * @return a map where keys are property names and values the found issues
 */
std::map<std::string, std::string> MuscatElastic::validateInputs() {
  std::map<std::string, std::string> issues;
  return issues;
}

/**
 * Factory method to return an instance of the required SparseInstrument class
 * @param modelWS The full workspace that the sparse one will be based on
 * @param wavelengthPoints The number of wavelength points to include in the
 * histograms in the sparse workspace
 * @param rows The number of rows of detectors to create
 * @param columns The number of columns of detectors to create
 * @return a pointer to an SparseInstrument object
 */
std::shared_ptr<SparseWorkspace>
MuscatElastic::createSparseWorkspace(const API::MatrixWorkspace &modelWS,
                                     const size_t wavelengthPoints,
                                     const size_t rows, const size_t columns) {
  auto sparseWS = std::make_shared<SparseWorkspace>(modelWS, wavelengthPoints,
                                                    rows, columns);
  return sparseWS;
}

MatrixWorkspace_uptr
MuscatElastic::createOutputWorkspace(const MatrixWorkspace &inputWS) const {
  MatrixWorkspace_uptr outputWS = DataObjects::create<Workspace2D>(inputWS);
  // The algorithm computes the signal values at bin centres so they should
  // be treated as a distribution
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Attenuation factor");
  return outputWS;
}

} // namespace Algorithms
} // namespace Mantid