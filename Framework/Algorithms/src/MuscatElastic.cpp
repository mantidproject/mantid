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
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/MersenneTwister.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::DataObjects::Workspace2D;

namespace {
constexpr int DEFAULT_NEVENTS = 1000;
constexpr int DEFAULT_SEED = 123456789;
constexpr int DEFAULT_NSCATTERINGS = 1;
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
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace.  The input workspace must "
                  "have X units of momentum transfer.");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("SofqWorkspace", "",
                                            Direction::Input, wsValidator),
      "The name of the workspace containing S(q).  The input workspace must "
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
  declareProperty("NumberScatterings", DEFAULT_NSCATTERINGS,
                  nScatteringsValidator, "Number of scatterings");

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
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const MatrixWorkspace_sptr SQWS = getProperty("SofqWorkspace");
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
  const int seed = getProperty("SeedValue");

  PARALLEL_FOR_IF(Kernel::threadSafe(simulationWS))
  for (int64_t i = 0; i < nhists; ++i) {
    PARALLEL_START_INTERUPT_REGION
    MersenneTwister rng(seed + int(i));
    double vmfp, sigma_total, scatteringXSection;
    auto numberDensity =
        inputWS->sample().getMaterial().numberDensityEffective();
    for (int bin = 0; bin < inputWS->blocksize(); bin++) {
      // use Kernel::UnitConversion::run instead?? Momentum, MomentumTransfer
      double kinc = inputWS->histogram(i).x()[bin] /
                    (2 * sin(0.5 * inputWS->detectorInfo().twoTheta(i)));
      double wavelength = 2 * M_PI / kinc;
      auto absorbXsection =
          inputWS->sample().getMaterial().absorbXSection(wavelength);
      if (sigmaSSWS) {
        // take log of sigmaSSWS and store it this way
        auto &y = sigmaSSWS->mutableY(0);
        std::transform(y.begin(), y.end(), y.begin(),
                       static_cast<double (*)(double)>(std::log));
        scatteringXSection = interpolateLogQuadratic(sigmaSSWS, kinc);
      } else {
        scatteringXSection =
            inputWS->sample().getMaterial().totalScatterXSection();
      }

      std::tie(vmfp, sigma_total) =
          new_vector(absorbXsection, numberDensity, scatteringXSection);

      const int nSingleScatterEvents = getProperty("NeutronEventsSingle");
      const int nMultiScatterEvents = getProperty("NeutronEventsMultiple");

      // take log of S(Q) and store it this way
      auto &y = SQWS->mutableY(0);
      std::transform(y.begin(), y.end(), y.begin(),
                     static_cast<double (*)(double)>(std::log));
      scatter(false, nSingleScatterEvents, 0, inputWS->sample(), *instrument,
              rng, vmfp, sigma_total, SQWS, kinc);
      scatter(false, nSingleScatterEvents, absorbXsection, inputWS->sample(),
              *instrument, rng, vmfp, sigma_total, SQWS, kinc);
      for (int ne = 0; ne < nScatters; ne++) {
        double total = scatter(true, nMultiScatterEvents, absorbXsection, inputWS->sample(),
                *instrument, rng, vmfp, sigma_total, SQWS, kinc);
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

double MuscatElastic::interpolateLogQuadratic(
    const MatrixWorkspace_sptr workspaceToInterpolate, double x) {
  if (x > workspaceToInterpolate->x(0).back()) {
    return workspaceToInterpolate->x(0).back();
  }
  if (x < workspaceToInterpolate->x(0).front()) {
    return workspaceToInterpolate->x(0).front();
  }
  // assume log(cross section) is quadratic in k
  auto idx = workspaceToInterpolate->yIndexOfX(x);
  // need at least two points to the right of the x value for the quadratic
  // interpolation to work
  auto ny = workspaceToInterpolate->blocksize();
  if (ny < 3) {
    throw std::runtime_error(
        "Need at least 3 y values to perform quadratic interpolation");
  }
  if (idx > ny - 3) {
    idx = ny - 3;
  }
  // apply equal bins checker?
  auto binWidth =
      workspaceToInterpolate->x(0)[1] - workspaceToInterpolate->x(0)[0];
  // U=0 on at point or bin edge to the left of where x lies
  auto U = (x - workspaceToInterpolate->x(0)[idx]) / binWidth;
  auto &y = workspaceToInterpolate->y(0);
  auto A = (y[idx] - 2 * y[idx + 1] + y[idx + 2]) / 2;
  auto B = (-3 * y[idx] - 4 * y[idx + 1] - y[idx + 2]) / 2;
  auto C = y[idx];
  return exp(A * U * U + B * U + C);
}

double MuscatElastic::scatter(const bool doMultipleScattering, const int nScatters,
                            const double absorbXsection, const Sample &sample,
                            const Geometry::Instrument &instrument,
                            Kernel::PseudoRandomNumberGenerator &rng,
                            const double vmfp, const double sigma_total,
                            const MatrixWorkspace_sptr SOfQ, const double kinc) {
  auto track = start_point(sample, instrument, rng);
  double weight = 1;
  updateWeightAndPosition(track, weight, vmfp, sigma_total, rng);
  if (doMultipleScattering) {
    for (int i = 0; i < nScatters; i++) {
      double QSS = 0;
      q_dir(track, SOfQ, kinc, rng, QSS, weight);
      updateWeightAndPosition(track, weight, vmfp, sigma_total, rng);
    }
  }
  return weight;
}

void MuscatElastic::q_dir(Geometry::Track track,
                          const MatrixWorkspace_sptr SOfQ, double kinc,
                          Kernel::PseudoRandomNumberGenerator &rng, double& QSS, double& weight) {
  auto qvalues = SOfQ->histogram(0).x().rawData();
  double QQ = qvalues.back() * rng.nextValue();
  // T = 2theta
  double cosT = 1 - QQ * QQ / (2 * kinc * kinc);
  double SQ = interpolateLogQuadratic(SOfQ, QQ);
  QSS += QQ * SQ;
  //weight = weight * sigs * SQ * QQ;
}

Geometry::Track
MuscatElastic::start_point(const Sample &sample,
                           const Geometry::Instrument &instrument,
                           Kernel::PseudoRandomNumberGenerator &rng) {
  int MAX_ATTEMPTS = 100;
  for (int i = 0; i < MAX_ATTEMPTS; i++) {
    auto t = generateInitialTrack(sample, instrument, rng);
    int nlinks = sample.getShape().interceptSurface(t);
    if (nlinks > 0) {
      return t;
    }
  }
  throw std::runtime_error("MuscatElastic::start_point() - Unable to "
                           "generate entry point into sample");
}

void MuscatElastic::updateWeightAndPosition(
    Geometry::Track &track, double &weight, double vmfp, double sigma_total,
    Kernel::PseudoRandomNumberGenerator &rng) {
  double dl = track.front().distInsideObject;
  double B9 = 1.0 - exp(-dl / vmfp);
  double vl = -(vmfp * log(1 - rng.nextValue() * B9));
  B9 = B9 / sigma_total;
  inc_xyz(track, vl);
}

Geometry::Track
MuscatElastic::generateInitialTrack(const API::Sample &sample,
                                    const Geometry::Instrument &instrument,
                                    Kernel::PseudoRandomNumberGenerator &rng) {
  const auto frame = instrument.getReferenceFrame();
  auto sampleBox = sample.getShape().getBoundingBox();
  // generate random point on front surface of sample bounding box
  auto ptx = sampleBox.minPoint()[frame->pointingHorizontal()] +
             rng.nextValue() * sampleBox.width()[frame->pointingHorizontal()];
  auto pty = sampleBox.minPoint()[frame->pointingUp()] +
             rng.nextValue() * sampleBox.width()[frame->pointingUp()];
  // perhaps eventually also generate random point on the beam profile?
  auto ptOnBeamProfile = Kernel::V3D();
  ptOnBeamProfile[frame->pointingHorizontal()] = ptx;
  ptOnBeamProfile[frame->pointingUp()] = pty;
  ptOnBeamProfile[frame->pointingAlongBeam()] =
      instrument.getSource()->getPos()[frame->pointingAlongBeam()];
  auto toSample = Kernel::V3D();
  toSample[frame->pointingAlongBeam()] = 1.;
  Geometry::Track trackToSample = Geometry::Track(ptOnBeamProfile, toSample);
  return trackToSample;
}

void MuscatElastic::inc_xyz(Geometry::Track &track, double vl) {
  track.clearIntersectionResults();
  Kernel::V3D position = track.front().entryPoint;
  Kernel::V3D direction = track.direction();
  auto x = position[0] + vl * direction[0];
  auto y = position[1] + vl * direction[1];
  auto z = position[2] + vl * direction[2];
  auto startPoint = track.startPoint();
  startPoint = V3D(x, y, z);
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