// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MonteCarloAbsorption.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/InterpolationOption.h"
#include "MantidAlgorithms/SampleCorrections/DetectorGridDefinition.h"
#include "MantidAlgorithms/SampleCorrections/MCAbsorptionStrategy.h"
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidAlgorithms/SampleCorrections/SparseInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/Interpolate.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::DataObjects::Workspace2D;
namespace PhysicalConstants = Mantid::PhysicalConstants;

/// @cond
namespace {

constexpr int DEFAULT_NEVENTS = 300;
constexpr int DEFAULT_SEED = 123456789;
constexpr int DEFAULT_LATITUDINAL_DETS = 5;
constexpr int DEFAULT_LONGITUDINAL_DETS = 10;

/// Energy (meV) to wavelength (angstroms)
inline double toWavelength(double energy) {
  static const double factor =
      1e10 * PhysicalConstants::h /
      sqrt(2.0 * PhysicalConstants::NeutronMass * PhysicalConstants::meV);
  return factor / sqrt(energy);
}

struct EFixedProvider {
  explicit EFixedProvider(const ExperimentInfo &expt)
      : m_expt(expt), m_emode(expt.getEMode()), m_value(0.0) {
    if (m_emode == DeltaEMode::Direct) {
      m_value = m_expt.getEFixed();
    }
  }
  inline DeltaEMode::Type emode() const { return m_emode; }
  inline double value(const Mantid::detid_t detID) const {
    if (m_emode != DeltaEMode::Indirect)
      return m_value;
    else
      return m_expt.getEFixed(detID);
  }

private:
  const ExperimentInfo &m_expt;
  const DeltaEMode::Type m_emode;
  double m_value;
};
} // namespace
/// @endcond

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(MonteCarloAbsorption)

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

/**
 * Initialize the algorithm
 */
void MonteCarloAbsorption::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input workspace.  The input workspace must "
                  "have X units of wavelength.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The name to use for the output workspace.");
  auto positiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);
  declareProperty("NumberOfWavelengthPoints", EMPTY_INT(), positiveInt,
                  "The number of wavelength points for which a simulation is "
                  "attempted (default: all points)");
  declareProperty(
      "EventsPerPoint", DEFAULT_NEVENTS, positiveInt,
      "The number of \"neutron\" events to generate per simulated point");
  declareProperty("SeedValue", DEFAULT_SEED, positiveInt,
                  "Seed the random number generator with this value");

  InterpolationOption interpolateOpt;
  declareProperty(interpolateOpt.property(), interpolateOpt.propertyDoc());
  declareProperty("SparseInstrument", false,
                  "Enable simulation on special "
                  "instrument with a sparse grid of "
                  "detectors interpolating the "
                  "results to the real instrument.");
  auto threeOrMore = boost::make_shared<Kernel::BoundedValidator<int>>();
  threeOrMore->setLower(3);
  declareProperty(
      "NumberOfDetectorRows", DEFAULT_LATITUDINAL_DETS, threeOrMore,
      "Number of detector rows in the detector grid of the sparse instrument.");
  setPropertySettings(
      "NumberOfDetectorRows",
      std::make_unique<EnabledWhenProperty>(
          "SparseInstrument", ePropertyCriterion::IS_NOT_DEFAULT));
  auto twoOrMore = boost::make_shared<Kernel::BoundedValidator<int>>();
  twoOrMore->setLower(2);
  declareProperty("NumberOfDetectorColumns", DEFAULT_LONGITUDINAL_DETS,
                  twoOrMore,
                  "Number of detector columns in the detector grid "
                  "of the sparse instrument.");
  setPropertySettings(
      "NumberOfDetectorColumns",
      std::make_unique<EnabledWhenProperty>(
          "SparseInstrument", ePropertyCriterion::IS_NOT_DEFAULT));

  // Control the number of attempts made to generate a random point in the
  // object
  declareProperty("MaxScatterPtAttempts", 5000, positiveInt,
                  "Maximum number of tries made to generate a scattering point "
                  "within the sample (+ optional container etc). Objects with "
                  "holes in them, e.g. a thin annulus can cause problems "
                  "if this number is too low.\n"
                  "If a scattering point cannot be generated by increasing "
                  "this value then there is most likely a problem with "
                  "the sample geometry.");
}

/**
 * Execution code
 */
void MonteCarloAbsorption::exec() {
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const int nevents = getProperty("EventsPerPoint");
  const int nlambda = getProperty("NumberOfWavelengthPoints");
  const int seed = getProperty("SeedValue");
  InterpolationOption interpolateOpt;
  interpolateOpt.set(getPropertyValue("Interpolation"));
  const bool useSparseInstrument = getProperty("SparseInstrument");
  const int maxScatterPtAttempts = getProperty("MaxScatterPtAttempts");
  auto outputWS = doSimulation(*inputWS, static_cast<size_t>(nevents), nlambda,
                               seed, interpolateOpt, useSparseInstrument,
                               static_cast<size_t>(maxScatterPtAttempts));
  setProperty("OutputWorkspace", std::move(outputWS));
}

/**
 * Validate the input properties.
 * @return a map where keys are property names and values the found issues
 */
std::map<std::string, std::string> MonteCarloAbsorption::validateInputs() {
  std::map<std::string, std::string> issues;
  const int nlambda = getProperty("NumberOfWavelengthPoints");
  InterpolationOption interpOpt;
  const std::string interpValue = getPropertyValue("Interpolation");
  interpOpt.set(interpValue);
  const auto nlambdaIssue = interpOpt.validateInputSize(nlambda);
  if (!nlambdaIssue.empty()) {
    issues["NumberOfWavelengthPoints"] = nlambdaIssue;
  }
  return issues;
}

/**
 * Run the simulation over the whole input workspace
 * @param inputWS A reference to the input workspace
 * @param nevents Number of MC events per wavelength point to simulate
 * @param nlambda Number of wavelength points to simulate. The remainder
 * are computed using interpolation
 * @param seed Seed value for the random number generator
 * @param interpolateOpt Method of interpolation to compute unsimulated points
 * @param useSparseInstrument If true, use sparse instrument in simulation
 * @param maxScatterPtAttempts The maximum number of tries to generate a
 * scatter point within the object
 * @return A new workspace containing the correction factors & errors
 */
MatrixWorkspace_uptr MonteCarloAbsorption::doSimulation(
    const MatrixWorkspace &inputWS, const size_t nevents, int nlambda,
    const int seed, const InterpolationOption &interpolateOpt,
    const bool useSparseInstrument, const size_t maxScatterPtAttempts) {
  auto outputWS = createOutputWorkspace(inputWS);
  const auto inputNbins = static_cast<int>(inputWS.blocksize());
  if (isEmpty(nlambda) || nlambda > inputNbins) {
    if (!isEmpty(nlambda)) {
      g_log.warning() << "The requested number of wavelength points is larger "
                         "than the spectra size. "
                         "Defaulting to spectra size.\n";
    }
    nlambda = inputNbins;
  }
  std::unique_ptr<const DetectorGridDefinition> detGrid;
  MatrixWorkspace_uptr sparseWS;
  if (useSparseInstrument) {
    const int latitudinalDets = getProperty("NumberOfDetectorRows");
    const int longitudinalDets = getProperty("NumberOfDetectorColumns");
    detGrid = SparseInstrument::createDetectorGridDefinition(
        inputWS, latitudinalDets, longitudinalDets);
    sparseWS = SparseInstrument::createSparseWS(inputWS, *detGrid, nlambda);
  }
  MatrixWorkspace &simulationWS = useSparseInstrument ? *sparseWS : *outputWS;
  const MatrixWorkspace &instrumentWS =
      useSparseInstrument ? simulationWS : inputWS;
  // Cache information about the workspace that will be used repeatedly
  auto instrument = instrumentWS.getInstrument();
  const auto nhists = static_cast<int64_t>(instrumentWS.getNumberHistograms());
  const auto nbins = static_cast<int>(simulationWS.blocksize());

  EFixedProvider efixed(instrumentWS);
  auto beamProfile = createBeamProfile(*instrument, inputWS.sample());

  // Configure progress
  const int lambdaStepSize = nbins / nlambda;
  Progress prog(this, 0.0, 1.0, nhists * nbins / lambdaStepSize);
  prog.setNotifyStep(0.01);
  const std::string reportMsg = "Computing corrections";

  // Configure strategy
  MCAbsorptionStrategy strategy(*beamProfile, inputWS.sample(), nevents,
                                maxScatterPtAttempts);

  const auto &spectrumInfo = simulationWS.spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(simulationWS))
  for (int64_t i = 0; i < nhists; ++i) {
    PARALLEL_START_INTERUPT_REGION

    auto &outE = simulationWS.mutableE(i);
    // The input was cloned so clear the errors out
    outE = 0.0;
    // Final detector position
    if (!spectrumInfo.hasDetectors(i)) {
      continue;
    }
    // Per spectrum values
    const auto &detPos = spectrumInfo.position(i);
    const double lambdaFixed =
        toWavelength(efixed.value(spectrumInfo.detector(i).getID()));
    MersenneTwister rng(seed);

    auto &outY = simulationWS.mutableY(i);
    const auto lambdas = simulationWS.points(i);
    // Simulation for each requested wavelength point
    for (int j = 0; j < nbins; j += lambdaStepSize) {
      prog.report(reportMsg);
      const double lambdaStep = lambdas[j];
      double lambdaIn(lambdaStep), lambdaOut(lambdaStep);
      if (efixed.emode() == DeltaEMode::Direct) {
        lambdaIn = lambdaFixed;
      } else if (efixed.emode() == DeltaEMode::Indirect) {
        lambdaOut = lambdaFixed;
      } else {
        // elastic case already initialized
      }
      std::tie(outY[j], std::ignore) =
          strategy.calculate(rng, detPos, lambdaIn, lambdaOut);

      // Ensure we have the last point for the interpolation
      if (lambdaStepSize > 1 && j + lambdaStepSize >= nbins && j + 1 != nbins) {
        j = nbins - lambdaStepSize - 1;
      }
    }

    // Interpolate through points not simulated
    if (!useSparseInstrument && lambdaStepSize > 1) {
      auto histnew = simulationWS.histogram(i);
      if (lambdaStepSize < nbins) {
        interpolateOpt.applyInplace(histnew, lambdaStepSize);
      } else {
        std::fill(histnew.mutableY().begin() + 1, histnew.mutableY().end(),
                  histnew.y()[0]);
      }

      outputWS->setHistogram(i, histnew);
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (useSparseInstrument) {
    interpolateFromSparse(*outputWS, simulationWS, interpolateOpt, *detGrid);
  }

  return outputWS;
}

MatrixWorkspace_uptr MonteCarloAbsorption::createOutputWorkspace(
    const MatrixWorkspace &inputWS) const {
  MatrixWorkspace_uptr outputWS = DataObjects::create<Workspace2D>(inputWS);
  // The algorithm computes the signal values at bin centres so they should
  // be treated as a distribution
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Attenuation factor");
  return outputWS;
}

/**
 * Create the beam profile. Currently only supports Rectangular. The dimensions
 * are either specified by those provided by `SetBeam` algorithm or default
 * to the width and height of the samples bounding box
 * @param instrument A reference to the instrument object
 * @param sample A reference to the sample object
 * @return A new IBeamProfile object
 */
std::unique_ptr<IBeamProfile>
MonteCarloAbsorption::createBeamProfile(const Instrument &instrument,
                                        const Sample &sample) const {
  const auto frame = instrument.getReferenceFrame();
  const auto source = instrument.getSource();

  auto beamWidthParam = source->getNumberParameter("beam-width");
  auto beamHeightParam = source->getNumberParameter("beam-height");
  double beamWidth(-1.0), beamHeight(-1.0);
  if (beamWidthParam.size() == 1 && beamHeightParam.size() == 1) {
    beamWidth = beamWidthParam[0];
    beamHeight = beamHeightParam[0];
  } else {
    const auto bbox = sample.getShape().getBoundingBox().width();
    beamWidth = bbox[frame->pointingHorizontal()];
    beamHeight = bbox[frame->pointingUp()];
  }
  return std::make_unique<RectangularBeamProfile>(*frame, source->getPos(),
                                                  beamWidth, beamHeight);
}

void MonteCarloAbsorption::interpolateFromSparse(
    MatrixWorkspace &targetWS, const MatrixWorkspace &sparseWS,
    const Mantid::Algorithms::InterpolationOption &interpOpt,
    const DetectorGridDefinition &detGrid) {
  const auto &spectrumInfo = targetWS.spectrumInfo();
  const auto samplePos = spectrumInfo.samplePosition();
  const auto refFrame = targetWS.getInstrument()->getReferenceFrame();
  PARALLEL_FOR_IF(Kernel::threadSafe(targetWS, sparseWS))
  for (int64_t i = 0; i < static_cast<decltype(i)>(spectrumInfo.size()); ++i) {
    PARALLEL_START_INTERUPT_REGION
    const auto detPos = spectrumInfo.position(i) - samplePos;
    double lat, lon;
    std::tie(lat, lon) =
        SparseInstrument::geographicalAngles(detPos, *refFrame);
    const auto nearestIndices = detGrid.nearestNeighbourIndices(lat, lon);
    const auto spatiallyInterpHisto =
        SparseInstrument::interpolateFromDetectorGrid(lat, lon, sparseWS,
                                                      nearestIndices);
    if (spatiallyInterpHisto.size() > 1) {
      auto targetHisto = targetWS.histogram(i);
      interpOpt.applyInPlace(spatiallyInterpHisto, targetHisto);
      targetWS.setHistogram(i, targetHisto);
    } else {
      targetWS.mutableY(i) = spatiallyInterpHisto.y().front();
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}
} // namespace Algorithms
} // namespace Mantid
